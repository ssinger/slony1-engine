/*-------------------------------------------------------------------------
 * runtime_config.c
 *
 *	Functions maintaining the in-memory configuration information
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: runtime_config.c,v 1.7 2004-02-24 21:03:34 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "libpq-fe.h"
#include "c.h"

#include "slon.h"


/* ----------
 * Global data
 * ----------
 */
pid_t					slon_pid;
char				   *rtcfg_cluster_name = NULL;
char				   *rtcfg_namespace = NULL;
char				   *rtcfg_conninfo = NULL;
int						rtcfg_nodeid = -1;
int						rtcfg_nodeactive = 0;
char				   *rtcfg_nodecomment = NULL;
char					rtcfg_lastevent[64];

SlonSet				   *rtcfg_set_list_head = NULL;
SlonSet				   *rtcfg_set_list_tail = NULL;
SlonNode			   *rtcfg_node_list_head = NULL;
SlonNode			   *rtcfg_node_list_tail = NULL;


/* ----------
 * Local data
 * ----------
 */
static pthread_mutex_t	config_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	cfgseq_lock = PTHREAD_MUTEX_INITIALIZER;
static int64			cfgseq = 0;

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
static void			rtcfg_startStopNodeThread(SlonNode *node);


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
	SlonNode	   *node;

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

	node = (SlonNode *)malloc(sizeof(SlonNode));
	if (node == NULL)
	{
		perror("rtcfg_storeNode: malloc()");
		slon_abort();
	}
	memset (node, 0, sizeof(SlonNode));

	node->no_id      = no_id;
	node->no_active  = false;
	node->no_comment = strdup(no_comment);
	pthread_mutex_init(&(node->message_lock), NULL);
	pthread_cond_init(&(node->message_cond), NULL);

	DLLIST_ADD_TAIL(rtcfg_node_list_head, rtcfg_node_list_tail, node);

	rtcfg_unlock();
	rtcfg_seq_bump();
}


/* ----------
 * rtcfg_setNodeLastEvent()
 *
 *	Set the last_event field in the node runtime structure.
 *
 *	Returns:	0 if the event_seq is <= the known value
 *				-1 if the node is not known
 *				event_seq otherwise
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

	slon_log(SLON_DEBUG1,
			"setNodeLastEvent: no_id=%d event_seq=%lld\n",
			no_id, retval);

	return retval;
}


/* ----------
 * rtcfg_getNodeLastEvent
 *
 *	Read the nodes last_event field
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
 * rtcfg_enableNode
 * ----------
 */
void
rtcfg_enableNode(int no_id)
{
	SlonNode	   *node;

	rtcfg_lock();

	node = rtcfg_findNode(no_id);
	if (!node)
	{
		rtcfg_unlock();

		slon_log(SLON_FATAL,
				"enableNode: unknown node ID %d\n", no_id);
		slon_abort();
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
 * slon_dropNode
 * ----------
 */
void
rtcfg_dropNode(int no_id)
{
	printf("TODO: ********** rtcfg_dropNode\n");
}


/* ----------
 * rtcfg_findNode
 * ----------
 */
SlonNode *
rtcfg_findNode(int no_id)
{
	SlonNode	   *node;

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
	SlonNode	   *node;

	rtcfg_lock();

	node = rtcfg_findNode(pa_server);
	if (!node)
	{
		rtcfg_unlock();

		slon_log(SLON_FATAL, 
				"storePath: unknown node ID %d\n", pa_server);
		slon_abort();
		return;
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
	node->pa_conninfo  = strdup(pa_conninfo);
	node->pa_connretry = pa_connretry;

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
rtcfg_storeListen(int li_origin, int li_provider)
{
	SlonNode	   *node;
	SlonListen	   *listen;

	rtcfg_lock();

	node = rtcfg_findNode(li_provider);
	if (!node)
	{
		slon_log(SLON_FATAL,
				"storeListen: unknown node ID %d\n", li_provider);
		slon_abort();
		return;
	}

	/*
	 * Check if we already listen for events from that origin
	 * at this provider.
	 */
	for (listen = node->listen_head; listen; listen = listen->next)
	{
		if (listen->li_origin == li_origin)
		{
			slon_log(SLON_DEBUG1,
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

	listen = (SlonListen *)malloc(sizeof(SlonListen));
	if (listen == NULL)
	{
		perror("rtcfg_storeListen: malloc()");
		slon_abort();
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


void
rtcfg_storeSet(int set_id, int set_origin, char *set_comment)
{
	SlonSet	   *set;

	rtcfg_lock();

	/*
	 * Try to update an existing set configuration
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
			slon_log(SLON_CONFIG,
					"storeSet: set_id=%d set_origin=%d "
					"set_comment='%s' - update set\n",
					set_id, set_origin, set_comment);
			free(set->set_comment);
			set->set_origin = set_origin;
			set->set_comment = strdup(set_comment);
			rtcfg_unlock();
			rtcfg_seq_bump();
			return;
		}
	}

	/*
	 * Add a new set to the configuration
	 */
	slon_log(SLON_CONFIG,
			"storeSet: set_id=%d set_origin=%d set_comment='%s'\n",
			set_id, set_origin, set_comment);
	set = (SlonSet *)malloc(sizeof(SlonSet));
	if (set == NULL)
	{
		perror("rtcfg_storeSet: malloc()");
		slon_abort();
	}
	memset(set, 0, sizeof(SlonSet));

	set->set_id = set_id;
	set->set_origin = set_origin;
	set->set_comment = strdup(set_comment);

	set->sub_provider = -1;

	DLLIST_ADD_TAIL(rtcfg_set_list_head, rtcfg_set_list_tail, set);
	rtcfg_unlock();
	rtcfg_seq_bump();
}


void
rtcfg_storeSubscribe(int sub_set, int sub_provider, int sub_forward)
{
	SlonSet	   *set;

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
					"sub_forward=%d\n",
					sub_set, sub_provider, sub_forward);
			if (set->sub_provider < 0)
				set->sub_active = 0;
			set->sub_provider = sub_provider;
			set->sub_forward  = sub_forward;
			rtcfg_unlock();
			rtcfg_seq_bump();
			return;
		}
	}

	slon_log(SLON_FATAL,
			"storeSubscribe: set %d not found\n", sub_set);
	rtcfg_unlock();
	slon_abort();
}


void
rtcfg_enableSubscription(int sub_set)
{
	SlonSet	   *set;

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
			if (set->sub_provider >= 0)
				set->sub_active = 1;
			rtcfg_unlock();
			rtcfg_seq_bump();
			return;
		}
	}

	slon_log(SLON_FATAL,
			"enableSubscription: set %d not found\n", sub_set);
	rtcfg_unlock();
	slon_abort();
}


/* ----------
 * rtcfg_startStopNodeThread
 * ----------
 */
static void
rtcfg_startStopNodeThread(SlonNode *node)
{
	int		need_listen = false;
	int		need_wakeup = false;

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
						remoteWorkerThread_main, (void *)node) < 0)
				{
					slon_log(SLON_FATAL,
							"startStopNodeThread: cannot create "
							"remoteWorkerThread - %s\n",
							strerror(errno));
					rtcfg_unlock();
					slon_abort();
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
						remoteListenThread_main, (void *)node) < 0)
				{
					slon_log(SLON_FATAL,
							"startStopNodeThread: cannot create "
							"remoteListenThread - %s\n",
							strerror(errno));
					rtcfg_unlock();
					slon_abort();
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


void
rtcfg_needActivate(int no_id)
{
	struct to_activate	   *anode;

	anode = (struct to_activate *)malloc(sizeof(struct to_activate));
	if (anode == NULL)
	{
		perror("rtcfg_needActivate: malloc()");
		slon_abort();
	}
	anode->no_id = no_id;
	DLLIST_ADD_TAIL(to_activate_head, to_activate_tail, anode);
}


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


void
rtcfg_seq_bump(void)
{
	pthread_mutex_lock(&cfgseq_lock);
	cfgseq++;
	pthread_mutex_unlock(&cfgseq_lock);
}


int64
rtcfg_seq_get(void)
{
	int64 retval;

	pthread_mutex_lock(&cfgseq_lock);
	retval = cfgseq;
	pthread_mutex_unlock(&cfgseq_lock);

	return retval;
}



