/*-------------------------------------------------------------------------
 * runtime_config.c
 *
 *	Functions maintaining the in-memory configuration information
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: runtime_config.c,v 1.2 2004-02-20 15:13:28 wieck Exp $
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
char				   *rtcfg_lastevent = NULL;

SlonSet				   *rtcfg_set_list_head = NULL;
SlonSet				   *rtcfg_set_list_tail = NULL;


/* ----------
 * Local data
 * ----------
 */
static pthread_mutex_t	config_lock = PTHREAD_MUTEX_INITIALIZER;
static SlonNode		   *node_list_head = NULL;
static SlonNode		   *node_list_tail = NULL;

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
static SlonNode	   *rtcfg_findNode(int no_id);
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
printf("rtcfg_storeNode: no_id=%d: UPD no_comment='%s'\n", no_id, no_comment);
		free(node->no_comment);
		node->no_comment = strdup(no_comment);

		rtcfg_unlock();
		return;
	}

	/*
	 * Add the new node to our in-memory configuration.
	 */
printf("rtcfg_storeNode: no_id=%d: NEW no_comment='%s'\n", no_id, no_comment);
	node = (SlonNode *)malloc(sizeof(SlonNode));
	memset (node, 0, sizeof(SlonNode));

	node->no_id      = no_id;
	node->no_active  = false;
	node->no_comment = strdup(no_comment);
#if 0
	pthread_mutex_init(&(node->node_lock), NULL);
#endif

	DLLIST_ADD_TAIL(node_list_head, node_list_tail, node);

	rtcfg_unlock();
}


void
rtcfg_setNodeLastEvent(int no_id, int64 event_seq)
{
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

		fprintf(stderr, "rtcfg_enableNode: unknown node ID %d\n", no_id);
		slon_abort();
		return;
	}

	/*
	 * Activate the node
	 */
printf("rtcfg_enableNode: no_id=%d\n", no_id);
	node->no_active = true;

	rtcfg_unlock();

	rtcfg_startStopNodeThread(node);
}


/* ----------
 * slon_dropNode
 * ----------
 */
void
rtcfg_dropNode(int no_id)
{
	printf("TODO: rtcfg_dropNode\n");
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

		fprintf(stderr, "rtcfg_storePath: unknown node ID %d\n", pa_server);
		slon_abort();
		return;
	}

	/*
	 * Store the (new) conninfo to the node
	 */
printf("rtcfg_storePath: pa_server=%d pa_conninfo=\"%s\" pa_connretry=%d\n", 
pa_server, pa_conninfo, pa_connretry);
	if (node->pa_conninfo != NULL)
		free(node->pa_conninfo);
	node->pa_conninfo  = strdup(pa_conninfo);
	node->pa_connretry = pa_connretry;

	rtcfg_unlock();

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
		fprintf(stderr, "rtcfg_storeListen: unknown node ID %d\n", li_provider);
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
printf("rtcfg_storeListen: already listening for li_origin=%d li_provider=%d\n",
li_origin, li_provider);
			rtcfg_unlock();
			return;
		}
	}

	/*
	 * Add the new event origin to the provider (this node)
	 */
printf("rtcfg_storeListen: add li_origin=%d to li_provider=%d\n",
li_origin, li_provider);
	listen = (SlonListen *)malloc(sizeof(SlonListen));
	memset(listen, 0, sizeof(SlonListen));

	listen->li_origin = li_origin;
	DLLIST_ADD_TAIL(node->listen_head, node->listen_tail, listen);

	rtcfg_unlock();

	/*
	 * Eventually start communicating with that node
	 */
	rtcfg_startStopNodeThread(node);
}


void
rtcfg_storeSet(int set_id, int set_origin, char *set_comment)
{
printf("rtcfg_storeSet: new set_id=%d set_origin=%d set_comment='%s'\n",
set_id, set_origin, set_comment);
#if 0
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Try to update an existing set configuration
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
printf("rtcfg_storeSet: update set_id=%d set_origin=%d set_comment='%s'\n",
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
printf("rtcfg_storeSet: new set_id=%d set_origin=%d set_comment='%s'\n",
set_id, set_origin, set_comment);
	set = (SlonSet *)malloc(sizeof(SlonSet));
	memset(set, 0, sizeof(SlonSet));

	set->set_id = set_id;
	set->set_origin = set_origin;
	set->set_comment = strdup(set_comment);

	set->sub_provider = -1;

	DLLIST_ADD_TAIL(rtcfg_set_list_head, rtcfg_set_list_tail, set);
	pthread_mutex_unlock(&config_lock);
#endif
}


void
rtcfg_storeSubscribe(int sub_set, int sub_provider, int sub_forward)
{
printf("rtcfg_storeSubscribe: sub_set=%d sub_provider=%d sub_forward=%d\n",
sub_set, sub_provider, sub_forward);
#if 0
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Find the set and store subscription information
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
printf("rtcfg_storeSubscribe: sub_set=%d sub_provider=%d sub_forward=%d\n",
sub_set, sub_provider, sub_forward);
			if (set->sub_provider < 0)
				set->sub_active = 0;
			set->sub_provider = sub_provider;
			set->sub_forward  = sub_forward;
			pthread_mutex_unlock(&config_lock);
			return;
		}
	}

	fprintf(stderr, "rtcfg_storeSubscribe: set %d not found\n", sub_set);
	pthread_mutex_unlock(&config_lock);
	slon_abort();
#endif
}


void
rtcfg_enableSubscription(int sub_set)
{
printf("rtcfg_enableSubscription: sub_set=%d\n", sub_set);
#if 0
	SlonSet	   *set;

	pthread_mutex_lock(&config_lock);

	/*
	 * Find the set and enable its subscription
	 */
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == sub_set)
		{
printf("rtcfg_enableSubscription: sub_set=%d\n", sub_set);
			if (set->sub_provider >= 0)
				set->sub_active = 1;
			pthread_mutex_unlock(&config_lock);
			return;
		}
	}

	fprintf(stderr, "rtcfg_enableSubscription: set %d not found\n", sub_set);
	pthread_mutex_unlock(&config_lock);
	slon_abort();
#endif
}


/* ----------
 * rtcfg_findNode
 * ----------
 */
static SlonNode *
rtcfg_findNode(int no_id)
{
	SlonNode	   *node;

	for (node = node_list_head; node; node = node->next)
	{
		if (node->no_id == no_id)
			return node;
	}

	return NULL;
}


/* ----------
 * rtcfg_startStopNodeThread
 * ----------
 */
static void
rtcfg_startStopNodeThread(SlonNode *node)
{
	rtcfg_lock();

	/*
	 * Check if we have to create a new remote_listen thread
	 */
	if (node->listen_status == SLON_TSTAT_NONE && node->listen_head != NULL)
	{
printf("TODO: rtcfg_startStopNodeThread() need to start \"remote_listen\" for no_id=%d\n", node->no_id);
	}

	rtcfg_unlock();

#if 0
	if (node->have_thread)
	{
printf("TODO: rtcfg_startStopNodeThread() have_thread branch\n");
	}
	else
	{
		if (!rtcfg_nodeactive)
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
			perror("rtcfg_startStopNodeThread: pthread_create()");
			return;
		}
		node->have_thread = true;
printf("rtcfg_startStopNodeThread() thread for node %d created\n", node->no_id);
	}
#endif
}


void
rtcfg_needActivate(int no_id)
{
	struct to_activate	   *anode;

	anode = (struct to_activate *)malloc(sizeof(struct to_activate));
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
#if 0
	SlonNode   *node;

	pthread_mutex_lock(&config_lock);
	for (node = node_list_head; node; node = node->next)
	{
		pthread_join(node->event_thread, NULL);
		pthread_mutex_lock(&(node->node_lock));
		node->have_thread = false;
		pthread_mutex_unlock(&(node->node_lock));
	}
	pthread_mutex_unlock(&config_lock);
#endif
}
