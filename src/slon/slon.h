/*-------------------------------------------------------------------------
 * slon.h
 *
 *	Global definitions for the main replication engine.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: slon.h,v 1.7 2004-01-09 21:33:14 wieck Exp $
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
	pthread_mutex_t		node_lock;		/* mutex for node */

	int					have_thread;	/* flag if event thread exists */
	pthread_t			event_thread;	/* thread listening for events */
	SlonConn		   *event_conn;		/* connection event thread uses */
	PGconn			   *local_dbconn;	/* connection for forwarding data */

	char			   *pa_conninfo;	/* conninfo to connect to it */
	int					pa_connretry;	/* connect retry interval in seconds */

	SlonListen		   *li_list_head;	/* list of event origins we receive */
	SlonListen		   *li_list_tail;	/* from this provider */

	pthread_mutex_t		data_dblock;	/* lock and DB connection to get */
	PGconn			   *data_dbconn;	/* data from the remote node */
	
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
 * Globals in slon.c
 * ----------
 */
extern pid_t	slon_pid;
extern char	   *local_cluster_name;
extern char	   *local_namespace;
extern char	   *local_conninfo;
extern int		local_nodeid;
extern int		local_nodeactive;
extern char	   *local_nodecomment;
extern char	   *local_lastevent;

extern SlonSet *set_list_head;
extern SlonSet *set_list_tail;


/* ----------
 * Functions in slon.c
 * ----------
 */
extern int		slon_getLocalNodeId(PGconn *conn);

extern void		slon_storeNode(int no_id, char *no_comment);
extern void		slon_enableNode(int no_id);		
extern void		slon_dropNode(int no_id);

extern void		slon_storePath(int pa_server, char *pa_conninfo,
							int pa_connretry);

extern void		slon_storeListen(int li_origin, int li_provider);

extern void		slon_storeSet(int set_id, int set_origin, char *set_comment);
extern void		slon_storeSubscribe(int sub_set, int sub_provider,
							int sub_forward);
extern void		slon_enableSubscription(int sub_set);

extern SlonConn *slon_connectdb(char *conninfo, char *symname);
extern void		slon_disconnectdb(SlonConn *conn);
extern SlonConn *slon_make_dummyconn(char *symname);
extern void		slon_free_dummyconn(SlonConn *conn);

#define slon_abort() {kill(slon_pid, SIGTERM);}
extern void		slon_exit(int code);
extern void		slon_quote(char *buf, char *value, char **endp);

typedef struct {
	size_t	size;
	char   *buf;
} slon_querybuf;
extern int		slon_mkquery(slon_querybuf *buf, char *fmt, ...);


/* ----------
 * Functions in local_node.c
 * ----------
 */
extern void	   *slon_localEventThread(void *dummy);
extern void	   *slon_localCleanupThread(void *dummy);
extern void	   *slon_localSyncThread(void *dummy);


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


#endif /*  SLON_H_INCLUDED */

