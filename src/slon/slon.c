/*-------------------------------------------------------------------------
 * slon.c
 *
 *	The control framework for the node daemon.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: slon.c,v 1.5 2003-12-16 17:00:34 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "libpq-fe.h"

#include "slon.h"


/* ----------
 * Global data
 * ----------
 */
pid_t					slon_pid;
char				   *local_cluster_name = NULL;
char				   *local_namespace = NULL;
char				   *local_conninfo = NULL;
int						local_nodeid = -1;
int						local_nodeactive = 0;
char				   *local_nodecomment = NULL;

SlonSet				   *set_list_head = NULL;
SlonSet				   *set_list_tail = NULL;

/* ----------
 * Local data
 * ----------
 */
static pthread_mutex_t	config_lock = PTHREAD_MUTEX_INITIALIZER;
static SlonNode		   *node_list_head = NULL;
static SlonNode		   *node_list_tail = NULL;

static pthread_t		local_event_thread;
static pthread_t		local_cleanup_thread;
static pthread_t		local_sync_thread;


struct to_activate {
	int					no_id;

	struct to_activate *prev;
	struct to_activate *next;
};
static struct to_activate *to_activate_head = NULL;
static struct to_activate *to_activate_tail = NULL;


/* ----------
 * Local functions
 * ----------
 */
static SlonNode	   *slon_findNode(int no_id);
static void			slon_startStopNodeThread(SlonNode *node);


/* ----------
 * main
 * ----------
 */
int
main (int argc, const char *argv[])
{
	char	   *cp1;
	char	   *cp2;
	char		query[1024];
	PGresult   *res;
	int			i, n;
	PGconn	   *startup_conn;
	SlonNode   *node;

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s clustername conninfo\n", argv[0]);
		return 1;
	}

	/*
	 * Remember the cluster name and build the properly quoted 
	 * namespace identifier
	 */
	slon_pid = getpid();
	local_cluster_name	= (char *)argv[1];
	local_namespace		= malloc(strlen(argv[1]) * 2 + 4);
	cp2 = local_namespace;
	*cp2++ = '"';
	*cp2++ = '_';
	for (cp1 = (char *)argv[1]; *cp1; cp1++)
	{
		if (*cp1 == '"')
			*cp2++ = '"';
		*cp2++ = *cp1;
	}
	*cp2++ = '"';
	*cp2 = '\0';

	/*
	 * Remember the connection information for the local node.
	 */
	local_conninfo = (char *)argv[2];

	/*
	 * Connect to the local database for reading the initial configuration
	 */
	startup_conn = PQconnectdb(local_conninfo);
	if (startup_conn == NULL)
	{
		fprintf(stderr, "PQconnectdb() failed\n");
		slon_exit(-1);
	}
	if (PQstatus(startup_conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Cannot connect to local database - %s",
				PQerrorMessage(startup_conn));
		PQfinish(startup_conn);
		slon_exit(-1);
	}

	/*
	 * Get our local node ID
	 */
	local_nodeid = slon_getLocalNodeId(startup_conn);
	if (local_nodeid < 0)
	{
		fprintf(stderr, "Node is not initialized properly\n");
		slon_exit(-1);
	}

printf("main: local node id = %d\n", local_nodeid);

	/*
	 * Start the event scheduling system
	 */
	if (sched_start_mainloop() < 0)
		slon_exit(-1);

	/*
	 * Read configuration table sl_node
	 */
	snprintf(query, 1024, "select no_id, no_active, no_comment from %s.sl_node",
			local_namespace);
	res = PQexec(startup_conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Cannot get node list - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		slon_exit(-1);
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int		no_id		= (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int		no_active	= (*PQgetvalue(res, i, 1) == 't') ? 1 : 0;
		char   *no_comment	= PQgetvalue(res, i, 2);

		if (no_id == local_nodeid)
		{
			/*
			 * Complete our own local node entry
			 */
			local_nodeactive  = no_active;
			local_nodecomment = strdup(no_comment);
		}
		else
		{
			/*
			 * Add a remote node
			 */
			slon_storeNode(no_id, no_comment);

			/*
			 * If it is active, remember for activation just before
			 * we start processing events.
			 */
			if (no_active)
			{
				struct to_activate	*anode;

				anode = (struct to_activate *)malloc(sizeof(struct to_activate));
				anode->no_id = no_id;
				DLLIST_ADD_TAIL(to_activate_head, to_activate_tail, anode);
			}
		}
	}
	PQclear(res);

	/*
	 * Read configuration table sl_path - the interesting pieces
	 */
	snprintf(query, 1024, "select pa_server, pa_conninfo, pa_connretry
			from %s.sl_path where pa_client = %d",
			local_namespace, local_nodeid);
	res = PQexec(startup_conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Cannot get path config - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		slon_exit(-1);
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int		pa_server		= (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		char   *pa_conninfo		= PQgetvalue(res, i, 1);
		int		pa_connretry	= (int) strtol(PQgetvalue(res, i, 2), NULL, 10);

		slon_storePath(pa_server, pa_conninfo, pa_connretry);
	}
	PQclear(res);

	/*
	 * Read configuration table sl_listen - the interesting pieces
	 */
	snprintf(query, 1024, "select li_origin, li_provider
			from %s.sl_listen where li_receiver = %d",
			local_namespace, local_nodeid);
	res = PQexec(startup_conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Cannot get listen config - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		slon_exit(-1);
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int		li_origin	= (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int		li_provider	= (int) strtol(PQgetvalue(res, i, 1), NULL, 10);

		slon_storeListen(li_origin, li_provider);
	}
	PQclear(res);

	/*
	 * Read configuration table sl_set
	 */
	snprintf(query, 1024, "select set_id, set_origin, set_comment
			from %s.sl_set",
			local_namespace);
	res = PQexec(startup_conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Cannot get set config - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		slon_exit(-1);
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int		set_id		= (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int		set_origin	= (int) strtol(PQgetvalue(res, i, 1), NULL, 10);
		char   *set_comment = PQgetvalue(res, i, 2);

		slon_storeSet(set_id, set_origin, set_comment);
	}
	PQclear(res);

	/*
	 * Read configuration table sl_subscribe - our subscriptions only
	 */
	snprintf(query, 1024, "select sub_set, sub_provider,
			sub_forward, sub_active
			from %s.sl_subscribe
			where sub_receiver = %d",
			local_namespace, local_nodeid);
	res = PQexec(startup_conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Cannot get subscription config - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		slon_exit(-1);
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int		sub_set			= (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int		sub_provider	= (int) strtol(PQgetvalue(res, i, 1), NULL, 10);
		int		sub_forward		= (strcmp(PQgetvalue(res, i, 2), "t") == 0) ?
									1 : 0;
		int		sub_active		= (strcmp(PQgetvalue(res, i, 3), "t") == 0) ?
									1 : 0;

		slon_storeSubscribe(sub_set, sub_provider, sub_forward);
		if (sub_active)
			slon_enableSubscription(sub_set);
	}
	PQclear(res);

	/*
	 * Done with the startup, don't need the local connection any more.
	 */
	PQfinish(startup_conn);

	/*
	 * Enable all nodes that are active
	 */
	while (to_activate_head != NULL)
	{
		struct to_activate *anode = to_activate_head;

		slon_enableNode(anode->no_id);
		DLLIST_REMOVE(to_activate_head, to_activate_tail, anode);
	}

	/*
	 * Create the local event thread that is monitoring
	 * the local node for administrative events to adjust the
	 * configuration at runtime.
	 */
	if (pthread_create(&local_event_thread, NULL, slon_localEventThread, NULL) < 0)
	{
		perror("pthread_create()");
		slon_exit(-1);
	}

	/*
	 * Create the local cleanup thread that will remove old
	 * events and log data.
	 */
	if (pthread_create(&local_cleanup_thread, NULL, slon_localCleanupThread, NULL) < 0)
	{
		perror("pthread_create()");
		slon_exit(-1);
	}

	/*
	 * Create the local sync thread that will generate SYNC
	 * events if we had local database updates.
	 */
	if (pthread_create(&local_sync_thread, NULL, slon_localSyncThread, NULL) < 0)
	{
		perror("pthread_create()");
		slon_exit(-1);
	}

	/*
	 * Wait until the scheduler has shut down all remote connections
	 */
	if (sched_wait_mainloop() < 0)
		return -1;

	/*
	 * Wait for all remote threads to finish
	 */
	pthread_mutex_lock(&config_lock);
	for (node = node_list_head; node; node = node->next)
	{
		pthread_join(node->event_thread, NULL);
		pthread_mutex_lock(&(node->node_lock));
		node->have_thread = false;
		pthread_mutex_unlock(&(node->node_lock));
	}
	pthread_mutex_unlock(&config_lock);

	/*
	 * Wait for the local threads to finish
	 */
	if (pthread_join(local_event_thread, NULL) < 0)
		perror("pthread_join()");

	if (pthread_join(local_cleanup_thread, NULL) < 0)
		perror("pthread_join()");

	if (pthread_join(local_sync_thread, NULL) < 0)
		perror("pthread_join()");

printf("main: done\n");
	return 0;
}


/* ----------
 * slon_storeNode
 * ----------
 */
void
slon_storeNode(int no_id, char *no_comment)
{
	SlonNode	   *node = slon_findNode(no_id);

	/*
	 * If we have that node already, just change the comment field.
	 */
	if (node)
	{
printf("slon_storeNode: no_id=%d: UPD no_comment='%s'\n", no_id, no_comment);
		free(node->no_comment);
		node->no_comment = strdup(no_comment);

		pthread_mutex_unlock(&(node->node_lock));
		return;
	}

	/*
	 * Add the new node to our in-memory configuration.
	 */
printf("slon_storeNode: no_id=%d: NEW no_comment='%s'\n", no_id, no_comment);
	node = (SlonNode *)malloc(sizeof(SlonNode));
	memset (node, 0, sizeof(SlonNode));

	node->no_id      = no_id;
	node->no_active  = false;
	node->no_comment = strdup(no_comment);
	pthread_mutex_init(&(node->node_lock), NULL);

	DLLIST_ADD_TAIL(node_list_head, node_list_tail, node);
}


/* ----------
 * slon_enableNode
 * ----------
 */
void
slon_enableNode(int no_id)
{
	SlonNode	   *node = slon_findNode(no_id);

	if (!node)
	{
		fprintf(stderr, "slon_enableNode: unknown node ID %d\n", no_id);
		slon_abort();
		return;
	}

	/*
	 * Activate the node
	 */
printf("slon_enableNode: no_id=%d\n", no_id);
	node->no_active = true;

	slon_startStopNodeThread(node);

	pthread_mutex_unlock(&(node->node_lock));
}


/* ----------
 * slon_dropNode
 * ----------
 */
void
slon_dropNode(int no_id)
{
	printf("TODO: slon_dropNode\n");
}


/* ----------
 * slon_storePath
 * ----------
 */
void
slon_storePath(int pa_server, char *pa_conninfo, int pa_connretry)
{
	SlonNode	   *node = slon_findNode(pa_server);

	if (!node)
	{
		fprintf(stderr, "slon_storePath: unknown node ID %d\n", pa_server);
		slon_abort();
		return;
	}

	/*
	 * Store the (new) conninfo to the node
	 */
printf("slon_storePath: pa_server=%d pa_conndinfo=\"%s\"\n", 
pa_server, pa_conninfo);
	if (node->pa_conninfo != NULL)
		free(node->pa_conninfo);
	node->pa_conninfo  = strdup(pa_conninfo);
	node->pa_connretry = pa_connretry;

	/*
	 * Eventually start communicating with that node
	 */
	slon_startStopNodeThread(node);

	pthread_mutex_unlock(&(node->node_lock));
}


/* ----------
 * slon_storeListen
 * ----------
 */
void
slon_storeListen(int li_origin, int li_provider)
{
	SlonNode	   *node = slon_findNode(li_provider);
	SlonListen	   *listen;

	if (!node)
	{
		fprintf(stderr, "slon_storeListen: unknown node ID %d\n", li_provider);
		slon_abort();
		return;
	}

	/*
	 * Check if we already listen for events from that origin
	 * at this provider.
	 */
	for (listen = node->li_list_head; listen; listen = listen->next)
	{
		if (listen->li_origin == li_origin)
		{
printf("slon_storeListen: already listening for li_origin=%d li_provider=%d\n",
li_origin, li_provider);
			pthread_mutex_unlock(&(node->node_lock));
			return;
		}
	}

	/*
	 * Add the new event origin to the provider (this node)
	 */
printf("slon_storeListen: add li_origin=%d to li_provider=%d\n",
li_origin, li_provider);
	listen = (SlonListen *)malloc(sizeof(SlonListen));
	memset(listen, 0, sizeof(SlonListen));

	listen->li_origin = li_origin;
	DLLIST_ADD_TAIL(node->li_list_head, node->li_list_tail, listen);

	/*
	 * Eventually start communicating with that node
	 */
	slon_startStopNodeThread(node);

	pthread_mutex_unlock(&(node->node_lock));
}


void
slon_storeSet(int set_id, int set_origin, char *set_comment)
{
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Try to update an existing set configuration
	 */
	for (set = set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
printf("slon_storeSet: update set_id=%d set_origin=%d set_comment='%s'\n",
set_id, set_origin, set_comment);
			free(set->set_comment);
			set->set_origin = set_origin;
			set->set_comment = strdup(set_comment);
			pthread_mutex_unlock(&config_lock);
			return;
		}
	}

	/*
	 * Add a new set to the configuration
	 */
printf("slon_storeSet: new set_id=%d set_origin=%d set_comment='%s'\n",
set_id, set_origin, set_comment);
	set = (SlonSet *)malloc(sizeof(SlonSet));
	memset(set, 0, sizeof(SlonSet));

	set->set_id = set_id;
	set->set_origin = set_origin;
	set->set_comment = strdup(set_comment);

	set->sub_provider = -1;

	DLLIST_ADD_TAIL(set_list_head, set_list_tail, set);
	pthread_mutex_unlock(&config_lock);
}


void
slon_storeSubscribe(int sub_set, int sub_provider, int sub_forward)
{
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Find the set and store subscription information
	 */
	for (set = set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
printf("slon_storeSubscribe: sub_set=%d sub_provider=%d sub_forward=%d\n",
sub_set, sub_provider, sub_forward);
			if (set->sub_provider < 0)
				set->sub_active = 0;
			set->sub_provider = sub_provider;
			set->sub_forward  = sub_forward;
			pthread_mutex_unlock(&config_lock);
			return;
		}
	}

	fprintf(stderr, "slon_storeSubscribe: set %d not found\n", sub_set);
	pthread_mutex_unlock(&config_lock);
	slon_abort();
}


void
slon_enableSubscription(int sub_set)
{
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Find the set and enable its subscription
	 */
	for (set = set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
printf("slon_enableSubscription: sub_set=%d\n", sub_set);
			if (set->sub_provider >= 0)
				set->sub_active = 1;
			pthread_mutex_unlock(&config_lock);
			return;
		}
	}

	fprintf(stderr, "slon_enableSubscription: set %d not found\n", sub_set);
	pthread_mutex_unlock(&config_lock);
	slon_abort();
}


/* ----
 * slon_getLocalNodeId
 *
 *	Query a connection for the value of sequence sl_local_node_id
 * ----
 */
int
slon_getLocalNodeId(PGconn *conn)
{
	char		query[1024];
	PGresult   *res;
	int			retval;

	snprintf(query, 1024, "select last_value::int4 from %s.sl_local_node_id",
			local_namespace);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "cannot get sl_local_node_id - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr, "query '%s' returned %d rows (expected 1)\n",
				query, PQntuples(res));
		PQclear(res);
		return -1;
	}

	retval = strtol(PQgetvalue(res, 0, 0), NULL, 10);
	PQclear(res);
	
	return retval;
}


void
slon_exit(int code)
{
	exit(code);
}


/* ----------
 * slon_connectdb
 * ----------
 */
SlonConn *
slon_connectdb(char *conninfo, char *symname)
{
	PGconn	   *dbconn;
	SlonConn   *conn;

	dbconn = PQconnectdb(conninfo);
	if (dbconn == NULL)
	{
		fprintf(stderr, "slon_connectdb: PQconnectdb(\"%s\") failed\n",
				conninfo);
		return NULL;
	}
	if (PQstatus(dbconn) != CONNECTION_OK)
	{
		fprintf(stderr, "slon_connectdb: PQconnectdb(\"%s\") failed - %s",
				conninfo, PQerrorMessage(dbconn));
		PQfinish(dbconn);
		return NULL;
	}

	conn = (SlonConn *)malloc(sizeof(SlonConn));
	memset(conn, 0, sizeof(SlonConn));
	conn->symname = strdup(symname);

	pthread_mutex_init(&(conn->conn_lock), NULL);
	pthread_cond_init(&(conn->conn_cond), NULL);
	pthread_mutex_lock(&(conn->conn_lock));
	
	conn->dbconn = dbconn;

	return conn;
}


/* ----------
 * slon_disconnectdb
 * ----------
 */
void
slon_disconnectdb(SlonConn *conn)
{
	PQfinish(conn->dbconn);
	pthread_mutex_unlock(&(conn->conn_lock));
	pthread_mutex_destroy(&(conn->conn_lock));
	free(conn->symname);
	free(conn);
}


/* ----------
 * slon_make_dummyconn
 * ----------
 */
SlonConn *
slon_make_dummyconn(char *symname)
{
	SlonConn   *conn;

	conn = (SlonConn *)malloc(sizeof(SlonConn));
	memset(conn, 0, sizeof(SlonConn));
	conn->symname = strdup(symname);

	pthread_mutex_init(&(conn->conn_lock), NULL);
	pthread_cond_init(&(conn->conn_cond), NULL);
	pthread_mutex_lock(&(conn->conn_lock));
	
	return conn;
}


/* ----------
 * slon_free_dummyconn
 * ----------
 */
void
slon_free_dummyconn(SlonConn *conn)
{
	pthread_mutex_unlock(&(conn->conn_lock));
	pthread_mutex_destroy(&(conn->conn_lock));
	free(conn->symname);
	free(conn);
}


/* ----------
 * slon_findNode
 * ----------
 */
static SlonNode *
slon_findNode(int no_id)
{
	SlonNode	   *node;

	pthread_mutex_lock(&config_lock);
	
	for (node = node_list_head; node; node = node->next)
	{
		if (node->no_id == no_id)
		{
			pthread_mutex_lock(&(node->node_lock));
			pthread_mutex_unlock(&config_lock);
			return node;
		}
	}

	pthread_mutex_unlock(&config_lock);
	return NULL;
}


/* ----------
 * slon_startStopNodeThread
 * ----------
 */
static void
slon_startStopNodeThread(SlonNode *node)
{
	if (node->have_thread)
	{
printf("TODO: slon_startStopNodeThread() have_thread branch\n");
	}
	else
	{
		if (!local_nodeactive)
			return;
		if (!node->no_active)
			return;
		if (node->pa_conninfo == NULL)
			return;
		if (node->li_list_head == NULL)
			return;

		/*
		 * All signs are go ... start the remote connection
		 */
		if (pthread_create(&(node->event_thread), NULL,
				slon_remoteEventThread, (void *)node) < 0)
		{
			perror("slon_startStopNodeThread: pthread_create()");
			return;
		}
		node->have_thread = true;
printf("slon_startStopNodeThread() thread for node %d created\n", node->no_id);
	}
}


