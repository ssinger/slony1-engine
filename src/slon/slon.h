/*-------------------------------------------------------------------------
 * slon.h
 *
 *	Global definitions for the main replication engine.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */

#ifndef SLON_H_INCLUDED
#define SLON_H_INCLUDED
#ifdef MSVC
#include "config_msvc.h"
#else
#include "config.h"
#endif
#include "types.h"
#include "libpq-fe.h"
#include "misc.h"
#include "conf-file.h"
#include "confoptions.h"
#include <pg_config.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#ifndef INT64_FORMAT
#define INT64_FORMAT "%" INT64_MODIFIER "d"
#endif

#define SLON_MEMDEBUG	1
#ifndef false
#define   false 0
#endif
#ifndef true
#define   true 1
#endif

#undef	SLON_CHECK_CMDTUPLES

#ifdef	SLON_CHECK_CMDTUPLES
#define SLON_COMMANDS_PER_LINE		1
#define SLON_DATA_FETCH_SIZE		100
#define SLON_WORKLINES_PER_HELPER	(SLON_DATA_FETCH_SIZE * 4)
#else
#define SLON_COMMANDS_PER_LINE		10
#define SLON_DATA_FETCH_SIZE		50
#define SLON_WORKLINES_PER_HELPER	(SLON_DATA_FETCH_SIZE * 5)
#endif

#define SLON_MAX_PATH 1024
/* Maximum path length - set to 1024, consistent with PostgreSQL */
/* See: http://archives.postgresql.org/pgsql-hackers/1999-10/msg00754.php */
/* Also  view src/include/pg_config.h.win32 src/include/pg_config_manual.h */

 /* FIXME: must determine and use OS specific max path length */
 /* cbb: Not forcibly necessary; note that MAXPGPATH is 1024 */

 /* cleanup calls */
#define SLON_VACUUM_FREQUENCY		3	/* vacuum every 3rd cleanup */


typedef enum
{
	SLON_TSTAT_NONE,
	SLON_TSTAT_RUNNING,
	SLON_TSTAT_SHUTDOWN,
	SLON_TSTAT_RESTART,
	SLON_TSTAT_DONE
}	SlonThreadStatus;


extern bool logpid;

/* ----------
 * In memory structures for cluster configuration
 * ----------
 */
typedef struct SlonNode_s SlonNode;
typedef struct SlonListen_s SlonListen;
typedef struct SlonSet_s SlonSet;
typedef struct SlonConn_s SlonConn;
typedef struct SlonState_s SlonState;

typedef struct SlonWorkMsg_s SlonWorkMsg;

/* ----------
 * SlonState
 * ----------
 */
struct SlonState_s
{
	char	   *actor;
	pid_t		pid;
	int			node;
	pid_t		conn_pid;
	char	   *activity;
	time_t		start_time;
	int64		event;
	char	   *event_type;
};

/* ----------
 * SlonNode
 * ----------
 */
struct SlonNode_s
{
	int			no_id;			/* node ID */
	int			no_active;		/* it's active state */
	char	   *no_comment;		/* comment field */
#if 0
	pthread_mutex_t node_lock;	/* mutex for node */
#endif

	char	   *pa_conninfo;	/* path to the node */
	int			pa_connretry;	/* connection retry interval */

	int64		last_event;		/* last event we have received */
	char	   *last_snapshot;	/* snapshot of last sync event */

	SlonThreadStatus listen_status;		/* status of the listen thread */
	pthread_t	listen_thread;	/* thread id of listen thread */
	SlonListen *listen_head;	/* list of origins we listen for */
	SlonListen *listen_tail;

	SlonThreadStatus worker_status;		/* status of the worker thread */
	pthread_t	worker_thread;	/* thread id of worker thread */
	pthread_mutex_t message_lock;		/* mutex for the message queue */
	pthread_cond_t message_cond;	/* condition variable for queue */
	SlonWorkMsg *message_head;
	SlonWorkMsg *message_tail;

	char	   *archive_name;
	char	   *archive_temp;
	char	   *archive_counter;
	char	   *archive_timestamp;
	FILE	   *archive_fp;

	SlonNode   *prev;
	SlonNode   *next;
};

/* ----------
 * SlonListen
 * ----------
 */
struct SlonListen_s
{
	int			li_origin;		/* origin of events */

	SlonListen *prev;
	SlonListen *next;
};

/* ----------
 * SlonSet
 * ----------
 */
struct SlonSet_s
{
	int			set_id;			/* set ID */
	int			set_origin;		/* set origin */
	char	   *set_comment;	/* set comment */

	int			sub_provider;	/* from where this node receives */
	/* data (if subscribed) */
	int			sub_forward;	/* if we need to forward data */
	int			sub_active;		/* if the subscription is active */

	SlonSet    *prev;
	SlonSet    *next;
};

/* ----------
 * SlonConn
 * ----------
 */
struct SlonConn_s
{
	char	   *symname;		/* Symbolic name of connection */
	struct SlonNode_s *node;	/* remote node this belongs to */
	PGconn	   *dbconn;			/* database connection */
	pthread_mutex_t conn_lock;	/* mutex for conn */
	pthread_cond_t conn_cond;	/* condition variable for conn */

	int			condition;		/* what are we waiting for? */
	struct timeval timeout;		/* timeofday for timeout */
	int			pg_version;		/* PostgreSQL version */
	int			conn_pid;		/* PID of connection */

	SlonConn   *prev;
	SlonConn   *next;
};

/* ----------
 * SlonDString
 * ----------
 */
#define		SLON_DSTRING_SIZE_INIT	256
#define		SLON_DSTRING_SIZE_INC	2

typedef struct
{
	size_t		n_alloc;
	size_t		n_used;
	char	   *data;
}	SlonDString;

#define		dstring_init(__ds) \
do { \
	(__ds)->n_alloc = SLON_DSTRING_SIZE_INIT; \
	(__ds)->n_used = 0; \
	(__ds)->data = malloc(SLON_DSTRING_SIZE_INIT); \
	if ((__ds)->data == NULL) { \
		slon_log(SLON_FATAL, "dstring_init: malloc() - %s", \
				strerror(errno)); \
		slon_abort(); \
	} \
} while (0)
#define		dstring_reset(__ds) \
do { \
	(__ds)->n_used = 0; \
	(__ds)->data[0] = '\0'; \
} while (0)
#define		dstring_free(__ds) \
do { \
	free((__ds)->data); \
	(__ds)->n_used = 0; \
	(__ds)->data = NULL; \
} while (0)
#define		dstring_nappend(__ds,__s,__n) \
do { \
	if ((__ds)->n_used + (__n) >= (__ds)->n_alloc)	\
	{ \
		while ((__ds)->n_used + (__n) >= (__ds)->n_alloc) \
			(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			slon_log(SLON_FATAL, "dstring_nappend: realloc() - %s", \
					strerror(errno)); \
			slon_abort(); \
		} \
	} \
	memcpy(&((__ds)->data[(__ds)->n_used]), (__s), (__n)); \
	(__ds)->n_used += (__n); \
} while (0)
#define		dstring_append(___ds,___s) \
do { \
	register int ___n = strlen((___s)); \
	dstring_nappend((___ds),(___s),___n); \
} while (0)
#define		dstring_addchar(__ds,__c) \
do { \
	if ((__ds)->n_used + 1 >= (__ds)->n_alloc)	\
	{ \
		(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			slon_log(SLON_FATAL, "dstring_addchar: realloc() - %s", \
					strerror(errno)); \
			slon_abort(); \
		} \
	} \
	(__ds)->data[(__ds)->n_used++] = (__c); \
} while (0)
#define		dstring_terminate(__ds) \
do { \
	(__ds)->data[(__ds)->n_used] = '\0'; \
} while (0)
#define		dstring_data(__ds)	((__ds)->data)


/* ----------
 * Macros to add and remove entries from double linked lists
 * ----------
 */
#define DLLIST_ADD_TAIL(_pf,_pl,_obj) \
do { \
	if ((_pl) == NULL) { \
		(_obj)->prev = (_obj)->next = NULL; \
		(_pf) = (_pl) = (_obj); \
	} else { \
		(_obj)->prev = (_pl); \
		(_obj)->next = NULL; \
		(_pl)->next = (_obj); \
		(_pl) = (_obj); \
	} \
} while (0)

#define DLLIST_ADD_HEAD(_pf,_pl,_obj) \
do { \
	if ((_pf) == NULL) { \
		(_obj)->prev = (_obj)->next = NULL; \
		(_pf) = (_pl) = (_obj); \
	} else { \
		(_obj)->prev = NULL; \
		(_obj)->next = (_pf); \
		(_pf)->prev = (_obj); \
		(_pf) = (_obj); \
	} \
} while (0)

#define DLLIST_REMOVE(_pf,_pl,_obj) \
do { \
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
	(_obj)->prev = (_obj)->next = NULL; \
} while (0)


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
typedef enum
{
	SCHED_STATUS_OK,
	SCHED_STATUS_SHUTDOWN,
	SCHED_STATUS_DONE,
	SCHED_STATUS_CANCEL,
	SCHED_STATUS_ERROR
}	ScheduleStatus;

/* ----------
 * Scheduler wait conditions
 * ----------
 */
#define SCHED_WAIT_SOCK_READ	1
#define SCHED_WAIT_SOCK_WRITE	2
#define SCHED_WAIT_TIMEOUT		4
#define SCHED_WAIT_CANCEL		8


/* ----------
 * Globals in runtime_config.c
 * ----------
 */
extern pid_t slon_pid;

#ifndef WIN32
extern pthread_mutex_t slon_watchdog_lock;
extern pid_t slon_watchdog_pid;
extern pid_t slon_worker_pid;
#endif
extern char *rtcfg_cluster_name;
extern char *rtcfg_namespace;
extern char *rtcfg_conninfo;
extern int	rtcfg_nodeid;
extern int	rtcfg_nodeactive;
extern char *rtcfg_nodecomment;
extern char rtcfg_lastevent[];

extern SlonNode *rtcfg_node_list_head;
extern SlonNode *rtcfg_node_list_tail;
extern SlonSet *rtcfg_set_list_head;
extern SlonSet *rtcfg_set_list_tail;


/* ----------
 * Functions in slon.c
 * ----------
 */
#ifndef WIN32
#define slon_abort() \
do { \
	pthread_mutex_lock(&slon_watchdog_lock); \
	if (slon_watchdog_pid >= 0) { \
		slon_log(SLON_DEBUG2, "slon_abort() from pid=%d\n", slon_pid); \
		(void) kill(slon_watchdog_pid, SIGTERM);			\
		slon_watchdog_pid = -1; \
	} \
	pthread_mutex_unlock(&slon_watchdog_lock); \
	pthread_exit(NULL); \
} while (0)
#define slon_restart() \
do { \
	pthread_mutex_lock(&slon_watchdog_lock); \
	if (slon_watchdog_pid >= 0) { \
		slon_log(SLON_DEBUG2, "slon_restart() from pid=%d\n", slon_pid); \
		(void) kill(slon_watchdog_pid, SIGHUP);			\
		slon_watchdog_pid = -1; \
	} \
	pthread_mutex_unlock(&slon_watchdog_lock); \
	pthread_exit(NULL); \
} while (0)
#define slon_retry() \
do { \
	pthread_mutex_lock(&slon_watchdog_lock); \
	if (slon_watchdog_pid >= 0) { \
		slon_log(SLON_DEBUG2, "slon_retry() from pid=%d\n", slon_pid); \
		(void) kill(slon_watchdog_pid, SIGUSR1);			\
		slon_watchdog_pid = -1; \
	} \
	pthread_mutex_unlock(&slon_watchdog_lock); \
	pthread_exit(NULL); \
} while (0)
#else							/* WIN32 */
/* On win32, we currently just bail out and let the service control manager
 * deal with possible restarts */
#define slon_abort() \
do { \
	WSACleanup(); \
	exit(1); \
} while (0)
#define slon_restart() \
do { \
	WSACleanup(); \
	exit(1); \
} while (0)
#define slon_retry() \
do { \
	WSACleanup(); \
	exit(1); \
} while (0)
#endif

extern void Usage(char *const argv[]);

extern int	sched_wakeuppipe[];
extern pthread_mutex_t slon_wait_listen_lock;
extern pthread_cond_t slon_wait_listen_cond;
extern int	slon_listen_started;

/* ----------
 * Functions in runtime_config.c
 * ----------
 */
extern void rtcfg_lock(void);
extern void rtcfg_unlock(void);

extern void rtcfg_storeNode(int no_id, char *no_comment);
extern void rtcfg_enableNode(int no_id);
extern void rtcfg_disableNode(int no_id);
extern SlonNode *rtcfg_findNode(int no_id);
extern int64 rtcfg_setNodeLastEvent(int no_id, int64 event_seq);
extern int64 rtcfg_getNodeLastEvent(int no_id);
extern void rtcfg_setNodeLastSnapshot(int no_id, char *snapshot);
extern char *rtcfg_getNodeLastSnapshot(int no_id);

extern void rtcfg_storePath(int pa_server, char *pa_conninfo,
				int pa_connretry);
extern void rtcfg_dropPath(int pa_server);

extern void rtcfg_reloadListen(PGconn *db);
extern void rtcfg_storeListen(int li_origin, int li_provider);
extern void rtcfg_dropListen(int li_origin, int li_provider);

extern void rtcfg_storeSet(int set_id, int set_origin, char *set_comment);
extern void rtcfg_dropSet(int set_id);
extern void rtcfg_moveSet(int set_id, int old_origin, int new_origin,
			  int sub_provider);

extern void rtcfg_storeSubscribe(int sub_set, int sub_provider,
					 char *sub_forward);
extern void rtcfg_enableSubscription(int sub_set, int sub_provider,
						 char *sub_forward);
extern void rtcfg_unsubscribeSet(int sub_set);

extern void rtcfg_needActivate(int no_id);
extern void rtcfg_doActivate(void);
extern void rtcfg_joinAllRemoteThreads(void);

extern void rtcfg_seq_bump(void);
extern int64 rtcfg_seq_get(void);


/* ----------
 * Functions in local_node.c
 * ----------
 */
extern void *slon_localEventThread(void *dummy);

/*
 * ----------
 * Global variables in cleanup_thread.c
 * ----------
 */

extern int	vac_frequency;
extern char *cleanup_interval;

/* ----------
 * Functions in cleanup_thread.c
 * ----------
 */
extern void *cleanupThread_main(void *dummy);

/* ----------
 * Global variables in sync_thread.c
 * ----------
 */
extern int	sync_interval;
extern int	sync_interval_timeout;


/* ----------
 * Functions in sync_thread.c
 * ----------
 */
extern void *syncThread_main(void *dummy);

/* ----------
 * Functions in monitor_thread.c
 * ----------
 */
extern void *monitorThread_main(void *dummy);
extern void monitor_state(const char *actor, int node, pid_t conn_pid, const char *activity, int64 event, const char *event_type);

/* ----------
 * Globals in monitor_thread.c
 * ----------
 */
extern int	monitor_interval;
extern bool monitor_threads;


/* ----------
 * Functions in local_listen.c
 * ----------
 */
extern void *localListenThread_main(void *dummy);


/* ----------
 * Functions in remote_listen.c
 * ----------
 */
extern void *remoteListenThread_main(void *cdata);


/* ----------
 * Globals in remote_worker.c
 * ----------
 */
extern int	sync_group_maxsize;
extern int	explain_interval;


/* ----------
 * Functions in remote_worker.c
 * ----------
 */
extern void *remoteWorkerThread_main(void *cdata);
extern void remoteWorker_event(int event_provider,
				   int ev_origin, int64 ev_seqno,
				   char *ev_timestamp,
				   char *ev_snapshot, char *ev_mintxid, char *ev_maxtxid,
				   char *ev_type,
				   char *ev_data1, char *ev_data2,
				   char *ev_data3, char *ev_data4,
				   char *ev_data5, char *ev_data6,
				   char *ev_data7, char *ev_data8);
extern void remoteWorker_wakeup(int no_id);
extern void remoteWorker_confirm(int no_id,
					 char *con_origin_c, char *con_received_c,
					 char *con_seqno_c, char *con_timestamp_c);


/* ----------
 * Functions in scheduler.c
 * ----------
 */
extern int	sched_start_mainloop(void);
extern int	sched_wait_mainloop(void);
extern int	sched_wait_conn(SlonConn * conn, int condition);
extern int	sched_wait_time(SlonConn * conn, int condition, int msec);
extern int	sched_msleep(SlonNode * node, int msec);
extern int	sched_get_status(void);
extern int	sched_wakeup_node(int no_id);


/* ----------
 * Functions in dbutils.c
 * ----------
 */
extern SlonConn *slon_connectdb(char *conninfo, char *symname);
extern void slon_disconnectdb(SlonConn * conn);
extern SlonConn *slon_make_dummyconn(char *symname);
extern void slon_free_dummyconn(SlonConn * conn);

extern int	db_getLocalNodeId(PGconn *conn);
extern int	db_checkSchemaVersion(PGconn *conn);

extern void slon_mkquery(SlonDString * ds, char *fmt,...);
extern void slon_appendquery(SlonDString * ds, char *fmt,...);
extern char *sql_on_connection;

/* ----------
 * Globals in misc.c
 * ----------
 */
extern int	slon_log_level;

#if !defined(pgpipe) && !defined(WIN32)
/* -----------------------------------
 * pgpipe is not defined in PG pre-8.0
 * -----------------------------------
 */
#define pgpipe(a)			pipe(a)
#define piperead(a,b,c)		read(a,b,c)
#define pipewrite(a,b,c)	write(a,b,c)
#endif

#if defined(WIN32)
#define snprintf pg_snprintf
#endif
#endif   /* SLON_H_INCLUDED */


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
