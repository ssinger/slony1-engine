/*-------------------------------------------------------------------------
 * runtime_config.c
 *
 *	Functions maintaining the in-memory configuration information
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#include "slon.h"


/* ----------
 * Global data
 * ----------
 */
pid_t		slon_pid;

#ifndef WIN32
pthread_mutex_t slon_watchdog_lock;
pid_t		slon_watchdog_pid;
pid_t		slon_worker_pid;
#endif
char	   *rtcfg_cluster_name = NULL;
char	   *rtcfg_namespace = NULL;
char	   *rtcfg_conninfo = NULL;
int			rtcfg_nodeid = -1;
int			rtcfg_nodeactive = 0;
char	   *rtcfg_nodecomment = NULL;
char		rtcfg_lastevent[64];

SlonSet    *rtcfg_set_list_head = NULL;
SlonSet    *rtcfg_set_list_tail = NULL;
SlonNode   *rtcfg_node_list_head = NULL;
SlonNode   *rtcfg_node_list_tail = NULL;


/* ----------
 * Local data
 * ----------
 */
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cfgseq_lock = PTHREAD_MUTEX_INITIALIZER;
static int64 cfgseq = 0;

struct to_activate
{
	int			no_id;

	struct to_activate *prev;
	struct to_activate *next;
};
static struct to_activate *to_activate_head = NULL;
static struct to_activate *to_activate_tail = NULL;


/* ----------
 * Local functions
 * ----------
 */
static void rtcfg_startStopNodeThread(SlonNode * node);


/* ----------
 * rtcfg_lock
 * ----------
 */
void
rtcfg_lock(void)
{
	pthread_mutex_lock(&config_lock);
}


/* ----------
 * rtcfg_unlock
 * ----------
 */
void
rtcfg_unlock(void)
{
	pthread_mutex_unlock(&config_lock);
}


/* ----------
 * rtcfg_storeNode
 * ----------
 */
void
rtcfg_storeNode(int no_id, char *no_comment)
{
	SlonNode   *node;

	rtcfg_lock();

	/*
	 * If we have that node already, just change the comment field.
	 */
	node = rtcfg_findNode(no_id);
	if (node)
	{
		slon_log(SLON_CONFIG,
				 "storeNode: no_id=%d no_comment='%s' - update node\n",
				 no_id, no_comment);

		free(node->no_comment);
		node->no_comment = strdup(no_comment);

		rtcfg_unlock();
		return;
	}

	/*
	 * Add the new node to our in-memory configuration.
	 */
	slon_log(SLON_CONFIG,
			 "storeNode: no_id=%d no_comment='%s'\n",
			 no_id, no_comment);

	node = (SlonNode *) malloc(sizeof(SlonNode));
	if (node == NULL)
	{
		perror("rtcfg_storeNode: malloc()");
		slon_retry();
	}
	memset(node, 0, sizeof(SlonNode));

	node->no_id = no_id;
	node->no_active = false;
	node->no_comment = strdup(no_comment);
	node->last_snapshot = strdup("1:1:");
	pthread_mutex_init(&(node->message_lock), NULL);
	pthread_cond_init(&(node->message_cond), NULL);

	DLLIST_ADD_TAIL(rtcfg_node_list_head, rtcfg_node_list_tail, node);

	rtcfg_unlock();
	rtcfg_seq_bump();
}


/* ----------
 * rtcfg_setNodeLastEvent()
 *
 * Set the last_event field in the node runtime structure.
 *
 * Returns: 0 if the event_seq is <= the known value -1 if the node is
 * not known event_seq otherwise
 * ----------
 */
int64
rtcfg_setNodeLastEvent(int no_id, int64 event_seq)
{
	SlonNode   *node;
	int64		retval;

	rtcfg_lock();
	if ((node = rtcfg_findNode(no_id)) != NULL)
	{
		if (node->last_event < event_seq)
		{
			node->last_event = event_seq;
			retval = event_seq;
		}
		else
			retval = 0;
	}
	else
		retval = -1;

	rtcfg_unlock();

	slon_log(SLON_DEBUG2,
			 "setNodeLastEvent: no_id=%d event_seq=" INT64_FORMAT "\n",
			 no_id, retval);

	return retval;
}


/* ----------
 * rtcfg_getNodeLastEvent
 *
 * Read the nodes last_event field
 * ----------
 */
int64
rtcfg_getNodeLastEvent(int no_id)
{
	SlonNode   *node;
	int64		retval;

	rtcfg_lock();
	if ((node = rtcfg_findNode(no_id)) != NULL)
	{
		retval = node->last_event;
	}
	else
		retval = -1;

	rtcfg_unlock();

	return retval;
}


/* ----------
 * rtcfg_setNodeLastSnapshot()
 *
 * Set the last_snapshot field in the node runtime structure.
 * ----------
 */
void
rtcfg_setNodeLastSnapshot(int no_id, char *snapshot)
{
	SlonNode   *node;

	if (snapshot == NULL || strcmp(snapshot, "") == 0)
		snapshot = "1:1:";

	rtcfg_lock();
	if ((node = rtcfg_findNode(no_id)) != NULL)
	{
		if (node->last_snapshot != NULL)
			free(node->last_snapshot);

		node->last_snapshot = strdup(snapshot);
	}

	rtcfg_unlock();

	slon_log(SLON_DEBUG2,
			 "setNodeLastSnapshot: no_id=%d snapshot='%s'\n",
			 no_id, snapshot);
}


/* ----------
 * rtcfg_getNodeLastSnapshot
 *
 * Read the nodes last_snapshot field
 * ----------
 */
char *
rtcfg_getNodeLastSnapshot(int no_id)
{
	SlonNode   *node;
	char	   *retval;

	rtcfg_lock();
	if ((node = rtcfg_findNode(no_id)) != NULL)
	{
		retval = node->last_snapshot;
	}
	else
		retval = NULL;

	rtcfg_unlock();

	return retval;
}


/* ----------
 * rtcfg_enableNode
 * ----------
 */
void
rtcfg_enableNode(int no_id)
{
	SlonNode   *node;

	rtcfg_lock();

	node = rtcfg_findNode(no_id);
	if (!node)
	{
		rtcfg_unlock();

		slon_log(SLON_FATAL,
				 "enableNode: unknown node ID %d\n", no_id);
		slon_retry();
		return;
	}

	/*
	 * Activate the node
	 */
	slon_log(SLON_CONFIG,
			 "enableNode: no_id=%d\n", no_id);
	node->no_active = true;

	rtcfg_unlock();
	rtcfg_seq_bump();

	rtcfg_startStopNodeThread(node);
}


/* ----------
 * slon_disableNode
 * ----------
 */
void
rtcfg_disableNode(int no_id)
{
	SlonNode   *node;

	rtcfg_lock();

	node = rtcfg_findNode(no_id);
	if (!node)
	{
		rtcfg_unlock();

		slon_log(SLON_FATAL,
				 "enableNode: unknown node ID %d\n", no_id);
		slon_retry();
		return;
	}

	/*
	 * Deactivate the node
	 */
	slon_log(SLON_CONFIG,
			 "disableNode: no_id=%d\n", no_id);
	node->no_active = false;

	rtcfg_unlock();
	rtcfg_seq_bump();

	/*
	 * rtcfg_startStopNodeThread(node);
	 */
}


/* ----------
 * rtcfg_findNode
 * ----------
 */
SlonNode *
rtcfg_findNode(int no_id)
{
	SlonNode   *node;

	for (node = rtcfg_node_list_head; node; node = node->next)
	{
		if (node->no_id == no_id)
			return node;
	}

	return NULL;
}


/* ----------
 * rtcfg_storePath
 * ----------
 */
void
rtcfg_storePath(int pa_server, char *pa_conninfo, int pa_connretry)
{
	SlonNode   *node;

	rtcfg_lock();

	node = rtcfg_findNode(pa_server);
	if (!node)
	{

		rtcfg_unlock();

		slon_log(SLON_WARN,
			   "storePath: unknown node ID %d - event pending\n", pa_server);
		rtcfg_storeNode(pa_server, "<event pending>");

		rtcfg_lock();
		node = rtcfg_findNode(pa_server);
	}

	/*
	 * Store the (new) conninfo to the node
	 */
	slon_log(SLON_CONFIG,
			 "storePath: pa_server=%d pa_client=%d "
			 "pa_conninfo=\"%s\" pa_connretry=%d\n",
			 pa_server, rtcfg_nodeid, pa_conninfo, pa_connretry);
	if (node->pa_conninfo != NULL)
		free(node->pa_conninfo);
	node->pa_conninfo = strdup(pa_conninfo);
	node->pa_connretry = pa_connretry;

	rtcfg_unlock();
	rtcfg_seq_bump();

	/*
	 * Eventually start communicating with that node
	 */
	rtcfg_startStopNodeThread(node);
}


/* ----------
 * rtcfg_dropPath
 * ----------
 */
void
rtcfg_dropPath(int pa_server)
{
	SlonNode   *node;
	SlonListen *listen;

	rtcfg_lock();

	node = rtcfg_findNode(pa_server);
	if (!node)
	{

		rtcfg_unlock();

		slon_log(SLON_WARN,
				 "dropPath: unknown node ID %d\n", pa_server);

		return;
	}

	/*
	 * Drop all listen information as well at this provider. Without a path we
	 * cannot listen.
	 */
	while (node->listen_head != NULL)
	{
		listen = node->listen_head;

		DLLIST_REMOVE(node->listen_head, node->listen_tail, listen);
		free(listen);
	}

	/*
	 * Remove the conninfo.
	 */
	slon_log(SLON_CONFIG,
			 "dropPath: pa_server=%d pa_client=%d\n",
			 pa_server, rtcfg_nodeid);
	if (node->pa_conninfo != NULL)
		free(node->pa_conninfo);
	node->pa_conninfo = NULL;
	node->pa_connretry = 0;

	rtcfg_unlock();
	rtcfg_seq_bump();

	/*
	 * Eventually start communicating with that node
	 */
	rtcfg_startStopNodeThread(node);
}


/* ----------
 * rtcfg_storeListen
 * ----------
 */
void
rtcfg_reloadListen(PGconn *db)
{
	SlonDString query;
	PGresult   *res;
	int			i,
				n;
	SlonNode   *node;
	SlonListen *listen;

	/*
	 * Clear out the entire Listen configuration
	 */
	rtcfg_lock();
	for (node = rtcfg_node_list_head; node; node = node->next)
	{
		while ((listen = node->listen_head) != NULL)
		{
			DLLIST_REMOVE(node->listen_head, node->listen_tail, listen);
			free(listen);
		}
	}
	rtcfg_unlock();
	rtcfg_seq_bump();

	/*
	 * Read configuration table sl_listen - the interesting pieces
	 */
	dstring_init(&query);

	slon_mkquery(&query,
				 "select li_origin, li_provider "
				 "from %s.sl_listen where li_receiver = %d",
				 rtcfg_namespace, rtcfg_nodeid);
	res = PQexec(db, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "Cannot get listen config - %s",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int			li_origin = (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int			li_provider = (int) strtol(PQgetvalue(res, i, 1), NULL, 10);

		rtcfg_storeListen(li_origin, li_provider);
	}
	PQclear(res);

	dstring_free(&query);

	for (node = rtcfg_node_list_head; node; node = node->next)
	{
		rtcfg_startStopNodeThread(node);
	}
}


/* ----------
 * rtcfg_storeListen
 * ----------
 */
void
rtcfg_storeListen(int li_origin, int li_provider)
{
	SlonNode   *node;
	SlonListen *listen;

	rtcfg_lock();

	node = rtcfg_findNode(li_provider);
	if (!node)
	{
		slon_log(SLON_FATAL,
				 "storeListen: unknown node ID %d\n", li_provider);
		slon_retry();
		return;
	}

	/*
	 * Check if we already listen for events from that origin at this
	 * provider.
	 */
	for (listen = node->listen_head; listen; listen = listen->next)
	{
		if (listen->li_origin == li_origin)
		{
			slon_log(SLON_DEBUG2,
					 "storeListen: li_origin=%d li_receiver=%d "
					 "li_provider=%d - already listening\n",
					 li_origin, rtcfg_nodeid, li_provider);
			rtcfg_unlock();
			return;
		}
	}

	/*
	 * Add the new event origin to the provider (this node)
	 */
	slon_log(SLON_CONFIG,
			 "storeListen: li_origin=%d li_receiver=%d li_provider=%d\n",
			 li_origin, rtcfg_nodeid, li_provider);

	listen = (SlonListen *) malloc(sizeof(SlonListen));
	if (listen == NULL)
	{
		perror("rtcfg_storeListen: malloc()");
		slon_retry();
	}
	memset(listen, 0, sizeof(SlonListen));

	listen->li_origin = li_origin;
	DLLIST_ADD_TAIL(node->listen_head, node->listen_tail, listen);

	rtcfg_unlock();
	rtcfg_seq_bump();

	/*
	 * Eventually start communicating with that node
	 */
	rtcfg_startStopNodeThread(node);
}


/* ----------
 * rtcfg_dropListen
 * ----------
 */
void
rtcfg_dropListen(int li_origin, int li_provider)
{
	SlonNode   *node;
	SlonListen *listen;

	rtcfg_lock();

	node = rtcfg_findNode(li_provider);
	if (!node)
	{
		slon_log(SLON_FATAL,
				 "dropListen: unknown node ID %d\n", li_provider);
		slon_retry();
		return;
	}

	/*
	 * Find that listen entry at this provider.
	 */
	for (listen = node->listen_head; listen; listen = listen->next)
	{
		if (listen->li_origin == li_origin)
		{
			slon_log(SLON_CONFIG,
					 "dropListen: li_origin=%d li_receiver=%d "
					 "li_provider=%d\n",
					 li_origin, rtcfg_nodeid, li_provider);

			DLLIST_REMOVE(node->listen_head, node->listen_tail, listen);
			free(listen);

			rtcfg_unlock();
			rtcfg_seq_bump();

			/*
			 * Eventually stop communicating with that node
			 */
			rtcfg_startStopNodeThread(node);
			return;
		}
	}

	rtcfg_unlock();

	/*
	 * Add the new event origin to the provider (this node)
	 */
	slon_log(SLON_DEBUG1,
			 "storeListen: li_origin=%d li_receiver=%d li_provider=%d "
			 "- not listening\n",
			 li_origin, rtcfg_nodeid, li_provider);
}


/* ----------
 * rtcfg_storeSet
 * ----------
 */
void
rtcfg_storeSet(int set_id, int set_origin, char *set_comment)
{
	SlonSet    *set;

	rtcfg_lock();

	/*
	 * Try to update an existing set configuration
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
			int			old_origin = set->set_origin;

			slon_log(SLON_CONFIG,
					 "storeSet: set_id=%d set_origin=%d "
					 "set_comment='%s' - update set\n",
					 set_id, set_origin,
					 (set_comment == NULL) ? "<unchanged>" : set_comment);
			set->set_origin = set_origin;
			if (set_comment != NULL)
			{
				free(set->set_comment);
				set->set_comment = strdup(set_comment);
			}
			rtcfg_unlock();
			rtcfg_seq_bump();
			if (old_origin != set_origin)
				sched_wakeup_node(old_origin);
			sched_wakeup_node(set_origin);
			return;
		}
	}

	/*
	 * Add a new set to the configuration
	 */
	slon_log(SLON_CONFIG,
			 "storeSet: set_id=%d set_origin=%d set_comment='%s'\n",
			 set_id, set_origin, set_comment);
	set = (SlonSet *) malloc(sizeof(SlonSet));
	if (set == NULL)
	{
		perror("rtcfg_storeSet: malloc()");
		slon_retry();
	}
	memset(set, 0, sizeof(SlonSet));

	set->set_id = set_id;
	set->set_origin = set_origin;
	set->set_comment = strdup((set_comment == NULL) ? "<unknown>" : set_comment);

	set->sub_provider = -1;

	DLLIST_ADD_TAIL(rtcfg_set_list_head, rtcfg_set_list_tail, set);
	rtcfg_unlock();
	rtcfg_seq_bump();
	sched_wakeup_node(set_origin);
}


/* ----------
 * rtcfg_dropSet
 * ----------
 */
void
rtcfg_dropSet(int set_id)
{
	SlonSet    *set;

	rtcfg_lock();

	/*
	 * Find the set and remove it from the config
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
			int			old_origin = set->set_origin;

			slon_log(SLON_CONFIG,
					 "dropSet: set_id=%d\n", set_id);
			DLLIST_REMOVE(rtcfg_set_list_head, rtcfg_set_list_tail, set);
			free(set->set_comment);
			free(set);

			rtcfg_unlock();
			rtcfg_seq_bump();
			sched_wakeup_node(old_origin);
			return;
		}
	}

	slon_log(SLON_CONFIG,
			 "dropSet: set_id=%d - set not found\n", set_id);
	rtcfg_unlock();
}

/* ----------
 * rtcfg_moveSet
 * ----------
 */
void
rtcfg_moveSet(int set_id, int old_origin, int new_origin, int sub_provider)
{
	SlonSet    *set;

	rtcfg_lock();

	/*
	 * find the set
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
			slon_log(SLON_CONFIG,
					 "moveSet: set_id=%d old_origin=%d "
					 "new_origin=%d\n",
					 set_id, old_origin, new_origin);

			set->set_origin = new_origin;
			set->sub_provider = sub_provider;
			if (rtcfg_nodeid == old_origin)
			{
				set->sub_active = true;
				set->sub_forward = true;
			}
			if (sub_provider < 0)
			{
				set->sub_active = false;
				set->sub_forward = false;
			}
			rtcfg_unlock();
			rtcfg_seq_bump();
			sched_wakeup_node(old_origin);
			sched_wakeup_node(new_origin);
			return;
		}
	}

	/*
	 * This cannot happen.
	 */
	rtcfg_unlock();
	slon_log(SLON_FATAL, "rtcfg_moveSet(): set %d not found\n", set_id);
	slon_retry();
}


/* ----------
 * rtcfg_storeSubscribe
 * ----------
 */
void
rtcfg_storeSubscribe(int sub_set, int sub_provider, char *sub_forward)
{
	SlonSet    *set;
	int			old_provider = -1;

	rtcfg_lock();

	/*
	 * Find the set and store subscription information
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
			slon_log(SLON_CONFIG,
					 "storeSubscribe: sub_set=%d sub_provider=%d "
					 "sub_forward='%s'\n",
					 sub_set, sub_provider, sub_forward);
			old_provider = set->sub_provider;
			if (set->sub_provider < 0)
				set->sub_active = 0;
			set->sub_provider = sub_provider;
			set->sub_forward = (*sub_forward == 't');
			rtcfg_unlock();
			rtcfg_seq_bump();

			/*
			 * Wakeup the worker threads for the old and new provider
			 */
			if (old_provider >= 0 && old_provider != sub_provider)
				sched_wakeup_node(old_provider);
			if (sub_provider >= 0)
				sched_wakeup_node(sub_provider);
			return;
		}
	}

	slon_log(SLON_FATAL,
			 "storeSubscribe: set %d not found\n", sub_set);
	rtcfg_unlock();
	slon_retry();
}


/* ----------
 * rtcfg_enableSubscription
 * ----------
 */
void
rtcfg_enableSubscription(int sub_set, int sub_provider, char *sub_forward)
{
	SlonSet    *set;
	int			old_provider = -1;

	rtcfg_lock();

	/*
	 * Find the set and enable its subscription
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
			slon_log(SLON_CONFIG,
					 "enableSubscription: sub_set=%d\n",
					 sub_set);
			old_provider = set->sub_provider;
			set->sub_provider = sub_provider;
			set->sub_forward = (*sub_forward == 't');
			if (set->sub_provider >= 0)
				set->sub_active = 1;
			else
				set->sub_active = 0;
			rtcfg_unlock();
			rtcfg_seq_bump();
			if (old_provider > 0 && old_provider != sub_provider)
				sched_wakeup_node(old_provider);
			if (sub_provider > 0)
				sched_wakeup_node(sub_provider);
			return;
		}
	}

	slon_log(SLON_FATAL,
			 "enableSubscription: set %d not found\n", sub_set);
	rtcfg_unlock();
	slon_retry();
}


/* ----------
 * rtcfg_unsubscribeSet
 * ----------
 */
void
rtcfg_unsubscribeSet(int sub_set)
{
	SlonSet    *set;
	int			old_provider = -1;

	rtcfg_lock();

	/*
	 * Find the set and store subscription information
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
			slon_log(SLON_CONFIG,
					 "unsubscribeSet: sub_set=%d\n",
					 sub_set);
			old_provider = set->sub_provider;
			set->sub_provider = -1;
			set->sub_active = false;
			set->sub_forward = false;
			rtcfg_unlock();
			rtcfg_seq_bump();

			/*
			 * Wakeup the worker threads for the old and new provider
			 */
			if (old_provider >= 0)
				sched_wakeup_node(old_provider);
			return;
		}
	}

	slon_log(SLON_FATAL,
			 "unsubscribeSet: set %d not found\n", sub_set);
	rtcfg_unlock();
	slon_retry();
}


/* ----------
 * rtcfg_startStopNodeThread
 * ----------
 */
static void
rtcfg_startStopNodeThread(SlonNode * node)
{
	int			need_listen = false;
	int			need_wakeup = false;

	rtcfg_lock();

	if (sched_get_status() == SCHED_STATUS_OK && node->no_active)
	{
		/*
		 * Make sure the node worker exists
		 */
		switch (node->worker_status)
		{
			case SLON_TSTAT_NONE:
				if (pthread_create(&(node->worker_thread), NULL,
								 remoteWorkerThread_main, (void *) node) < 0)
				{
					slon_log(SLON_FATAL,
							 "startStopNodeThread: cannot create "
							 "remoteWorkerThread - %s\n",
							 strerror(errno));
					rtcfg_unlock();
					slon_retry();
				}
				node->worker_status = SLON_TSTAT_RUNNING;
				break;

			case SLON_TSTAT_RUNNING:
				break;

			default:
				printf("TODO: ********** rtcfg_startStopNodeThread: restart node worker\n");
		}
	}
	else
	{
		/*
		 * Make sure there is no node worker
		 */
		switch (node->worker_status)
		{
			case SLON_TSTAT_NONE:
				break;
			default:
				break;
		}
	}

	/*
	 * Determine if we need a listener thread
	 */
	if (node->listen_head != NULL)
		need_listen = true;
	if (!(node->no_active))
		need_listen = false;

	/*
	 * Start or stop the remoteListenThread
	 */
	if (need_listen)
	{
		/*
		 * Node specific listen thread is required
		 */
		switch (node->listen_status)
		{
			case SLON_TSTAT_NONE:
				node->listen_status = SLON_TSTAT_RUNNING;
				if (pthread_create(&(node->listen_thread), NULL,
								 remoteListenThread_main, (void *) node) < 0)
				{
					slon_log(SLON_FATAL,
							 "startStopNodeThread: cannot create "
							 "remoteListenThread - %s\n",
							 strerror(errno));
					rtcfg_unlock();
					slon_retry();
				}
				break;

			case SLON_TSTAT_RUNNING:
				need_wakeup = true;
				break;

			case SLON_TSTAT_SHUTDOWN:
				node->listen_status = SLON_TSTAT_RESTART;
				need_wakeup = true;
				break;

			case SLON_TSTAT_RESTART:
				need_wakeup = true;
				break;

			case SLON_TSTAT_DONE:
				pthread_join(node->listen_thread, NULL);
				node->listen_status = SLON_TSTAT_NONE;
				break;
		}
	}
	else
	{
		/*
		 * Node specific listen thread not required
		 */
		switch (node->listen_status)
		{
			case SLON_TSTAT_NONE:
				break;

			case SLON_TSTAT_RUNNING:
				node->listen_status = SLON_TSTAT_SHUTDOWN;
				need_wakeup = true;
				break;

			case SLON_TSTAT_SHUTDOWN:
				need_wakeup = true;
				break;

			case SLON_TSTAT_RESTART:
				node->listen_status = SLON_TSTAT_SHUTDOWN;
				need_wakeup = true;
				break;

			case SLON_TSTAT_DONE:
				pthread_join(node->listen_thread, NULL);
				node->listen_status = SLON_TSTAT_NONE;
				break;
		}
	}

	rtcfg_unlock();

	/*
	 * If need be, wakeup all node threads to reread the configuration.
	 */
	if (need_wakeup)
		sched_wakeup_node(node->no_id);
}


/* ----------
 * rtcfg_needActivate
 * ----------
 */
void
rtcfg_needActivate(int no_id)
{
	struct to_activate *anode;

	anode = (struct to_activate *) malloc(sizeof(struct to_activate));
	if (anode == NULL)
	{
		perror("rtcfg_needActivate: malloc()");
		slon_retry();
	}
	anode->no_id = no_id;
	DLLIST_ADD_TAIL(to_activate_head, to_activate_tail, anode);
}


/* ----------
 * rtcfg_doActivate
 * ----------
 */
void
rtcfg_doActivate(void)
{
	while (to_activate_head != NULL)
	{
		struct to_activate *anode = to_activate_head;

		rtcfg_enableNode(anode->no_id);
		DLLIST_REMOVE(to_activate_head, to_activate_tail, anode);
		free(anode);
	}
}


/* ----------
 * rtcfg_joinAllRemoteThreads
 * ----------
 */
void
rtcfg_joinAllRemoteThreads(void)
{
	SlonNode   *node;

	rtcfg_lock();

	for (node = rtcfg_node_list_head; node; node = node->next)
	{
		switch (node->listen_status)
		{
			case SLON_TSTAT_NONE:
				break;

			case SLON_TSTAT_RUNNING:
			case SLON_TSTAT_SHUTDOWN:
			case SLON_TSTAT_RESTART:
				node->listen_status = SLON_TSTAT_SHUTDOWN;
				/* fall through */

			case SLON_TSTAT_DONE:
				rtcfg_unlock();
				sched_wakeup_node(node->no_id);
				pthread_join(node->listen_thread, NULL);
				rtcfg_lock();
				node->listen_status = SLON_TSTAT_NONE;
				break;
		}

		switch (node->worker_status)
		{
			case SLON_TSTAT_NONE:
				break;

			case SLON_TSTAT_RUNNING:
			case SLON_TSTAT_SHUTDOWN:
			case SLON_TSTAT_RESTART:
				node->worker_status = SLON_TSTAT_SHUTDOWN;
				/* fall through */

			case SLON_TSTAT_DONE:
				rtcfg_unlock();
				remoteWorker_wakeup(node->no_id);
				pthread_join(node->worker_thread, NULL);
				rtcfg_lock();
				node->worker_status = SLON_TSTAT_NONE;
				break;
		}

	}

	rtcfg_unlock();
}


/* ----------
 * rtcfg_seq_bump
 * ----------
 */
void
rtcfg_seq_bump(void)
{
	pthread_mutex_lock(&cfgseq_lock);
	cfgseq++;
	pthread_mutex_unlock(&cfgseq_lock);
}


/* ----------
 * rtcfg_seq_get
 * ----------
 */
int64
rtcfg_seq_get(void)
{
	int64		retval;

	pthread_mutex_lock(&cfgseq_lock);
	retval = cfgseq;
	pthread_mutex_unlock(&cfgseq_lock);

	return retval;
}

/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
