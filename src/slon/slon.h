/*-------------------------------------------------------------------------
 * slon.h
 *
 *	Global definitions for the main replication engine.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: slon.h,v 1.9 2004-02-20 15:13:28 wieck Exp $
 *-------------------------------------------------------------------------
 */

#ifndef SLON_H_INCLUDED
#define SLON_H_INCLUDED


#ifndef false
#define   false 0
#endif
#ifndef true
#define   true (~false)
#endif


#define	SLON_TSTAT_NONE		0
#define	SLON_TSTAT_RUNNING	1
#define	SLON_TSTAT_SHUTDOWN	2



/* ----------
 * In memory structures for cluster configuration
 * ----------
 */
typedef struct SlonNode_s	SlonNode;
typedef struct SlonListen_s	SlonListen;
typedef struct SlonSet_s	SlonSet;
typedef struct SlonConn_s	SlonConn;

/* ----
 * SlonNode
 * ----
 */
struct SlonNode_s {
	int					no_id;			/* node ID */
	int					no_active;		/* it's active state */
	char			   *no_comment;		/* comment field */
#if 0
	pthread_mutex_t		node_lock;		/* mutex for node */
#endif

	char			   *pa_conninfo;	/* path to the node */
	int					pa_connretry;	/* connection retry interval */

	int64				last_event;		/* last event we have received */

	int					listen_status;	/* status of the listen thread */
	pthread_t			listen_thread;	/* thread id of listen thread */
	SlonListen		   *listen_head;	/* list of origins we listen for */
	SlonListen		   *listen_tail;

	SlonNode		   *prev;
	SlonNode		   *next;
};

/* ----
 * SlonListen
 * ----
 */
struct SlonListen_s {
	int					li_origin;		/* origin of events */

	SlonListen		   *prev;
	SlonListen		   *next;
};

/* ----
 * SlonSet
 * ----
 */
struct SlonSet_s {
	int					set_id;			/* set ID */
	int					set_origin;		/* set origin */
	char			   *set_comment;	/* set comment */

	int					sub_provider;	/* from where this node receives */
										/* data (if subscribed) */
	int					sub_forward;	/* if we need to forward data */
	int					sub_active;		/* if the subscription is active */

	SlonSet			   *prev;
	SlonSet			   *next;
};

/* ----
 * SlonConn
 * ----
 */
struct SlonConn_s {
	char			   *symname;		/* Symbolic name of connection */
	struct SlonNode_s  *node;			/* remote node this belongs to */
	PGconn			   *dbconn;			/* database connection */
	pthread_mutex_t		conn_lock;		/* mutex for conn */
	pthread_cond_t		conn_cond;		/* condition variable for conn */

	int					condition;		/* what are we waiting for? */
	struct timeval		timeout;		/* timeofday for timeout */

	SlonConn		   *prev;
	SlonConn		   *next;
};

/* ----
 * SlonDString
 * ----
 */
#define		SLON_DSTRING_SIZE_INIT	64
#define		SLON_DSTRING_SIZE_INC	64

typedef struct
{
	size_t		n_alloc;
	size_t		n_used;
	char	   *data;
} SlonDString;

#define		dstring_init(__ds) \
{ \
	(__ds)->n_alloc = SLON_DSTRING_SIZE_INIT; \
	(__ds)->n_used = 0; \
	(__ds)->data = malloc(SLON_DSTRING_SIZE_INIT); \
}
#define		dstring_reset(__ds) \
{ \
	(__ds)->n_used = 0; \
	(__ds)->data[0] = '\0'; \
}
#define		dstring_free(__ds) \
{ \
	free((__ds)->data); \
	(__ds)->n_used = 0; \
	(__ds)->data = NULL; \
}
#define		dstring_nappend(__ds,__s,__n) \
{ \
	if ((__ds)->n_used + (__n) >= (__ds)->n_alloc)  \
	{ \
		while ((__ds)->n_used + (__n) >= (__ds)->n_alloc) \
			(__ds)->n_alloc += SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
	} \
	memcpy(&((__ds)->data[(__ds)->n_used]), (__s), (__n)); \
	(__ds)->n_used += (__n); \
}
#define		dstring_append(___ds,___s) \
{ \
	register int ___n = strlen((___s)); \
	dstring_nappend((___ds),(___s),___n); \
}
#define		dstring_addchar(__ds,__c) \
{ \
	if ((__ds)->n_used + 1 >= (__ds)->n_alloc)  \
	{ \
		(__ds)->n_alloc += SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
	} \
	(__ds)->data[(__ds)->n_used++] = (__c); \
}
#define		dstring_terminate(__ds) \
{ \
	(__ds)->data[(__ds)->n_used] = '\0'; \
}
#define		dstring_data(__ds)	((__ds)->data)


/* ----------
 * Macros to add and remove entries from double linked lists
 * ----------
 */
#define	DLLIST_ADD_TAIL(_pf,_pl,_obj) \
	if ((_pl) == NULL) { \
		(_obj)->prev = (_obj)->next = NULL; \
		(_pf) = (_pl) = (_obj); \
	} else { \
		(_obj)->prev = (_pl); \
		(_obj)->next = NULL; \
		(_pl)->next = (_obj); \
		(_pl) = (_obj); \
	}

#define DLLIST_ADD_HEAD(_pf,_pl,_obj) \
	if ((_pf) == NULL) { \
		(_obj)->prev = (_obj)->next = NULL; \
		(_pf) = (_pl) = (_obj); \
	} else { \
		(_obj)->prev = NULL; \
		(_obj)->next = (_pf); \
		(_pf)->prev = (_obj); \
		(_pf) = (_obj); \
	}

#define DLLIST_REMOVE(_pf,_pl,_obj) \
	if ((_obj)->prev == NULL) { \
		(_pf) = (_obj)->next; \
	} else { \
		(_obj)->prev->next = (_obj)->next; \
	} \
	if ((_obj)->next == NULL) { \
		(_pl) = (_obj)->prev; \
	} else { \
		(_obj)->next->prev = (_obj)->prev; \
	} \
	(_obj)->prev = (_obj)->next = NULL


/* ----------
 * Macro to compute the difference between two timeval structs
 * as a double precision floating point.
 * t1 = start time
 * t2 = end time
 * ----------
 */
#define TIMEVAL_DIFF(_t1,_t2) \
	(((_t1)->tv_usec <= (_t2)->tv_usec) ? \
		(double)((_t2)->tv_sec - (_t1)->tv_sec) + (double)((_t2)->tv_usec - (_t1)->tv_usec) / 1000000.0 : \
		(double)((_t2)->tv_sec - (_t1)->tv_sec - 1) + (double)((_t2)->tv_usec + 1000000 - (_t1)->tv_usec) / 1000000.0)


/* ----------
 * Scheduler runmodes
 * ----------
 */
#define SCHED_STATUS_OK			0
#define SCHED_STATUS_SHUTDOWN	1
#define SCHED_STATUS_DONE		2
#define SCHED_STATUS_ERROR		3

/* ----------
 * Scheduler wait conditions
 * ----------
 */
#define SCHED_WAIT_SOCK_READ	1
#define SCHED_WAIT_SOCK_WRITE	2
#define SCHED_WAIT_TIMEOUT		4


/* ----------
 * Globals in runtime_config.c
 * ----------
 */
extern pid_t	slon_pid;
extern char	   *rtcfg_cluster_name;
extern char	   *rtcfg_namespace;
extern char	   *rtcfg_conninfo;
extern int		rtcfg_nodeid;
extern int		rtcfg_nodeactive;
extern char	   *rtcfg_nodecomment;
extern char	   *rtcfg_lastevent;

extern SlonSet *rtcfg_set_list_head;
extern SlonSet *rtcfg_set_list_tail;


/* ----------
 * Functions in slon.c
 * ----------
 */
#define slon_abort() {kill(slon_pid, SIGTERM);}
extern void		slon_exit(int code);


/* ----------
 * Functions in runtime_config.c
 * ----------
 */
extern void		rtcfg_lock(void);
extern void		rtcfg_unlock(void);

extern void		rtcfg_storeNode(int no_id, char *no_comment);
extern void		rtcfg_enableNode(int no_id);		
extern void		rtcfg_dropNode(int no_id);
extern void		rtcfg_setNodeLastEvent(int no_id, int64 last_event);

extern void		rtcfg_storePath(int pa_server, char *pa_conninfo,
							int pa_connretry);

extern void		rtcfg_storeListen(int li_origin, int li_provider);

extern void		rtcfg_storeSet(int set_id, int set_origin, char *set_comment);

extern void		rtcfg_storeSubscribe(int sub_set, int sub_provider,
							int sub_forward);
extern void		rtcfg_enableSubscription(int sub_set);

extern void		rtcfg_needActivate(int no_id);
extern void		rtcfg_doActivate(void);
extern void		rtcfg_joinAllRemoteThreads(void);


/* ----------
 * Functions in local_node.c
 * ----------
 */
extern void	   *slon_localEventThread(void *dummy);


/* ----------
 * Functions in cleanup_thread.c
 * ----------
 */
extern void	   *cleanupThread_main(void *dummy);


/* ----------
 * Functions in sync_thread.c
 * ----------
 */
extern void	   *syncThread_main(void *dummy);


/* ----------
 * Functions in remote_node.c
 * ----------
 */
extern void	   *slon_remoteEventThread(void *cdata);


/* ----------
 * Functions in scheduler.c
 * ----------
 */
extern int		sched_start_mainloop(void);
extern int		sched_wait_mainloop(void);
extern int		sched_wait_conn(SlonConn *conn, int condition);
extern int		sched_wait_time(SlonConn *conn, int condition, int msec);
extern int		sched_get_status(void);


/* ----------
 * Functions in dbutils.c
 * ----------
 */
extern SlonConn *slon_connectdb(char *conninfo, char *symname);
extern void		slon_disconnectdb(SlonConn *conn);
extern SlonConn *slon_make_dummyconn(char *symname);
extern void		slon_free_dummyconn(SlonConn *conn);

extern int		db_getLocalNodeId(PGconn *conn);

extern int		slon_mkquery(SlonDString *ds, char *fmt, ...);


/* ----------
 * Functions in misc.c
 * ----------
 */


#endif /*  SLON_H_INCLUDED */

