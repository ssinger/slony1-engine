/*-------------------------------------------------------------------------
 * remote_worker.c
 *
 *	Implementation of the thread processing remote events.
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
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif


#include "slon.h"
#include "../parsestatements/scanner.h"
extern int	STMTS[MAXSTATEMENTS];

#define MAXGROUPSIZE 10000		/* What is the largest number of SYNCs we'd
					 * want to group together??? */


/* ----------
 * Local definitions
 * ----------
 */

/*
 * Internal message types
 */
typedef enum
{
		WMSG_EVENT,
		WMSG_WAKEUP,
		WMSG_CONFIRM
} MessageType;


/*
 * Message structure resulting from a remote event
 */
typedef struct SlonWorkMsg_event_s SlonWorkMsg_event;
struct SlonWorkMsg_event_s
{
	MessageType			msg_type;
	SlonWorkMsg_event *prev;
	SlonWorkMsg_event *next;

	int			event_provider;

	int			ev_origin;
	int64		ev_seqno;
	char	   *ev_timestamp_c;
	char	   *ev_snapshot_c;
	char	   *ev_mintxid_c;
	char	   *ev_maxtxid_c;
	char	   *ev_type;
	char	   *ev_data1;
	char	   *ev_data2;
	char	   *ev_data3;
	char	   *ev_data4;
	char	   *ev_data5;
	char	   *ev_data6;
	char	   *ev_data7;
	char	   *ev_data8;
	char		raw_data[1];
};


/*
 * Message structure resulting from a remote confirm
 */
typedef struct SlonWorkMsg_confirm_s SlonWorkMsg_confirm;
struct SlonWorkMsg_confirm_s
{
	MessageType			msg_type;
	SlonWorkMsg_confirm *prev;
	SlonWorkMsg_confirm *next;

	int			con_origin;
	int			con_received;
	int64		con_seqno;
	char		con_timestamp_c[64];
};


/*
 * Generic message header
 */
struct SlonWorkMsg_s
{
	MessageType			msg_type;
	SlonWorkMsg *prev;
	SlonWorkMsg *next;
};


typedef struct ProviderInfo_s ProviderInfo;
typedef struct ProviderSet_s ProviderSet;
typedef struct WorkerGroupData_s WorkerGroupData;
typedef struct WorkerGroupLine_s WorkerGroupLine;


struct ProviderSet_s
{
	int			set_id;
	int			sub_forward;
	char		ssy_seqno[64];

	ProviderSet *prev;
	ProviderSet *next;
};


typedef enum
{
	SLON_WG_IDLE,
	SLON_WG_BUSY,
	SLON_WG_DONE,
	SLON_WG_EXIT,
	SLON_WG_ABORT
} WorkGroupStatus;


typedef enum
{
	SLON_WGLC_ACTION,
	SLON_WGLC_DONE,
	SLON_WGLC_ERROR
} WorkGroupLineCode;

typedef struct PerfMon_s PerfMon;   /* Structure for doing performance monitoring */
struct PerfMon_s 
{
	struct timeval prev_t;
	struct timeval now_t;
	double prov_query_t;      /* Time spent running queries against the provider */
	int prov_query_c;         /* Number of queries run against the provider */
	double subscr_query_t;      /* Time spent running prep queries against the subscriber */
	int subscr_query_c;         /* Number of prep queries run against the subscriber */
	double subscr_iud__t;    /* Time spent running IUD against subscriber */
	int subscr_iud__c;       /* Number of IUD requests run against subscriber */
	double large_tuples_t;          /* Number of large tuples processed */
	int large_tuples_c;          /* Number of large tuples processed */
	int num_inserts;
	int num_updates;
	int num_deletes;
	int num_truncates;
};

struct ProviderInfo_s
{
	int			no_id;
	char	   *pa_conninfo;
	int			pa_connretry;
	SlonConn   *conn;

	WorkerGroupData *wd;

	pthread_t	helper_thread;
	pthread_mutex_t helper_lock;
	pthread_cond_t helper_cond;
	WorkGroupStatus helper_status;
	SlonDString helper_query;
	int log_status;

	ProviderSet *set_head;
	ProviderSet *set_tail;

	ProviderInfo *prev;
	ProviderInfo *next;
};


struct WorkerGroupData_s
{
	SlonNode   *node;

	int			active_log_table;

	char	   *tab_forward;
	char	  **tab_fqname;
	int			tab_fqname_size;

	ProviderInfo *provider_head;
	ProviderInfo *provider_tail;

	pthread_mutex_t workdata_lock;
	WorkGroupStatus workgroup_status;
	int			workdata_largemem;

	pthread_cond_t repldata_cond;
	WorkerGroupLine *repldata_head;
	WorkerGroupLine *repldata_tail;

	pthread_cond_t linepool_cond;
	WorkerGroupLine *linepool_head;
	WorkerGroupLine *linepool_tail;
};


struct WorkerGroupLine_s
{
	WorkGroupLineCode code;
	ProviderInfo *provider;
	SlonDString data;
	SlonDString log;
	int			line_largemem;

	WorkerGroupLine *prev;
	WorkerGroupLine *next;
};


/*
 * Global status for all remote worker threads, remembering the last seen
 * confirmed sequence number.
 */
struct node_confirm_status
{
	int			con_origin;
	int			con_received;
	int64		con_seqno;

	struct node_confirm_status *prev;
	struct node_confirm_status *next;
};
static struct node_confirm_status *node_confirm_head = NULL;
static struct node_confirm_status *node_confirm_tail = NULL;
static pthread_mutex_t node_confirm_lock = PTHREAD_MUTEX_INITIALIZER;

int			sync_group_maxsize;
int			sync_max_rowsize;
int			sync_max_largemem;
int			explain_interval;
time_t		explain_lastsec;
int			explain_thistime;

static int	last_sync_group_size;
static int	next_sync_group_size;

int			desired_sync_time;
static int	ideal_sync;
static struct timeval sync_start;
static struct timeval sync_end;
static int	last_sync_length;
static int	max_sync;
int			min_sync;
int			quit_sync_provider;
int			quit_sync_finalsync;

/* ----------
 * Local functions
 * ----------
 */
/*
 * Monitoring data structure
 */

static void init_perfmon(PerfMon *pm);
static void start_monitored_event(PerfMon *pm);
static void monitor_provider_query(PerfMon *pm);
static void monitor_subscriber_query(PerfMon *pm);
static void monitor_subscriber_iud(PerfMon *pm);
static void monitor_largetuples(PerfMon *pm);

static void adjust_provider_info(SlonNode *node,
					 WorkerGroupData *wd, int cleanup);
static int query_execute(SlonNode *node, PGconn *dbconn,
			  SlonDString *dsp);
static void query_append_event(SlonDString *dsp,
							   SlonWorkMsg_event *event);
static void store_confirm_forward(SlonNode *node, SlonConn *conn,
					  SlonWorkMsg_confirm *confirm);
static int64 get_last_forwarded_confirm(int origin, int receiver);
static int copy_set(SlonNode *node, SlonConn *local_conn, int set_id,
		 SlonWorkMsg_event *event);
static int sync_event(SlonNode *node, SlonConn *local_conn,
		   WorkerGroupData *wd, SlonWorkMsg_event *event);
static void *sync_helper(void *cdata);


static int archive_open(SlonNode *node, char *seqbuf,
			 PGconn *dbconn);
static int	archive_close(SlonNode *node);
static void archive_terminate(SlonNode *node);
static int	archive_append_ds(SlonNode *node, SlonDString *ds);
static int	archive_append_str(SlonNode *node, const char *s);
static int	archive_append_data(SlonNode *node, const char *s, int len);


static void compress_actionseq(const char *ssy_actionseq, SlonDString *action_subquery);

static int	check_set_subscriber(int set_id, int node_id, PGconn *local_dbconn);

/* ----------
 * slon_remoteWorkerThread
 *
 * Listen for events on the local database connection. This means, events
 * generated by the local node only.
 * ----------
 */
void *
remoteWorkerThread_main(void *cdata)
{
	SlonNode   *node = (SlonNode *) cdata;
	WorkerGroupData *wd;
	SlonConn   *local_conn;
	PGconn	   *local_dbconn;
	SlonDString query1;
	SlonDString query2;
	SlonDString query3;
	SlonWorkMsg *msg;
	SlonWorkMsg_event *event;
	bool		check_config = true;
	int64		curr_config = -1;
	char		seqbuf[64];
	bool		event_ok;
	bool		need_reloadListen = false;
	char		conn_symname[32];

	slon_log(SLON_INFO,
			 "remoteWorkerThread_%d: thread starts\n",
			 node->no_id);

	/*
	 * Initialize local data
	 */
	wd = (WorkerGroupData *) malloc(sizeof(WorkerGroupData));
	if (wd == 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: could not malloc() space for WorkerGroupData\n",
				node->no_id);
		slon_retry();
	}
	else
	{
		memset(wd, 0, sizeof(WorkerGroupData));
	}

	pthread_mutex_init(&(wd->workdata_lock), NULL);
	pthread_cond_init(&(wd->repldata_cond), NULL);
	pthread_cond_init(&(wd->linepool_cond), NULL);
	pthread_mutex_lock(&(wd->workdata_lock));
	wd->workgroup_status = SLON_WG_IDLE;
	wd->node = node;
	wd->workdata_largemem = 0;

	wd->tab_fqname_size = SLON_MAX_PATH;
	wd->tab_fqname = (char **) malloc(sizeof(char *) * wd->tab_fqname_size);
	memset(wd->tab_fqname, 0, sizeof(char *) * wd->tab_fqname_size);
	wd->tab_forward = malloc(wd->tab_fqname_size);
	memset(wd->tab_forward, 0, (size_t) (wd->tab_fqname_size));

	dstring_init(&query1);
	dstring_init(&query2);
	dstring_init(&query3);

	/*
	 * Connect to the local database
	 */
	sprintf(conn_symname, "remoteWorkerThread_%d", node->no_id);
	if ((local_conn = slon_connectdb(rtcfg_conninfo, conn_symname)) == NULL)
		slon_retry();
	local_dbconn = local_conn->dbconn;

	monitor_state(conn_symname, node->no_id, local_conn->conn_pid, "thread main loop", 0, "n/a");

	/*
	 * Put the connection into replication mode
	 */
	(void) slon_mkquery(&query1,
						"set session_replication_role = replica; ",
						rtcfg_namespace, rtcfg_cluster_name);
	if (query_execute(node, local_dbconn, &query1) < 0)
		slon_retry();

	/*
	 * Work until shutdown or node destruction
	 */
	while (true)
	  {
		/*
		 * If we got the special WMSG_WAKEUP, check the current runmode of the
		 * scheduler and the status of our node.
		 */
		if (sched_get_status() != SCHED_STATUS_OK)
			break;

		if (check_config)
		{
			rtcfg_lock();
			if (!node->no_active)
			{
				rtcfg_unlock();
				break;
			}
			if (node->worker_status != SLON_TSTAT_RUNNING)
			{
				rtcfg_unlock();
				break;
			}

			if (curr_config != rtcfg_seq_get())
			{
				adjust_provider_info(node, wd, false);
				curr_config = rtcfg_seq_get();

				/*
				 * If we do archive logging, we need the ssy_seqno of all sets
				 * this worker is replicating.
				 */
				if (archive_dir)
				{
					ProviderInfo *pinfo;
					ProviderSet *pset;
					PGresult   *res;

					for (pinfo = wd->provider_head; pinfo != NULL; pinfo = pinfo->next)
					{
						for (pset = pinfo->set_head; pset != NULL; pset = pset->next)
						{
							slon_mkquery(&query1,
										 "select max(ssy_seqno) from ("
									   "select ssy_seqno from %s.sl_setsync "
										 "  where ssy_setid = %d "
										 "    and ssy_origin = %d "
										 "union "
										 "select ev_seqno from %s.sl_event "
										 "  where ev_origin = %d "
										 "    and ev_type <> 'SYNC' "
										 ") as S; ",
								  rtcfg_namespace, pset->set_id, node->no_id,
										 rtcfg_namespace, node->no_id);
							if (query_execute(node, local_dbconn, &query1) < 0)
								slon_retry();

							res = PQexec(local_dbconn, dstring_data(&query1));
							if (PQresultStatus(res) != PGRES_TUPLES_OK)
							{
								slon_log(SLON_FATAL, "remoteWorkerThread_%d: \"%s\" %s",
										 node->no_id, dstring_data(&query1),
										 PQresultErrorMessage(res));
								PQclear(res);
								slon_retry();
							}
							if (PQntuples(res) != 1)
							{
								slon_log(SLON_FATAL, "remoteWorkerThread_%d: Query \"%s\" did not return one row\n",
										 node->no_id, dstring_data(&query1));
								PQclear(res);
								slon_retry();
							}
							strcpy(pset->ssy_seqno, PQgetvalue(res, 0, 0));
							PQclear(res);

							slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: set %d starts at ssy_seqno %s\n",
								 node->no_id, pset->set_id, pset->ssy_seqno);
						}
					}
				}
			}
			rtcfg_unlock();

			check_config = false;
		}

		/*
		 * Receive the next message from the queue. If there is no one
		 * present, wait on the condition variable.
		 */
		pthread_mutex_lock(&(node->message_lock));
		if (node->message_head == NULL)
		{
			pthread_cond_wait(&(node->message_cond), &(node->message_lock));
			if (node->message_head == NULL)
			{
				slon_log(SLON_FATAL,
						 "remoteWorkerThread_%d: got message "
						 "condition but queue is empty\n",
						 node->no_id);
				slon_retry();
			}
		}
		msg = node->message_head;
		DLLIST_REMOVE(node->message_head, node->message_tail, msg);
		pthread_mutex_unlock(&(node->message_lock));

		/*
		 * Process WAKEUP messages by simply setting the check_config flag.
		 */
		if (msg->msg_type == WMSG_WAKEUP)
		{
#ifdef SLON_MEMDEBUG
			memset(msg, 55, sizeof(SlonWorkMsg));
#endif
			free(msg);
			check_config = true;
			continue;
		}

		/*
		 * Process confirm messages.
		 */
		if (msg->msg_type == WMSG_CONFIRM)
		{
			store_confirm_forward(node, local_conn,
								  (SlonWorkMsg_confirm *) msg);
#ifdef SLON_MEMDEBUG
			memset(msg, 55, sizeof(SlonWorkMsg_confirm));
#endif
			free(msg);
			continue;
		}

		/*
		 * This must be an event message then.
		 */
		if (msg->msg_type != WMSG_EVENT)
		{
			slon_log(SLON_FATAL,
					 "remoteWorkerThread_%d: unknown WMSG type %d\n",
					 node->no_id, msg->msg_type);
			slon_retry();
		}
		event_ok = true;
		event = (SlonWorkMsg_event *) msg;
		sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);

		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "Received event #%d from %s type:%s\n",
				 node->no_id, event->ev_origin, seqbuf,
				 event->ev_type);

		/*
		 * Construct the queries to begin a transaction, insert the event 
		 * into our local sl_event
		 * table and confirm it in our local sl_confirm table. When this
		 * transaction commits, every other remote node listening for events
		 * with us as a provider will pick up the news.
		 */
		(void) slon_mkquery(&query1,
							"begin transaction; "
							"set transaction isolation level serializable; ");

		monitor_state(conn_symname, node->no_id, local_conn->conn_pid, event->ev_type, event->ev_seqno, event->ev_type);

		/*
		 * Event type specific processing
		 */
		if (strcmp(event->ev_type, "SYNC") == 0)
		{
			SlonWorkMsg_event *sync_group[MAXGROUPSIZE + 1];
			int			sync_group_size;

			int			seconds;
			ScheduleStatus			rc;
			int			i;

			/*
			 * SYNC event
			 */

			sync_group[0] = event;
			sync_group_size = 1;

			if (true)
			{
				/* Force last_sync_group_size to a reasonable range */
				if (last_sync_group_size < 1)
					last_sync_group_size = 1;
				if (last_sync_group_size > MAXGROUPSIZE)
					last_sync_group_size = MAXGROUPSIZE;

				gettimeofday(&sync_end, NULL);
				last_sync_length =
					(sync_end.tv_sec - sync_start.tv_sec) * 1000 +
					(sync_end.tv_usec - sync_start.tv_usec) / 1000;

				/* Force last_sync_length to a reasonable range */
				if ((last_sync_length < 10) || (last_sync_length > 1000000))
				{
					/* sync_length seems to be trash - force group size to 1 */
					next_sync_group_size = 1;
				}
				else
				{
					/*
					 * Estimate an "ideal" number of syncs based on how long
					 * they took last time
					 */
					if (desired_sync_time != 0)
					{
						ideal_sync = (last_sync_group_size * desired_sync_time) / last_sync_length;
					}
					else
					{
						ideal_sync = sync_group_maxsize;
					}
					max_sync = last_sync_group_size * 2 + 1;
					next_sync_group_size = ideal_sync;
					if (next_sync_group_size > max_sync)
						next_sync_group_size = max_sync;
					if (next_sync_group_size < 1)
						next_sync_group_size = 1;
					if (next_sync_group_size > sync_group_maxsize)
						next_sync_group_size = sync_group_maxsize;
					slon_log(SLON_DEBUG1, "calc sync size - last time: %d last length: %d ideal: %d proposed size: %d\n",
							 last_sync_group_size, last_sync_length, ideal_sync, next_sync_group_size);
				}


				/*
				 * Quit upon receiving event # quit_sync_number from node #
				 * quit_sync_provider
				 */
				if (quit_sync_provider != 0)
				{
					if (quit_sync_provider == node->no_id)
					{
						if ((next_sync_group_size + (event->ev_seqno)) > quit_sync_finalsync)
						{
							next_sync_group_size = quit_sync_finalsync - event->ev_seqno;
						}
						if (event->ev_seqno >= quit_sync_finalsync)
						{
							slon_log(SLON_FATAL, "ABORT at sync %d per command line request%n", quit_sync_finalsync);
							slon_mkquery(&query2, "rollback transaction; ");
							query_execute(node, local_dbconn, &query2);
							dstring_reset(&query2);

							slon_retry();
						}
					}
				}

				gettimeofday(&sync_start, NULL);

				pthread_mutex_lock(&(node->message_lock));
				while (sync_group_size < next_sync_group_size && sync_group_size < MAXGROUPSIZE && node->message_head != NULL)
				{
					if (node->message_head->msg_type != WMSG_EVENT)
						break;
					if (strcmp(((SlonWorkMsg_event *) (node->message_head))->ev_type,
							   "SYNC") != 0)
						break;

					msg = node->message_head;
					event = (SlonWorkMsg_event *) (node->message_head);
					sync_group[sync_group_size++] = event;
					DLLIST_REMOVE(node->message_head, node->message_tail, msg);
					last_sync_group_size++;
				}
				pthread_mutex_unlock(&(node->message_lock));
			}
			while (true)
			{
				/*
				 * Execute the forwarding stuff, but do not commit
				 * the transaction yet.
				 */
				if (query_execute(node, local_dbconn, &query1) < 0)
					slon_retry();

				/*
				 * Process the sync and apply the replication data. If
				 * successful, exit this loop and commit the transaction.
				 */
				seconds = sync_event(node, local_conn, wd, event);
				if (seconds == 0)
				{
					rc = SCHED_STATUS_OK;
					break;
				}

				/*
				 * Something went wrong. Rollback and try again after the
				 * specified timeout.
				 */
				archive_terminate(node);
				(void) slon_mkquery(&query2, "rollback transaction");
				if (query_execute(node, local_dbconn, &query2) < 0)
					slon_retry();

				if ((rc = sched_msleep(node, seconds * 1000)) != SCHED_STATUS_OK)
					break;
			}
			if (rc != SCHED_STATUS_OK)
				break;

			/*
			 * replace query1 with the forwarding of all the grouped sync
			 * events and a commit. Also free all the WMSG structures except
			 * the last one (it's freed further down).
			 */
			dstring_reset(&query1);
			last_sync_group_size = 0;
			for (i = 0; i < sync_group_size; i++)
			{
					query_append_event(&query1, sync_group[i]);
					if (i < (sync_group_size - 1))
							free(sync_group[i]);
					last_sync_group_size++;
			}
			slon_appendquery(&query1, "commit transaction;");

			if (query_execute(node, local_dbconn, &query1) < 0)
				slon_retry();
		}
		else	/* not SYNC */
		{
		
			/**
			 * open the transaction.
			 */
			if (query_execute(node, local_dbconn, &query1) < 0)
				slon_retry();
			dstring_reset(&query1);
			/*
			 * For all non-SYNC events, we write at least a standard event
			 * tracking log file and adjust the ssy_seqno in our internal
			 * tracking.
			 */
			if (archive_dir)
			{
				char		buf[256];

				if (archive_open(node, seqbuf, local_dbconn) < 0)
					slon_retry();
				sprintf(buf, "-- %s", event->ev_type);
				if (archive_append_str(node, buf) < 0)
					slon_retry();

				/*
				 * Leave the archive open for event specific actions.
				 */
			}

			/*
			 * Simple configuration events. Call the corresponding runtime
			 * config function, add the query to call the configuration event
			 * specific stored procedure.
			 */
			if (strcmp(event->ev_type, "STORE_NODE") == 0)
			{
				int			no_id = (int) strtol(event->ev_data1, NULL, 10);
				char	   *no_comment = event->ev_data2;

				if (no_id != rtcfg_nodeid)
					rtcfg_storeNode(no_id, no_comment);

				slon_appendquery(&query1,
								 "select %s.storeNode_int(%d, '%q'); ",
								 rtcfg_namespace,
								 no_id, no_comment);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "ENABLE_NODE") == 0)
			{
				int			no_id = (int) strtol(event->ev_data1, NULL, 10);

				if (no_id != rtcfg_nodeid)
					rtcfg_enableNode(no_id);

				slon_appendquery(&query1,
								 "select %s.enableNode_int(%d); ",
								 rtcfg_namespace,
								 no_id);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "DROP_NODE") == 0)
			{
				int			no_id = (int) strtol(event->ev_data1, NULL, 10);

				if (no_id != rtcfg_nodeid)
					rtcfg_disableNode(no_id);

				slon_appendquery(&query1,
								 "select %s.dropNode_int(%d); ",
								 rtcfg_namespace,
								 no_id);

				/*
				 * If this is our own nodeid, then calling disableNode_int()
				 * will destroy the whole configuration including the entire
				 * schema. Make sure we call just that and get out of here
				 * ASAP!
				 */
				if (no_id == rtcfg_nodeid)
				{
					slon_log(SLON_WARN, "remoteWorkerThread_%d: "
							 "got DROP NODE for local node ID\n",
							 node->no_id);

					slon_appendquery(&query1, "commit transaction; ");
					if (query_execute(node, local_dbconn, &query1) < 0)
						slon_retry();

					(void) slon_mkquery(&query1, "select %s.uninstallNode(); ",
										rtcfg_namespace);
					if (query_execute(node, local_dbconn, &query1) < 0)
						slon_retry();

					(void) slon_mkquery(&query1, "drop schema %s cascade; ",
										rtcfg_namespace);
					query_execute(node, local_dbconn, &query1);

					slon_retry();
				}

				/*
				 * this is a remote node. Arrange for daemon restart.
				 */
				slon_appendquery(&query1,
								 "notify \"_%s_Restart\"; ",
								 rtcfg_cluster_name);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "CLONE_NODE") == 0)
			{
				int			no_id = (int) strtol(event->ev_data1, NULL, 10);
				int			no_provider = (int) strtol(event->ev_data2, NULL, 10);
				char	   *no_comment = event->ev_data3;
				int64       last_event_id;
				PGresult   *res;

				rtcfg_storeNode(no_id, no_comment);
				slon_appendquery(&query1,
							"select %s.cloneNodePrepare_int(%d, %d, '%q'); ",
								 rtcfg_namespace,
								 no_id, no_provider, no_comment);
				slon_appendquery(&query1,"select coalesce(max(con_seqno),0)"
								 "from %s.sl_confirm "
								 "  where con_origin = %d  and con_received"
								 "= %d", rtcfg_namespace, node->no_id, no_id);
				res = PQexec(local_dbconn, dstring_data(&query1));
				if (PQresultStatus(res) != PGRES_TUPLES_OK )
				{
				  slon_log(SLON_ERROR,"remoteWorkerThread_%d error querying "
						   "last confirmed id for node %d in CLONE NODE\n",
						   node->no_id, no_id);
				  slon_retry();
				}
				if( PQntuples(res) 	!= 0)
				{
				  last_event_id = strtoll(PQgetvalue(res, 0, 0),NULL,10);
				  rtcfg_setNodeLastEvent(no_id, last_event_id);
				}
				PQclear(res);
				dstring_reset(&query1);
				rtcfg_enableNode(no_id);

			}
			else if (strcmp(event->ev_type, "STORE_PATH") == 0)
			{
				int			pa_server = (int) strtol(event->ev_data1, NULL, 10);
				int			pa_client = (int) strtol(event->ev_data2, NULL, 10);
				char	   *pa_conninfo = event->ev_data3;
				int			pa_connretry = (int) strtol(event->ev_data4, NULL, 10);

				if (pa_client == rtcfg_nodeid)
					rtcfg_storePath(pa_server, pa_conninfo, pa_connretry);

				slon_appendquery(&query1,
								 "lock table %s.sl_config_lock;"
								 "select %s.storePath_int(%d, %d, '%q', %d); ",
								 rtcfg_namespace,
								 rtcfg_namespace,
							pa_server, pa_client, pa_conninfo, pa_connretry);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "DROP_PATH") == 0)
			{
				int			pa_server = (int) strtol(event->ev_data1, NULL, 10);
				int			pa_client = (int) strtol(event->ev_data2, NULL, 10);

				if (pa_client == rtcfg_nodeid)
					rtcfg_dropPath(pa_server);

				slon_appendquery(&query1,
								 "select %s.dropPath_int(%d, %d); ",
								 rtcfg_namespace,
								 pa_server, pa_client);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "STORE_LISTEN") == 0)
			{
				int			li_origin = (int) strtol(event->ev_data1, NULL, 10);
				int			li_provider = (int) strtol(event->ev_data2, NULL, 10);
				int			li_receiver = (int) strtol(event->ev_data3, NULL, 10);

				if (li_receiver == rtcfg_nodeid)
					rtcfg_storeListen(li_origin, li_provider);

				slon_appendquery(&query1,
								 "select %s.storeListen_int(%d, %d, %d); ",
								 rtcfg_namespace,
								 li_origin, li_provider, li_receiver);
			}
			else if (strcmp(event->ev_type, "DROP_LISTEN") == 0)
			{
				int			li_origin = (int) strtol(event->ev_data1, NULL, 10);
				int			li_provider = (int) strtol(event->ev_data2, NULL, 10);
				int			li_receiver = (int) strtol(event->ev_data3, NULL, 10);

				if (li_receiver == rtcfg_nodeid)
					rtcfg_dropListen(li_origin, li_provider);

				slon_appendquery(&query1,
								 "select %s.dropListen_int(%d, %d, %d); ",
								 rtcfg_namespace,
								 li_origin, li_provider, li_receiver);
			}
			else if (strcmp(event->ev_type, "STORE_SET") == 0)
			{
				int			set_id = (int) strtol(event->ev_data1, NULL, 10);
				int			set_origin = (int) strtol(event->ev_data2, NULL, 10);
				char	   *set_comment = event->ev_data3;

				if (set_origin != rtcfg_nodeid)
					rtcfg_storeSet(set_id, set_origin, set_comment);

				slon_appendquery(&query1,
								 "select %s.storeSet_int(%d, %d, '%q'); ",
								 rtcfg_namespace,
								 set_id, set_origin, set_comment);
			}
			else if (strcmp(event->ev_type, "DROP_SET") == 0)
			{
				int			set_id = (int) strtol(event->ev_data1, NULL, 10);

				rtcfg_dropSet(set_id);

				slon_appendquery(&query1,
								 "select %s.dropSet_int(%d); ",
								 rtcfg_namespace, set_id);
			}
			else if (strcmp(event->ev_type, "MERGE_SET") == 0)
			{
				int			set_id = (int) strtol(event->ev_data1, NULL, 10);
				int			add_id = (int) strtol(event->ev_data2, NULL, 10);

				rtcfg_dropSet(add_id);

				slon_appendquery(&query1,
								 "select %s.mergeSet_int(%d, %d); ",
								 rtcfg_namespace,
								 set_id, add_id);

			}
			else if (strcmp(event->ev_type, "SET_ADD_TABLE") == 0)
			{
				/*
				 * Nothing to do ATM ... we don't support adding tables to
				 * subscribed sets yet and table information is not maintained
				 * in the runtime configuration.
				 */
			}
			else if (strcmp(event->ev_type, "SET_ADD_SEQUENCE") == 0)
			{
				/*
				 * Nothing to do ATM ... we don't support adding sequences to
				 * subscribed sets yet and sequences information is not
				 * maintained in the runtime configuration.
				 */
			}
			else if (strcmp(event->ev_type, "SET_DROP_TABLE") == 0)
			{
				int			tab_id = (int) strtol(event->ev_data1, NULL, 10);

				slon_appendquery(&query1, "select %s.setDropTable_int(%d);",
								 rtcfg_namespace,
								 tab_id);
			}
			else if (strcmp(event->ev_type, "SET_DROP_SEQUENCE") == 0)
			{
				int			seq_id = (int) strtol(event->ev_data1, NULL, 10);

				slon_appendquery(&query1, "select %s.setDropSequence_int(%d);",
								 rtcfg_namespace,
								 seq_id);
			}
			else if (strcmp(event->ev_type, "SET_MOVE_TABLE") == 0)
			{
				int			tab_id = (int) strtol(event->ev_data1, NULL, 10);
				int			new_set_id = (int) strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1, "select %s.setMoveTable_int(%d, %d);",
								 rtcfg_namespace,
								 tab_id, new_set_id);
			}
			else if (strcmp(event->ev_type, "SET_MOVE_SEQUENCE") == 0)
			{
				int			seq_id = (int) strtol(event->ev_data1, NULL, 10);
				int			new_set_id = (int) strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1, "select %s.setMoveSequence_int(%d, %d);",
								 rtcfg_namespace,
								 seq_id, new_set_id);
			}
			else if (strcmp(event->ev_type, "ACCEPT_SET") == 0)
			{
				int			set_id,
							old_origin,
							new_origin;
				char	   *wait_seqno;
				PGresult   *res;

				slon_log(SLON_INFO, "start processing ACCEPT_SET\n");
				set_id = (int) strtol(event->ev_data1, NULL, 10);
				slon_log(SLON_INFO, "ACCEPT: set=%d\n", set_id);
				old_origin = (int) strtol(event->ev_data2, NULL, 10);
				slon_log(SLON_INFO, "ACCEPT: old origin=%d\n", old_origin);
				new_origin = (int) strtol(event->ev_data3, NULL, 10);
				slon_log(SLON_INFO, "ACCEPT: new origin=%d\n", new_origin);
				wait_seqno = event->ev_data4;
				slon_log(SLON_INFO, "ACCEPT: move set seq=%s\n", wait_seqno);

				slon_log(SLON_INFO, "got parms ACCEPT_SET\n");

				/*
				 * If we're a remote node, and haven't yet received the
				 * MOVE/FAILOVER_SET event from the new origin, then we'll
				 * need to sleep a bit...  This avoids a race condition where
				 * new SYNCs take place on the new origin, and are ignored on
				 * some subscribers (and their children) because the MOVE_SET
				 * wasn't yet received and processed
				 */

				if ((rtcfg_nodeid != old_origin) && (rtcfg_nodeid != new_origin))
				{
					slon_log(SLON_DEBUG1, "ACCEPT_SET - node not origin\n");
					(void) slon_mkquery(&query2,
										"select 1 from %s.sl_event "
										"where "
										"     (ev_origin = %d and "
										"      ev_seqno = %s and "
										"      ev_type = 'MOVE_SET' and "
										"      ev_data1 = '%d' and "
										"      ev_data2 = '%d' and "
										"      ev_data3 = '%d') "
										"or "
										"     (ev_origin = %d and "
										"      ev_seqno = %s and "
										"      ev_type = 'FAILOVER_SET' and "
										"      ev_data1 = '%d' and "
										"      ev_data2 = '%d' and "
										"      ev_data3 = '%d'); ",

										rtcfg_namespace,
					  old_origin, wait_seqno, set_id, old_origin, new_origin,
					 old_origin, wait_seqno, old_origin, new_origin, set_id);

					res = PQexec(local_dbconn, dstring_data(&query2));
					while (PQntuples(res) == 0)
					{
						PQclear(res);

						slon_log(SLON_INFO, "ACCEPT_SET - MOVE_SET or FAILOVER_SET not received yet - sleep\n");

						/* Rollback the transaction for now */
						(void) slon_mkquery(&query3, "rollback transaction");
						if (query_execute(node, local_dbconn, &query3) < 0)
							slon_retry();

						/* Sleep */
						if (sched_msleep(node, 10000) != SCHED_STATUS_OK)
							slon_retry();

						/* Start the transaction again */
						(void) slon_mkquery(&query3,
							"begin transaction; "
							"set transaction isolation level serializable; ");
						slon_appendquery(&query1,
							 "lock table %s.sl_config_lock; ",
							 rtcfg_namespace);
						if (query_execute(node, local_dbconn, &query3) < 0)
							slon_retry();

						/* See if we have the missing event now */
						res = PQexec(local_dbconn, dstring_data(&query2));
					}
					PQclear(res);
					slon_log(SLON_DEBUG1, "ACCEPT_SET - MOVE_SET or FAILOVER_SET exists - adjusting setsync status\n");

					/*
					 * Finalize the setsync status to mave the ACCEPT_SET's
					 * seqno and snapshot info.
					 */
					slon_appendquery(&query1,
									 "update %s.sl_setsync "
									 "    set ssy_origin = %d, "
									 "        ssy_seqno = '%s', "
									 "        ssy_snapshot = '%s', "
									 "        ssy_action_list = '' "
									 "    where ssy_setid = %d; ",
									 rtcfg_namespace,
									 new_origin,
									 seqbuf,
									 event->ev_snapshot_c,
									 set_id);

					/*
					 * Execute all queries and restart slon.
					 */
					slon_appendquery(&query1,
									 "notify \"_%s_Restart\"; ",
									 rtcfg_cluster_name);
					query_append_event(&query1, event);
					slon_appendquery(&query1, "commit transaction;");

					query_execute(node, local_dbconn, &query1);
					slon_log(SLON_DEBUG1, "ACCEPT_SET - done\n");
					archive_close(node);
					slon_retry();

					need_reloadListen = true;
				}
				else
				{
					slon_log(SLON_DEBUG1, "ACCEPT_SET - on origin node...\n");
				}

			}
			else if (strcmp(event->ev_type, "MOVE_SET") == 0)
			{
				int			set_id = (int) strtol(event->ev_data1, NULL, 10);
				int			old_origin = (int) strtol(event->ev_data2, NULL, 10);
				int			new_origin = (int) strtol(event->ev_data3, NULL, 10);
				int			sub_provider = -1;
				PGresult   *res;

				/*
				 * Move set is a little more tricky. The stored proc does a
				 * lot of rearranging in the provider chain. To catch up with
				 * that, we need to execute it now and select the resulting
				 * provider for us.
				 */

				slon_appendquery(&query1,
								 "select %s.moveSet_int(%d, %d, %d, %s); ",
								 rtcfg_namespace,
								 set_id, old_origin, new_origin, seqbuf);
				if (query_execute(node, local_dbconn, &query1) < 0)
					slon_retry();

				(void) slon_mkquery(&query1,
								  "select sub_provider from %s.sl_subscribe "
							   "	where sub_receiver = %d and sub_set = %d",
									rtcfg_namespace, rtcfg_nodeid, set_id);
				res = PQexec(local_dbconn, dstring_data(&query1));
				if (PQresultStatus(res) != PGRES_TUPLES_OK)
				{
					slon_log(SLON_FATAL, "remoteWorkerThread_%d: \"%s\" %s",
							 node->no_id, dstring_data(&query1),
							 PQresultErrorMessage(res));
					PQclear(res);
					slon_retry();
				}
				if (PQntuples(res) == 1)
				{
					sub_provider =
						(int) strtol(PQgetvalue(res, 0, 0), NULL, 10);
				}
				PQclear(res);

				/*
				 * Update the internal runtime configuration
				 */
				rtcfg_moveSet(set_id, old_origin, new_origin, sub_provider);

				dstring_reset(&query1);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "FAILOVER_SET") == 0)
			{
				int			failed_node = (int) strtol(event->ev_data1, NULL, 10);
				int			backup_node = (int) strtol(event->ev_data2, NULL, 10);
				int			set_id = (int) strtol(event->ev_data3, NULL, 10);

				rtcfg_storeSet(set_id, backup_node, NULL);

				slon_appendquery(&query1,
								 "lock table %s.sl_event_lock;"
								 "select %s.failoverSet_int(%d, %d, %d, %s); ",
								 rtcfg_namespace,
								 rtcfg_namespace,
								 failed_node, backup_node, set_id, seqbuf);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "SUBSCRIBE_SET") == 0)
			{
				int			sub_set = (int) strtol(event->ev_data1, NULL, 10);
				int			sub_provider = (int) strtol(event->ev_data2, NULL, 10);
				int			sub_receiver = (int) strtol(event->ev_data3, NULL, 10);
				char	   *sub_forward = event->ev_data4;
				char	   *omit_copy = event->ev_data5;

				if (sub_receiver == rtcfg_nodeid)
					rtcfg_storeSubscribe(sub_set, sub_provider, sub_forward);

				slon_appendquery(&query1,
								 "lock table %s.sl_event_lock;"
								 "select %s.subscribeSet_int(%d, %d, %d, '%q', '%q'); ",
								 rtcfg_namespace,
								 rtcfg_namespace,
								 sub_set, sub_provider, sub_receiver, sub_forward, omit_copy);
				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "ENABLE_SUBSCRIPTION") == 0)
			{
				int			sub_set = (int) strtol(event->ev_data1, NULL, 10);
				int			sub_provider = (int) strtol(event->ev_data2, NULL, 10);
				int			sub_receiver = (int) strtol(event->ev_data3, NULL, 10);
				char	   *sub_forward = event->ev_data4;
				int         copy_set_retries = 0;

				/*
				 * Do the actual enabling of the set only if we are the
				 * receiver.
				 */
				if (sub_receiver == rtcfg_nodeid &&
					event->ev_origin == node->no_id)
				{
					ScheduleStatus			sched_rc;
					int			sleeptime = 15;

					(void) slon_mkquery(&query2, "rollback transaction");
					check_config = true;
					
					slon_appendquery(&query1,
									 "lock table %s.sl_config_lock; ",
									 rtcfg_namespace);
					while (true)
					{
						/*
						 * If we received this event from another node than
						 * the data provider, wait until the data provider has
						 * synced up far enough.
						 */
						if (event->ev_origin != sub_provider &&
							event->event_provider != sub_provider)
						{
							int64		prov_seqno = get_last_forwarded_confirm(
															event->ev_origin,
															   sub_provider);

							if (prov_seqno < 0 || event->ev_seqno > prov_seqno)
							{
								slon_log(SLON_WARN, "remoteWorkerThread_%d: "
								   "copy set: data provider %d only on sync "
										 INT64_FORMAT
										 " - sleep 5 seconds\n",
										 node->no_id, sub_provider,
										 prov_seqno);

								/**
								 * Release the sl_config_lock
								 * we want other threads to be
								 * able to continue during the sleep.
								 */
								if (query_execute(node, local_dbconn, &query2) < 0)
									slon_retry();
								sched_rc = sched_msleep(node, 5000);
								if (sched_rc != SCHED_STATUS_OK)
								{
									event_ok = false;
									break;
								}
								/**
								 * Obtain the config lock again.
								 * it was released above.
								 */
								slon_mkquery(&query1, "start transaction;"
											 "set transaction isolation level serializable;");
								slon_appendquery(&query1,
												  "lock table %s.sl_config_lock; ",
												 rtcfg_namespace);

								if (query_execute(node, local_dbconn, &query1) < 0)
									slon_retry();

								continue;
							}
						}
					

						/* 
						 * if we have failed more than once we need to restart
						 * our transaction or we can end up with odd results
						 * in our subscription tables, and in 8.4+ LOCK
						 * TABLE requires you to be in a txn.
						 */
						if(copy_set_retries != 0)
						  {
							slon_mkquery(&query1, "start transaction;"
										 "set transaction isolation level serializable;");
							slon_appendquery(&query1,
											 "lock table %s.sl_config_lock; ",
											 rtcfg_namespace);

							if (query_execute(node, local_dbconn, &query1) < 0)
							  slon_retry();
						  }

						/*
						 * If the copy succeeds, exit the loop and let the
						 * transaction commit.
						 */
						if (copy_set(node, local_conn, sub_set, event) == 0)
						{
							rtcfg_enableSubscription(sub_set, sub_provider, sub_forward);
							dstring_reset(&query1);
							(void) slon_mkquery(&query1,
								"select %s.enableSubscription(%d, %d, %d); ",
												rtcfg_namespace,
										sub_set, sub_provider, sub_receiver);
							sched_rc = SCHED_STATUS_OK;
							copy_set_retries = 0;
							break;
						}

						copy_set_retries++;

						/*
						 * Data copy for new enabled set has failed. Rollback
						 * the transaction, sleep and try again.
						 */
						slon_log(SLON_WARN, "remoteWorkerThread_%d: "
								 "data copy for set %d failed %d times - "
								 "sleep %d seconds\n",
								 node->no_id, sub_set, copy_set_retries,
								 sleeptime);

						if (query_execute(node, local_dbconn, &query2) < 0)
							slon_retry();
						sched_rc = sched_msleep(node, sleeptime * 1000);
						if (sched_rc != SCHED_STATUS_OK)
						{
							event_ok = false;
							break;
						}
						if (sleeptime < 60)
							sleeptime *= 2;
					}
				}
				else
				{
					/*
					 * Somebody else got enabled, just remember it
					 */
					slon_appendquery(&query1,
								"select %s.enableSubscription(%d, %d, %d); ",
									 rtcfg_namespace,
									 sub_set, sub_provider, sub_receiver);
				}

				/*
				 * Note: No need to do anything based on archive_dir here;
				 * copy_set does that nicely already.
				 */
				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "UNSUBSCRIBE_SET") == 0)
			{
				int			sub_set = (int) strtol(event->ev_data1, NULL, 10);
				int			sub_receiver = (int) strtol(event->ev_data2, NULL, 10);

				/*
				 * All the real work is done when the config utility calls
				 * unsubscribeSet() itself. Just propagate the event here.
				 */
				slon_appendquery(&query1,
								 "select %s.unsubscribeSet_int(%d, %d); ",
								 rtcfg_namespace,
								 sub_set, sub_receiver);

				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "DDL_SCRIPT") == 0)
			{
				int			ddl_setid = (int) strtol(event->ev_data1, NULL, 10);
				char	   *ddl_script = event->ev_data2;
				int			ddl_only_on_node = (int) strtol(event->ev_data3, NULL, 10);
				int			num_statements = -1,
							stmtno;
				int			node_in_set;
				int			localNodeId;

				PGresult   *res;
				ExecStatusType rstat;

				/**
				 * Check to make sure this node is part of the set
				 */
				slon_log(SLON_INFO, "Checking local node id\n");
				localNodeId = db_getLocalNodeId(local_dbconn);
				slon_log(SLON_INFO, "Found local node id\n");
				node_in_set = check_set_subscriber(ddl_setid, localNodeId, local_dbconn);

				if (!node_in_set)
				{
					/**
					 *
					 * Node is not part of the set.
					 * Do not forward the DDL to the node,
					 * nor should it be included in the log for log-shipping.
					 */
					slon_log(SLON_INFO, "Not forwarding DDL to node %d for set %d\n",
							 node->no_id, ddl_setid);

				}
				else
				{

					slon_appendquery(&query1,
									 "set session_replication_role to local; "
									 "select %s.ddlScript_prepare_int(%d, %d); ",
									 rtcfg_namespace,
									 ddl_setid, ddl_only_on_node);

					if (query_execute(node, local_dbconn, &query1) < 0)
					{
						slon_log(SLON_ERROR, "remoteWorkerThread_%d: DDL preparation failed - set %d - only on node %d\n",
								 node->no_id, ddl_setid, ddl_only_on_node);
						slon_retry();
					}

					num_statements = scan_for_statements(ddl_script);
					slon_log(SLON_CONFIG, "remoteWorkerThread_%d: DDL request with %d statements\n",
							 node->no_id, num_statements);
					if ((num_statements < 0) || (num_statements >= MAXSTATEMENTS))
					{
						slon_log(SLON_ERROR, "remoteWorkerThread_%d: DDL had invalid number of statements - %d\n",
								 node->no_id, num_statements);
						slon_retry();
					}

					for (stmtno = 0; stmtno < num_statements; stmtno++)
					{
						int			startpos,
									endpos;
						char	   *dest;

						if (stmtno == 0)
							startpos = 0;
						else
							startpos = STMTS[stmtno - 1];

						endpos = STMTS[stmtno];
						dest = (char *) malloc(endpos - startpos + 1);
						if (dest == 0)
						{
							slon_log(SLON_ERROR, "remoteWorkerThread_%d: malloc() failure in DDL_SCRIPT - could not allocate %d bytes of memory\n",
									 node->no_id, endpos - startpos + 1);
							slon_retry();
						}
						strncpy(dest, ddl_script + startpos, endpos - startpos);
						dest[STMTS[stmtno] - startpos] = 0;
						(void) slon_mkquery(&query1, "%s", dest);
						slon_log(SLON_CONFIG, "remoteWorkerThread_%d: DDL Statement %d: [%s]\n",
								 node->no_id, stmtno, dest);
						free(dest);

						res = PQexec(local_dbconn, dstring_data(&query1));

						if (PQresultStatus(res) != PGRES_COMMAND_OK &&
							PQresultStatus(res) != PGRES_TUPLES_OK &&
							PQresultStatus(res) != PGRES_EMPTY_QUERY)
						{
							rstat = PQresultStatus(res);
							slon_log(SLON_ERROR, "DDL Statement failed - %s\n", PQresStatus(rstat));
							PQclear(res);
							dstring_free(&query1);
							slon_retry();
						}
						rstat = PQresultStatus(res);
						slon_log(SLON_CONFIG, "DDL success - %s\n", PQresStatus(rstat));
						PQclear(res);
					}

					(void) slon_mkquery(&query1,
								 "select %s.ddlScript_complete_int(%d, %d); "
								 "set session_replication_role to replica; ",
										rtcfg_namespace,
										ddl_setid,
										ddl_only_on_node);

					/*
					 * DDL_SCRIPT needs to be turned into a log shipping
					 * script
					 */

					/*
					 * Note that the issue about parsing that mandates
					 * breaking up compound statements into
					 * individually-processed statements does not apply to log
					 * shipping as psql parses and processes each statement
					 * individually
					 */

					if (archive_dir)
					{
						if ((ddl_only_on_node < 1) || (ddl_only_on_node == rtcfg_nodeid))
						{

							if (archive_append_str(node, "set session_replication_role to local;\n") < 0)
								slon_retry();
							if (archive_append_str(node, ddl_script) < 0)
								slon_retry();
							if (archive_append_str(node, "set session_replication_role to replica;\n") < 0)
								slon_retry();
						}
					}
				}
			}
			else if (strcmp(event->ev_type, "RESET_CONFIG") == 0)
			{
				int			reset_config_setid = (int) strtol(event->ev_data1, NULL, 10);
				int			reset_configonly_on_node = (int) strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1,
								 "select %s.updateReloid(%d, '%q', %d); ",
								 rtcfg_namespace,
							   reset_config_setid, reset_configonly_on_node);
			}
			else
			{
				printf("TODO: ********** remoteWorkerThread: node %d - EVENT %d," INT64_FORMAT " %s - unknown event type\n",
					   node->no_id, event->ev_origin, event->ev_seqno, event->ev_type);
			}

			/*
			 * All simple configuration events fall through here. Commit the
			 * transaction.
			 */
			if (event_ok)
			{
					query_append_event(&query1, event);
					slon_appendquery(&query1, "commit transaction;");
					if (archive_close(node) < 0)
							slon_retry();
			}
			else
			{
				(void) slon_mkquery(&query1, "rollback transaction;");
				archive_terminate(node);
			}
			monitor_state(conn_symname, node->no_id, local_conn->conn_pid, "thread main loop", event->ev_seqno, event->ev_type);
			if (query_execute(node, local_dbconn, &query1) < 0)
				slon_retry();

			if (need_reloadListen)
			{
				rtcfg_reloadListen(local_dbconn);
				need_reloadListen = false;
			}
		}

#ifdef SLON_MEMDEBUG
		memset(msg, 55, sizeof(SlonWorkMsg_event));
#endif
		free(msg);
	}

	/*
	 * Thread exit time has arrived. Disconnect from all data providers and
	 * free memory
	 */
	adjust_provider_info(node, wd, true);

	pthread_mutex_unlock(&(wd->workdata_lock));
	pthread_mutex_destroy(&(wd->workdata_lock));
	pthread_cond_destroy(&(wd->repldata_cond));
	pthread_cond_destroy(&(wd->linepool_cond));

	slon_disconnectdb(local_conn);
	dstring_free(&query1);
	dstring_free(&query2);
	dstring_free(&query3);
	free(wd->tab_fqname);
	free(wd->tab_forward);
#ifdef SLON_MEMDEBUG
	local_conn = NULL;
	memset(wd, 66, sizeof(WorkerGroupData));
#endif
	free(wd);

	slon_log(SLON_INFO,
			 "remoteWorkerThread_%d: thread done\n",
			 node->no_id);
	pthread_exit(NULL);
	return 0;
}


/* ----------
 * adjust_provider_info
 * ----------
 */
static void
adjust_provider_info(SlonNode *node, WorkerGroupData *wd, int cleanup)
{
	ProviderInfo *provider;
	ProviderInfo *provnext;
	ProviderSet *pset;
	SlonNode   *rtcfg_node;
	SlonSet    *rtcfg_set;
	int			i;

	slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
			 "update provider configuration\n",
			 node->no_id);

	/*
	 * The runtime configuration has changed.
	 */

	/*
	 * Step 1.
	 *
	 * Remove all sets from the providers.
	 */
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		/*
		 * We create a lock here and keep it until we made our final decision
		 * about what to do with the helper thread.
		 */
		pthread_mutex_lock(&(provider->helper_lock));

		while ((pset = provider->set_head) != NULL)
		{
			DLLIST_REMOVE(provider->set_head, provider->set_tail,
						  pset);
#ifdef SLON_MEMDEBUG
			memset(pset, 55, sizeof(ProviderSet));
#endif
			free(pset);
		}
	}

	/*
	 * Step 2.
	 *
	 * Add all currently replicated sets (back) to the providers adding new
	 * providers as necessary. This step is skippen in cleanup mode, causing
	 * all providers to become obsolete and thus the whole provider info
	 * extinct.
	 */
	if (!cleanup)
	{
		for (rtcfg_set = rtcfg_set_list_head; rtcfg_set;
			 rtcfg_set = rtcfg_set->next)
		{
			/*
			 * We're only interested in the set's that origin on the remote
			 * node this worker is replicating.
			 */
			if (rtcfg_set->set_origin != node->no_id)
				continue;

			if (rtcfg_set->sub_provider >= 0 &&
				rtcfg_set->sub_active)
			{
				/*
				 * We need to replicate this set. Find or add the provider to
				 * our in-thread data.
				 */
				for (provider = wd->provider_head; provider;
					 provider = provider->next)
				{
					if (provider->no_id == rtcfg_set->sub_provider)
						break;
				}
				if (provider == NULL)
				{
					/*
					 * No provider entry found. Create a new one.
					 */
					provider = (ProviderInfo *)
						malloc(sizeof(ProviderInfo));
					memset(provider, 0, sizeof(ProviderInfo));
					provider->no_id = rtcfg_set->sub_provider;
					provider->wd = wd;

					/*
					 * Also create a helper thread for this provider, which
					 * will actually run the log data selection for us.
					 */
					pthread_mutex_init(&(provider->helper_lock), NULL);
					pthread_mutex_lock(&(provider->helper_lock));
					pthread_cond_init(&(provider->helper_cond), NULL);
					dstring_init(&(provider->helper_query));
					provider->helper_status = SLON_WG_IDLE;
					if (pthread_create(&(provider->helper_thread), NULL,
									   sync_helper, (void *) provider) != 0)
					{
						slon_log(SLON_FATAL, "remoteWorkerThread_%d: ",
								 "pthread_create() - %s\n",
								 node->no_id, strerror(errno));
						slon_retry();
					}
					slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
							 "helper thread for provider %d created\n",
							 node->no_id, provider->no_id);

					/*
					 * Add more workgroup data lines to the pool.
					 */
					for (i = 0; i < SLON_WORKLINES_PER_HELPER; i++)
					{
						WorkerGroupLine *line;

						line = (WorkerGroupLine *) malloc(sizeof(WorkerGroupLine));
						memset(line, 0, sizeof(WorkerGroupLine));
						line->line_largemem = 0;
						dstring_init(&(line->data));
						dstring_init(&(line->log));
						DLLIST_ADD_TAIL(wd->linepool_head, wd->linepool_tail,
										line);
					}

					/*
					 * Add the provider to our work group
					 */
					DLLIST_ADD_TAIL(wd->provider_head, wd->provider_tail,
									provider);

					/*
					 * Copy the runtime configurations conninfo into the
					 * provider info.
					 */
					rtcfg_node = rtcfg_findNode(provider->no_id);
					if (rtcfg_node != NULL)
					{
						provider->pa_connretry = rtcfg_node->pa_connretry;
						if (rtcfg_node->pa_conninfo != NULL)
							provider->pa_conninfo =
								strdup(rtcfg_node->pa_conninfo);
					}
				}

				/*
				 * Add the set to the list of sets we get from this provider.
				 */
				pset = (ProviderSet *)
					malloc(sizeof(ProviderSet));
				memset(pset, 0, sizeof(ProviderSet));
				pset->set_id = rtcfg_set->set_id;
				pset->sub_forward = rtcfg_set->sub_forward;

				DLLIST_ADD_TAIL(provider->set_head,
								provider->set_tail, pset);

				slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
						 "added active set %d to provider %d\n",
						 node->no_id, pset->set_id, provider->no_id);
			}
		}
	}

	/*
	 * Step 3.
	 *
	 * Remove all providers that we don't need any more.
	 */
	for (provider = wd->provider_head; provider; provider = provnext)
	{
		SlonNode   *rtcfg_node;

		provnext = provider->next;

		/*
		 * If the list of currently replicated sets we receive from this
		 * provider is empty, we don't need to maintain a connection to it.
		 */
		if (provider->set_head == NULL)
		{
			/*
			 * Tell this helper thread to exit, join him and destroy thread
			 * related data.
			 */
			provider->helper_status = SLON_WG_EXIT;
			pthread_cond_signal(&(provider->helper_cond));
			pthread_mutex_unlock(&(provider->helper_lock));
			pthread_join(provider->helper_thread, NULL);
			pthread_cond_destroy(&(provider->helper_cond));
			pthread_mutex_destroy(&(provider->helper_lock));

			slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
					 "helper thread for provider %d terminated\n",
					 node->no_id, provider->no_id);

			/*
			 * Remove the line buffers we added for this helper.
			 */
			for (i = 0; i < SLON_WORKLINES_PER_HELPER; i++)
			{
				WorkerGroupLine *line;

				if ((line = wd->linepool_head) == NULL)
					break;
				dstring_free(&(line->data));
				dstring_free(&(line->log));
				DLLIST_REMOVE(wd->linepool_head, wd->linepool_tail,
							  line);
#ifdef SLON_MEMDEBUG
				memset(line, 55, sizeof(WorkerGroupLine));
#endif
				free(line);
			}

			/*
			 * Disconnect from the database.
			 */
			if (provider->conn != NULL)
			{
				slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
						 "disconnecting from data provider %d\n",
						 node->no_id, provider->no_id);
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
			}

			/*
			 * Free other resources
			 */
			if (provider->pa_conninfo != NULL)
				free(provider->pa_conninfo);
			provider->pa_conninfo = NULL;
			DLLIST_REMOVE(wd->provider_head, wd->provider_tail, provider);
			dstring_free(&(provider->helper_query));
#ifdef SLON_MEMDEBUG
			memset(provider, 55, sizeof(ProviderInfo));
#endif
			free(provider);

			continue;
		}

		/*
		 * If the connection info has changed, we have to reconnect.
		 */
		rtcfg_node = rtcfg_findNode(provider->no_id);
		if (rtcfg_node == NULL || rtcfg_node->pa_conninfo == NULL ||
			provider->pa_conninfo == NULL ||
			strcmp(provider->pa_conninfo, rtcfg_node->pa_conninfo) != 0)
		{
			if (provider->conn != NULL)
			{
				slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
						 "disconnecting from data provider %d\n",
						 node->no_id, provider->no_id);
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
			}
			if (provider->pa_conninfo != NULL)
				free(provider->pa_conninfo);
			if ((rtcfg_node == NULL) || (rtcfg_node->pa_conninfo == NULL))
				provider->pa_conninfo = NULL;
			else
				provider->pa_conninfo = strdup(rtcfg_node->pa_conninfo);
		}

		/*
		 * Unlock the helper thread ... he should now go and wait for work.
		 */
		pthread_mutex_unlock(&(provider->helper_lock));
	}
}


/* ----------
 * remoteWorker_event
 *
 * Used by the remoteListeThread to forward events selected from the event
 * provider database to the remote nodes worker thread.
 * ----------
 */
void
remoteWorker_event(int event_provider,
				   int ev_origin, int64 ev_seqno,
				   char *ev_timestamp,
				   char *ev_snapshot, char *ev_mintxid, char *ev_maxtxid,
				   char *ev_type,
				   char *ev_data1, char *ev_data2,
				   char *ev_data3, char *ev_data4,
				   char *ev_data5, char *ev_data6,
				   char *ev_data7, char *ev_data8)
{
	SlonNode   *node;
	SlonWorkMsg_event *msg;
	int			len;
	char	   *cp;
	int			len_timestamp;
	int			len_snapshot;
	int			len_mintxid;
	int			len_maxtxid;
	int			len_type;
	int			len_data1 = 0;
	int			len_data2 = 0;
	int			len_data3 = 0;
	int			len_data4 = 0;
	int			len_data5 = 0;
	int			len_data6 = 0;
	int			len_data7 = 0;
	int			len_data8 = 0;

	/*
	 * Stop forwarding events if the replication engine is shutting down
	 */
	if (sched_get_status() != SCHED_STATUS_OK)
	{
		slon_log(SLON_DEBUG1,
				 "remoteWorker_event: ignore new events due to shutdown\n");
		return;
	}

	/*
	 * Find the node, make sure it is active and that this event is not
	 * already queued or processed.
	 */
	rtcfg_lock();
	node = rtcfg_findNode(ev_origin);
	if (node == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWorker_event: event %d," INT64_FORMAT
				 " ignored - unknown origin\n",
				 ev_origin, ev_seqno);
		return;
	}
	if (!node->no_active)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWorker_event: event %d," INT64_FORMAT
				 " ignored - origin inactive\n",
				 ev_origin, ev_seqno);
		return;
	}
	if (node->last_event >= ev_seqno)
	{
		rtcfg_unlock();
		slon_log(SLON_DEBUG2,
				 "remoteWorker_event: event %d," INT64_FORMAT
				 " ignored - duplicate\n",
				 ev_origin, ev_seqno);
		return;
	}

	/*
	 * We lock the worker threads message queue before bumping the nodes last
	 * known event sequence to avoid that another listener queues a later
	 * message before we can insert this one.
	 */
	pthread_mutex_lock(&(node->message_lock));
	node->last_event = ev_seqno;
	rtcfg_unlock();

	/*
	 * Compute the message length and allocate memory. The allocated memory
	 * only needs to be zero-initialized in the structure size. The following
	 * additional space for the event payload data is overwritten completely
	 * anyway.
	 */
	len = offsetof(SlonWorkMsg_event, raw_data)
		+(len_timestamp = strlen(ev_timestamp) + 1)
		+ (len_snapshot = strlen(ev_snapshot) + 1)
		+ (len_mintxid = strlen(ev_mintxid) + 1)
		+ (len_maxtxid = strlen(ev_maxtxid) + 1)
		+ (len_type = strlen(ev_type) + 1)
		+ ((ev_data1 == NULL) ? 0 : (len_data1 = strlen(ev_data1) + 1))
		+ ((ev_data2 == NULL) ? 0 : (len_data2 = strlen(ev_data2) + 1))
		+ ((ev_data3 == NULL) ? 0 : (len_data3 = strlen(ev_data3) + 1))
		+ ((ev_data4 == NULL) ? 0 : (len_data4 = strlen(ev_data4) + 1))
		+ ((ev_data5 == NULL) ? 0 : (len_data5 = strlen(ev_data5) + 1))
		+ ((ev_data6 == NULL) ? 0 : (len_data6 = strlen(ev_data6) + 1))
		+ ((ev_data7 == NULL) ? 0 : (len_data7 = strlen(ev_data7) + 1))
		+ ((ev_data8 == NULL) ? 0 : (len_data8 = strlen(ev_data8) + 1));

	msg = (SlonWorkMsg_event *) malloc(len);
	if (msg == NULL)
	{
		perror("remoteWorker_event: malloc()");
		slon_retry();
	}
	memset(msg, 0, sizeof(SlonWorkMsg_event));

	/*
	 * Copy all data into the message.
	 */
	cp = &(msg->raw_data[0]);
	msg->msg_type = WMSG_EVENT;
	msg->event_provider = event_provider;
	msg->ev_origin = ev_origin;
	msg->ev_seqno = ev_seqno;
	msg->ev_timestamp_c = cp;
	strcpy(cp, ev_timestamp);
	cp += len_timestamp;
	msg->ev_snapshot_c = cp;
	strcpy(cp, ev_snapshot);
	cp += len_snapshot;
	msg->ev_mintxid_c = cp;
	strcpy(cp, ev_mintxid);
	cp += len_mintxid;
	msg->ev_maxtxid_c = cp;
	strcpy(cp, ev_maxtxid);
	cp += len_maxtxid;
	msg->ev_type = cp;
	strcpy(cp, ev_type);
	cp += len_type;
	if (ev_data1 != NULL)
	{
		msg->ev_data1 = cp;
		strcpy(cp, ev_data1);
		cp += len_data1;
	}
	if (ev_data2 != NULL)
	{
		msg->ev_data2 = cp;
		strcpy(cp, ev_data2);
		cp += len_data2;
	}
	if (ev_data3 != NULL)
	{
		msg->ev_data3 = cp;
		strcpy(cp, ev_data3);
		cp += len_data3;
	}
	if (ev_data4 != NULL)
	{
		msg->ev_data4 = cp;
		strcpy(cp, ev_data4);
		cp += len_data4;
	}
	if (ev_data5 != NULL)
	{
		msg->ev_data5 = cp;
		strcpy(cp, ev_data5);
		cp += len_data5;
	}
	if (ev_data6 != NULL)
	{
		msg->ev_data6 = cp;
		strcpy(cp, ev_data6);
		cp += len_data6;
	}
	if (ev_data7 != NULL)
	{
		msg->ev_data7 = cp;
		strcpy(cp, ev_data7);
		cp += len_data7;
	}
	if (ev_data8 != NULL)
	{
		msg->ev_data8 = cp;
		strcpy(cp, ev_data8);
		cp += len_data8;
	}

	/*
	 * Add the message to the queue and trigger the condition variable in case
	 * the worker is idle.
	 */
	DLLIST_ADD_TAIL(node->message_head, node->message_tail,
					(SlonWorkMsg *) msg);
	pthread_cond_signal(&(node->message_cond));
	pthread_mutex_unlock(&(node->message_lock));
}


/* ----------
 * remoteWorker_wakeup
 *
 * Send a special WAKEUP message to a worker, causing it to recheck the runmode
 * and the runtime configuration.
 * ----------
 */
void
remoteWorker_wakeup(int no_id)
{
	SlonNode   *node;
	SlonWorkMsg *msg;

	/*
	 * Can't wakeup myself, can I? No, we never have a "remote" worker for our
	 * own node ID.
	 */
	if (no_id == rtcfg_nodeid)
		return;

	rtcfg_lock();
	node = rtcfg_findNode(no_id);
	if (node == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_DEBUG1,
				 "remoteWorker_wakeup: unknown node %d\n",
				 no_id);
		return;
	}
	if (node->worker_status == SLON_TSTAT_NONE)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWorker_wakeup: node %d - no worker thread\n",
				 no_id);
		return;
	}
	rtcfg_unlock();

	msg = (SlonWorkMsg *) malloc(sizeof(SlonWorkMsg));
	msg->msg_type = WMSG_WAKEUP;

	pthread_mutex_lock(&(node->message_lock));
	DLLIST_ADD_TAIL(node->message_head, node->message_tail, msg);
	pthread_cond_signal(&(node->message_cond));
	pthread_mutex_unlock(&(node->message_lock));
}


/* ----------
 * remoteWorker_confirm
 *
 * Add a confirm message to the remote worker message queue
 * ----------
 */
void
remoteWorker_confirm(int no_id,
					 char *con_origin_c, char *con_received_c,
					 char *con_seqno_c, char *con_timestamp_c)
{
	SlonNode   *node;
	SlonWorkMsg_confirm *msg;
	SlonWorkMsg_confirm *oldmsg;
	int			con_origin;
	int			con_received;
	int64		con_seqno;

	con_origin = strtol(con_origin_c, NULL, 10);
	con_received = strtol(con_received_c, NULL, 10);
	slon_scanint64(con_seqno_c, &con_seqno);

	/*
	 * Check that the node exists and that we have a worker thread.
	 */
	rtcfg_lock();
	node = rtcfg_findNode(no_id);
	if (node == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_DEBUG2,
				 "remoteWorker_confirm: unknown node %d\n",
				 no_id);
		return;
	}
	if (node->worker_status == SLON_TSTAT_NONE)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWorker_wakeup: node %d - no worker thread\n",
				 no_id);
		return;
	}
	rtcfg_unlock();


	/*
	 * Lock the message queue
	 */
	pthread_mutex_lock(&(node->message_lock));

	/*
	 * Look if we already have a confirm message for this origin+received node
	 * pair.
	 */
	for (oldmsg = (SlonWorkMsg_confirm *) (node->message_head);
		 oldmsg; oldmsg = (SlonWorkMsg_confirm *) (oldmsg->next))
	{
		if (oldmsg->msg_type == WMSG_CONFIRM)
		{
			if (oldmsg->con_origin == con_origin &&
				oldmsg->con_received == con_received)
			{
				/*
				 * Existing message found. Change it if new seqno is greater
				 * than old. Otherwise just ignore this confirm.
				 */
				if (oldmsg->con_seqno < con_seqno)
				{
					oldmsg->con_seqno = con_seqno;
					strcpy(oldmsg->con_timestamp_c, con_timestamp_c);

					/*
					 * Move the message to the head of the queue
					 */
					if ((SlonWorkMsg *) oldmsg != node->message_head)
					{
						DLLIST_REMOVE(node->message_head,
									  node->message_tail, (SlonWorkMsg *) oldmsg);
						DLLIST_ADD_HEAD(node->message_head,
								node->message_tail, (SlonWorkMsg *) oldmsg);
					}
				}
				pthread_mutex_unlock(&(node->message_lock));
				return;
			}
		}
	}

	/*
	 * No message found. Create a new one and stick it in front of the queue.
	 */
	msg = (SlonWorkMsg_confirm *)
		malloc(sizeof(SlonWorkMsg_confirm));
	msg->msg_type = WMSG_CONFIRM;

	msg->con_origin = con_origin;
	msg->con_received = con_received;
	msg->con_seqno = con_seqno;
	strcpy(msg->con_timestamp_c, con_timestamp_c);

	DLLIST_ADD_HEAD(node->message_head, node->message_tail,
					(SlonWorkMsg *) msg);

	/*
	 * Send a condition signal to the worker thread in case it is waiting for
	 * new messages.
	 */
	pthread_cond_signal(&(node->message_cond));
	pthread_mutex_unlock(&(node->message_lock));
}


/* ----------
 * query_execute
 *
 * Execute a query string that does not return a result set.
 * ----------
 */
static int
query_execute(SlonNode *node, PGconn *dbconn, SlonDString *dsp)

{
	PGresult   *res;
	int			rc;

	res = PQexec(dbconn, dstring_data(dsp));
	rc = PQresultStatus(res);
	if (rc != PGRES_COMMAND_OK && rc != PGRES_TUPLES_OK &&
		rc != PGRES_EMPTY_QUERY)
	{
		slon_log(SLON_ERROR,
				 "remoteWorkerThread_%d: \"%s\" %s %s",
				 node->no_id, dstring_data(dsp),
				 PQresStatus(rc),
				 PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);
	return 0;
}


/* ----------
 * query_append_event
 *
 * Add queries to a dstring that insert a duplicate of an event record
 * as well as the confirmation for it.  
 * ----------
 */
static void
query_append_event(SlonDString *dsp, SlonWorkMsg_event *event)
{
	char		seqbuf[64];

	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);
	slon_appendquery(dsp,
					 "insert into %s.sl_event "
					 "    (ev_origin, ev_seqno, ev_timestamp, "
					 "     ev_snapshot, ev_type ",
					 rtcfg_namespace);
	if (event->ev_data1 != NULL)
		dstring_append(dsp, ", ev_data1");
	if (event->ev_data2 != NULL)
		dstring_append(dsp, ", ev_data2");
	if (event->ev_data3 != NULL)
		dstring_append(dsp, ", ev_data3");
	if (event->ev_data4 != NULL)
		dstring_append(dsp, ", ev_data4");
	if (event->ev_data5 != NULL)
		dstring_append(dsp, ", ev_data5");
	if (event->ev_data6 != NULL)
		dstring_append(dsp, ", ev_data6");
	if (event->ev_data7 != NULL)
		dstring_append(dsp, ", ev_data7");
	if (event->ev_data8 != NULL)
		dstring_append(dsp, ", ev_data8");
	slon_appendquery(dsp,
					 "    ) values ('%d', '%s', '%s', '%s', '%s'",
					 event->ev_origin, seqbuf, event->ev_timestamp_c,
					 event->ev_snapshot_c,
					 event->ev_type);
	if (event->ev_data1 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data1);
	if (event->ev_data2 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data2);
	if (event->ev_data3 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data3);
	if (event->ev_data4 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data4);
	if (event->ev_data5 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data5);
	if (event->ev_data6 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data6);
	if (event->ev_data7 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data7);
	if (event->ev_data8 != NULL)
		slon_appendquery(dsp, ", '%q'", event->ev_data8);
	slon_appendquery(dsp,
					 "); "
					 "insert into %s.sl_confirm "
					 "	(con_origin, con_received, con_seqno, con_timestamp) "
					 "   values (%d, %d, '%s', now()); ",
					 rtcfg_namespace,
					 event->ev_origin, rtcfg_nodeid, seqbuf);
}


/* ----------
 * store_confirm_forward
 *
 * Call the forwardConfirm() stored procedure.
 * ----------
 */
static void
store_confirm_forward(SlonNode *node, SlonConn *conn,
					  SlonWorkMsg_confirm *confirm)
{
	SlonDString query;
	PGresult   *res;
	char		seqbuf[64];
	struct node_confirm_status *cstat;
	int			cstat_found = false;

	/*
	 * Check the global confirm status if we already know about this
	 * confirmation.
	 */
	pthread_mutex_lock(&node_confirm_lock);
	for (cstat = node_confirm_head; cstat; cstat = cstat->next)
	{
		if (cstat->con_origin == confirm->con_origin &&
			cstat->con_received == confirm->con_received)
		{
			/*
			 * origin+received pair record found.
			 */
			if (cstat->con_seqno >= confirm->con_seqno)
			{
				/*
				 * Confirm status is newer or equal, ignore message.
				 */
				pthread_mutex_unlock(&node_confirm_lock);
				return;
			}

			/*
			 * Set the confirm status to the new seqno and continue below.
			 */
			cstat_found = true;
			cstat->con_seqno = confirm->con_seqno;
			break;
		}
	}

	/*
	 * If there was no such confirm status entry, add a new one.
	 */
	if (!cstat_found)
	{
		cstat = (struct node_confirm_status *)
			malloc(sizeof(struct node_confirm_status));
		cstat->con_origin = confirm->con_origin;
		cstat->con_received = confirm->con_received;
		cstat->con_seqno = confirm->con_seqno;
		DLLIST_ADD_TAIL(node_confirm_head, node_confirm_tail, cstat);
	}
	pthread_mutex_unlock(&node_confirm_lock);

	/*
	 * Call the stored procedure to forward this status through the table
	 * sl_confirm.
	 */
	dstring_init(&query);
	sprintf(seqbuf, INT64_FORMAT, confirm->con_seqno);

	slon_log(SLON_DEBUG2,
			 "remoteWorkerThread_%d: forward confirm %d,%s received by %d\n",
			 node->no_id, confirm->con_origin, seqbuf, confirm->con_received);

	(void) slon_mkquery(&query,
						"select %s.forwardConfirm(%d, %d, '%s', '%q'); ",
						rtcfg_namespace,
						confirm->con_origin, confirm->con_received,
						seqbuf, confirm->con_timestamp_c);

	res = PQexec(conn->dbconn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR,
				 "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query),
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		return;
	}
	PQclear(res);
	dstring_free(&query);
	return;
}


/* ----------
 * get_last_forwarded_confirm
 *
 * Look what confirmed event seqno we forwarded last for a given origin+receiver
 * pair.
 * ----------
 */
static int64
get_last_forwarded_confirm(int origin, int receiver)
{
	struct node_confirm_status *cstat;

	/*
	 * Check the global confirm status if we already know about this
	 * confirmation.
	 */
	pthread_mutex_lock(&node_confirm_lock);
	for (cstat = node_confirm_head; cstat; cstat = cstat->next)
	{
		if (cstat->con_origin == origin &&
			cstat->con_received == receiver)
		{
			/*
			 * origin+received pair record found.
			 */
			pthread_mutex_unlock(&node_confirm_lock);
			return cstat->con_seqno;
		}
	}
	pthread_mutex_unlock(&node_confirm_lock);

	return -1;
}


/* ----------
 * copy_set
 * ----------
 */
static int
copy_set(SlonNode *node, SlonConn *local_conn, int set_id,
		 SlonWorkMsg_event *event)
{
	SlonSet    *set;
	SlonConn   *pro_conn;
	PGconn	   *pro_dbconn;
	PGconn	   *loc_dbconn;
	char	   *conninfo;
	char		conn_symname[64];
	SlonDString query1;
	SlonDString query2;
	SlonDString query3;
	SlonDString lsquery;
	SlonDString indexregenquery;
	int			ntuples1;
	int			tupno1;
	PGresult   *res1;
	PGresult   *res2;
	PGresult   *res3;
	int			rc;
	int			set_origin = 0;
	SlonNode   *sub_node;
	int			sub_provider = 0;
	char	   *ssy_seqno = NULL;
	char	   *ssy_snapshot = NULL;
	SlonDString ssy_action_list;
	char		seqbuf[64];
	char	   *copydata = NULL;
	bool omit_copy = false;
	char *v_omit_copy = event->ev_data5;
	struct timeval tv_start;
	struct timeval tv_start2;
	struct timeval tv_now;

	gettimeofday(&tv_start, NULL);

	if (strcmp(v_omit_copy, "f") == 0) {
		omit_copy = false;
	} else {
		if (strcmp(v_omit_copy, "t") == 0) {
			omit_copy = true;
		} else {
			slon_log(SLON_ERROR, "copy_set %d - omit_copy not in (t,f)- [%s]\n", set_id, v_omit_copy);
		}
	}
	slon_log(SLON_INFO, "copy_set %d - omit=%s - bool=%d\n", set_id, v_omit_copy, omit_copy);

	if (omit_copy) {
		slon_log(SLON_INFO, "omit is TRUE\n");
	} else {
		slon_log(SLON_INFO, "omit is FALSE\n");
	}

	/*
	 * Lookup the provider nodes conninfo
	 */
	rtcfg_lock();
	for (set = rtcfg_set_list_head; set; set = set->next)
	{
		if (set->set_id == set_id)
		{
			set_origin = set->set_origin;
			sub_provider = set->sub_provider;
			break;
		}
	}
	if (sub_provider < 0) {
		rtcfg_unlock();
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: provider node %d for set %d"
			 "not found in runtime configuration\n",
			 node->no_id,
			 sub_provider,
			 set_id);
		slon_retry();
		return -1;
		
	}
	if (set_origin < 0) {
		rtcfg_unlock();
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: origin node %d for set %d "
			 "not found in runtime configuration\n",
			 node->no_id, 
			 set_origin,
			 set_id);
		slon_retry();
		return -1;
	}
	if (set == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: set %d "
				 "not found in runtime configuration\n",
				 node->no_id, set_id);
		return -1;
	}
	if ((sub_node = rtcfg_findNode(sub_provider)) == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: provider node %d "
				 "not found in runtime configuration\n",
				 node->no_id, sub_provider);
		return -1;
	}
	if (sub_node->pa_conninfo == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: node %d "
				 "has no pa_conninfo\n",
				 node->no_id, sub_provider);
		return -1;
	}
	conninfo = strdup(sub_node->pa_conninfo);
	rtcfg_unlock();

	/*
	 * Connect to the provider DB
	 */
	sprintf(conn_symname, "copy_set_%d", set_id);
	if ((pro_conn = slon_connectdb(conninfo, conn_symname)) == NULL)
	{
		free(conninfo);
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "copy set %d - cannot connect to provider DB node %d\n",
				 node->no_id, set_id, sub_node->no_id);
		return -1;
	}
	free(conninfo);
	conninfo = NULL;
	slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
			 "connected to provider DB\n",
			 node->no_id);

	pro_dbconn = pro_conn->dbconn;
	loc_dbconn = local_conn->dbconn;
	dstring_init(&query1);
	dstring_init(&query2);
	dstring_init(&query3);
	dstring_init(&lsquery);
	dstring_init(&indexregenquery);
	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);

	/*
	 * Register this connection in sl_nodelock
	 */
	(void) slon_mkquery(&query1,
						"select %s.registerNodeConnection(%d); ",
						rtcfg_namespace, rtcfg_nodeid);
	if (query_execute(node, pro_dbconn, &query1) < 0)
	{
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}

	/*
	 * Begin a serialized transaction and verify that the event's snapshot
	 * xxid is less than the present snapshot. This ensures that all
	 * transactions that have been in progress when the subscription got
	 * enabled (which is after the triggers on the tables have been defined),
	 * have finished. Otherwise a long running open transaction would not have
	 * the trigger definitions yet, and an insert would not get logged. But if
	 * it still runs when we start to copy the set, then we don't see the row
	 * either and it would get lost.
	 */
	if (sub_provider == set_origin)
	{
		(void) slon_mkquery(&query1,
							"start transaction; "
							"set transaction isolation level serializable; "
							"select \"pg_catalog\".txid_snapshot_xmin(\"pg_catalog\".txid_current_snapshot()) <= '%s'; ",
							event->ev_maxtxid_c);
		res1 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (*(PQgetvalue(res1, 0, 0)) == 't')
		{
			slon_log(SLON_WARN, "remoteWorkerThread_%d: "
				  "transactions earlier than XID %s are still in progress\n",
					 node->no_id, event->ev_maxtxid_c);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		PQclear(res1);
	}
	else
	{
		(void) slon_mkquery(&query1,
							"start transaction; "
							"set transaction isolation level serializable; ");
		if (query_execute(node, pro_dbconn, &query1) < 0)
		{
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
	}

	/*
	 * check tables/sequences in set to make sure they are there and in good
	 * order.  Don't copy any data yet; we want to just do a first pass that
	 * finds "bozo errors"
	 */

	/*
	 * Check tables and sequences in set to make sure they are all
	 * appropriately configured...
	 */

	/*
	 * Select the list of all tables the provider currently has in the set.
	 */
	(void) slon_mkquery(&query1,
						"select T.tab_id, "
						"    %s.slon_quote_brute(PGN.nspname) || '.' || "
						"    %s.slon_quote_brute(PGC.relname) as tab_fqname, "
						"    T.tab_idxname, T.tab_comment "
						"from %s.sl_table T, "
						"    \"pg_catalog\".pg_class PGC, "
						"    \"pg_catalog\".pg_namespace PGN "
						"where T.tab_set = %d "
						"    and T.tab_reloid = PGC.oid "
						"    and PGC.relnamespace = PGN.oid "
						"order by tab_id; ",
						rtcfg_namespace,
						rtcfg_namespace,
						rtcfg_namespace,
						set_id);
	res1 = PQexec(pro_dbconn, dstring_data(&query1));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	ntuples1 = PQntuples(res1);

	/*
	 * For each table in the set
	 */
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		char	   *tab_fqname = PQgetvalue(res1, tupno1, 1);

		gettimeofday(&tv_start2, NULL);
		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "prepare to copy table %s\n",
				 node->no_id, tab_fqname);

		(void) slon_mkquery(&query3, "select * from %s limit 0;",
							tab_fqname);
		res2 = PQexec(loc_dbconn, dstring_data(&query3));
		if (PQresultStatus(res2) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: Could not find table %s "
					 "on subscriber\n", node->no_id, tab_fqname);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query3);
			archive_terminate(node);
			return -1;
		}
		PQclear(res2);

		/*
		 * Request an exclusive lock on each table
		 *
		 * We do this immediately so that we don't get stuck later if
		 * something else had been holding onto one or another table.
		 */

		(void) slon_mkquery(&query3, "lock table %s;\n", tab_fqname);
		res2 = PQexec(loc_dbconn, dstring_data(&query3));
		if (PQresultStatus(res2) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: Could not lock table %s "
					 "on subscriber\n", node->no_id, tab_fqname);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query3);
			archive_terminate(node);
			return -1;
		}
		PQclear(res2);
	}
	PQclear(res1);

	slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
			 "all tables for set %d found on subscriber\n",
			 node->no_id, set_id);

	/*
	 * Add in the sequences contained in the set
	 */
	(void) slon_mkquery(&query1,
						"select SQ.seq_id, "
						"    %s.slon_quote_brute(PGN.nspname) || '.' || "
						"    %s.slon_quote_brute(PGC.relname) as tab_fqname, "
						"		SQ.seq_comment "
						"	from %s.sl_sequence SQ, "
						"		\"pg_catalog\".pg_class PGC, "
						"		\"pg_catalog\".pg_namespace PGN "
						"	where SQ.seq_set = %d "
						"		and PGC.oid = SQ.seq_reloid "
						"		and PGN.oid = PGC.relnamespace; ",
						rtcfg_namespace,
						rtcfg_namespace,
						rtcfg_namespace,
						set_id);
	res1 = PQexec(pro_dbconn, dstring_data(&query1));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	ntuples1 = PQntuples(res1);
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		char	   *seq_id = PQgetvalue(res1, tupno1, 0);
		char	   *seq_fqname = PQgetvalue(res1, tupno1, 1);
		char	   *seq_comment = PQgetvalue(res1, tupno1, 2);

		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "copy sequence %s\n",
				 node->no_id, seq_fqname);

		(void) slon_mkquery(&query1,
						  "select %s.setAddSequence_int(%d, %s, '%q', '%q')",
							rtcfg_namespace, set_id, seq_id,
							seq_fqname, seq_comment);
		if (query_execute(node, loc_dbconn, &query1) < 0)
		{
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
	}
	PQclear(res1);


	/*
	 * Select the list of all tables the provider currently has in the set.
	 */
	(void) slon_mkquery(&query1,
						"select T.tab_id, "
						"    %s.slon_quote_brute(PGN.nspname) || '.' || "
						"    %s.slon_quote_brute(PGC.relname) as tab_fqname, "
						"    T.tab_idxname, T.tab_comment "
						"from %s.sl_table T, "
						"    \"pg_catalog\".pg_class PGC, "
						"    \"pg_catalog\".pg_namespace PGN "
						"where T.tab_set = %d "
						"    and T.tab_reloid = PGC.oid "
						"    and PGC.relnamespace = PGN.oid "
						"order by tab_id; ",
						rtcfg_namespace,
						rtcfg_namespace,
						rtcfg_namespace,
						set_id);
	res1 = PQexec(pro_dbconn, dstring_data(&query1));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	ntuples1 = PQntuples(res1);

	/*
	 * For each table in the set
	 */
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		int			tab_id = strtol(PQgetvalue(res1, tupno1, 0), NULL, 10);
		char	   *tab_fqname = PQgetvalue(res1, tupno1, 1);
		char	   *tab_idxname = PQgetvalue(res1, tupno1, 2);
		char	   *tab_comment = PQgetvalue(res1, tupno1, 3);
		int64		copysize = 0;

		gettimeofday(&tv_start2, NULL);
		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "copy table %s\n",
				 node->no_id, tab_fqname);

		/*
		 * Call the setAddTable_int() stored procedure. Up to now, while we
		 * have not been subscribed to the set, this should have been
		 * suppressed.
		 */
		(void) slon_mkquery(&query1,
					 "select %s.setAddTable_int(%d, %d, '%q', '%q', '%q'); ",
							rtcfg_namespace,
					   set_id, tab_id, tab_fqname, tab_idxname, tab_comment);
		if (query_execute(node, loc_dbconn, &query1) < 0)
		{
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}

		/*
		 * Begin a COPY from stdin for the table on the local DB
		 */
		if (omit_copy) {
			slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
					 "COPY of table %s suppressed due to OMIT COPY option\n",
					 node->no_id, tab_fqname);
		} else {
		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "Begin COPY of table %s\n",
				 node->no_id, tab_fqname);

		(void) slon_mkquery(&query2, "select %s.copyFields(%d);",
							rtcfg_namespace, tab_id);

		res3 = PQexec(pro_dbconn, dstring_data(&query2));

		if (PQresultStatus(res3) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s\n",
					 node->no_id, dstring_data(&query2),
					 PQresultErrorMessage(res3));
			PQclear(res3);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}

		(void) slon_mkquery(&query1,
							"select %s.prepareTableForCopy(%d); "
							"copy %s %s from stdin; ",
							rtcfg_namespace,
							tab_id, tab_fqname,
							PQgetvalue(res3, 0, 0)
			);
		res2 = PQexec(loc_dbconn, dstring_data(&query1));
		if (PQresultStatus(res2) != PGRES_COPY_IN)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s %s\n",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res2),
					 PQerrorMessage(loc_dbconn));
			PQclear(res3);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (archive_dir)
		{
			(void) slon_mkquery(&query1,
			"delete from %s;\ncopy %s %s from stdin;", tab_fqname, tab_fqname,
								PQgetvalue(res3, 0, 0));
			rc = archive_append_ds(node, &query1);
			if (rc < 0)
			{
				PQclear(res3);
				PQclear(res2);
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
		}

		/*
		 * Begin a COPY to stdout for the table on the provider DB
		 */
		(void) slon_mkquery(&query1,
			   "copy %s %s to stdout; ", tab_fqname, PQgetvalue(res3, 0, 0));
		PQclear(res3);
		res3 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res3) != PGRES_COPY_OUT)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s %s\n",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res2),
					 PQerrorMessage(pro_dbconn));
			PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation failed");
			PQclear(res3);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}

		/*
		 * Copy the data over
		 */
		while ((rc = PQgetCopyData(pro_dbconn, &copydata, 0)) > 0)
		{
			int			len = strlen(copydata);

			copysize += (int64) len;
			if (PQputCopyData(loc_dbconn, copydata, len) != 1)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "PQputCopyData() - %s",
						 node->no_id, PQerrorMessage(loc_dbconn));
#ifdef SLON_MEMDEBUG
				memset(copydata, 88, len);
#endif
				PQfreemem(copydata);
				PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation failed");
				PQclear(res3);
				PQclear(res2);
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
			if (archive_dir)
			{
				rc = archive_append_data(node, copydata, len);
				if (rc < 0)
				{
#ifdef SLON_MEMDEBUG
					memset(copydata, 88, len);
#endif
					PQfreemem(copydata);
					PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation");
					PQclear(res3);
					PQclear(res2);
					PQclear(res1);
					slon_disconnectdb(pro_conn);
					dstring_free(&query1);
					dstring_free(&query2);
					dstring_free(&query3);
					dstring_free(&lsquery);
					dstring_free(&indexregenquery);
					archive_terminate(node);
					return -1;

				}
			}
#ifdef SLON_MEMDEBUG
			memset(copydata, 88, len);
#endif
			PQfreemem(copydata);
		}
		if (rc != -1)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "PGgetCopyData() %s",
					 node->no_id, PQerrorMessage(pro_dbconn));
			PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation failed");
			PQclear(res3);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}

		/*
		 * Check that the COPY to stdout on the provider node finished
		 * successful.
		 */
		PQclear(res3);
		res3 = PQgetResult(pro_dbconn);
		if (PQresultStatus(res3) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "copy to stdout on provider - %s %s",
					 node->no_id, PQresStatus(PQresultStatus(res3)),
					 PQresultErrorMessage(res3));
			PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation failed");
			PQclear(res3);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		PQclear(res3);

		/*
		 * End the COPY from stdin on the local node with success
		 */
		if (PQputCopyEnd(loc_dbconn, NULL) != 1)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "PGputCopyEnd() %s",
					 node->no_id, PQerrorMessage(loc_dbconn));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		PQclear(res2);
		res2 = PQgetResult(loc_dbconn);
		if (PQresultStatus(res2) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "copy from stdin on local node - %s %s",
					 node->no_id, PQresStatus(PQresultStatus(res2)),
					 PQresultErrorMessage(res2));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (archive_dir)
		{
			rc = archive_append_str(node, "\\.");
			if (rc < 0)
			{
				PQclear(res2);
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
		}

		PQclear(res2);
		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 INT64_FORMAT " bytes copied for table %s\n",
				 node->no_id, copysize, tab_fqname);

		/*
		 * Analyze the table to update statistics
		 */
		(void) slon_mkquery(&query1, "select %s.finishTableAfterCopy(%d); "
							"analyze %s; ",
							rtcfg_namespace, tab_id,
							tab_fqname);
		if (query_execute(node, loc_dbconn, &query1) < 0)
		{
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (archive_dir)
		{
			rc = archive_append_ds(node, &query1);
			if (rc < 0)
			{
				return -1;
			}
		}
		}
		gettimeofday(&tv_now, NULL);
		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "%.3f seconds to copy table %s\n",
				 node->no_id,
				 TIMEVAL_DIFF(&tv_start2, &tv_now), tab_fqname);
	}
	PQclear(res1);

	gettimeofday(&tv_start2, NULL);

	/*
	 * And copy over the sequence last_value corresponding to the
	 * ENABLE_SUBSCRIPTION event.
	 */
	(void) slon_mkquery(&query1,
						"select SL.seql_seqid, max(SL.seql_last_value), "
						"    %s.slon_quote_brute(PGN.nspname) || '.' || "
						"    %s.slon_quote_brute(PGC.relname) as tab_fqname "
						"	from %s.sl_sequence SQ, %s.sl_seqlog SL, "
						"		\"pg_catalog\".pg_class PGC, "
						"		\"pg_catalog\".pg_namespace PGN "
						"	where SQ.seq_set = %d "
						"		and SL.seql_seqid = SQ.seq_id "
						"		and SL.seql_ev_seqno <= '%s' "
						"		and PGC.oid = SQ.seq_reloid "
						"		and PGN.oid = PGC.relnamespace "
						"  group by 1, 3; ",
						rtcfg_namespace,
						rtcfg_namespace,
						rtcfg_namespace,
						rtcfg_namespace,
						set_id, seqbuf);
	res1 = PQexec(pro_dbconn, dstring_data(&query1));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	ntuples1 = PQntuples(res1);
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		char	   *seql_seqid = PQgetvalue(res1, tupno1, 0);
		char	   *seql_last_value = PQgetvalue(res1, tupno1, 1);
		char	   *seq_fqname = PQgetvalue(res1, tupno1, 2);

		slon_log(SLON_CONFIG, "remoteWorkerThread_%d: "
				 "set last_value of sequence %s (%s) to %s\n",
				 node->no_id, seql_seqid, seq_fqname, seql_last_value);



		(void) slon_mkquery(&query1,
							"select \"pg_catalog\".setval('%q', '%s'); ",
							seq_fqname, seql_last_value);

		if (archive_dir)
		{
			rc = archive_append_ds(node, &query1);
			if (rc < 0)
			{
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
		}

		slon_appendquery(&query1,
						 "insert into %s.sl_seqlog "
					"		(seql_seqid, seql_origin, seql_ev_seqno, "
						 "		seql_last_value) values "
						 "		(%s, %d, '%s', '%s'); ",
						 rtcfg_namespace,
						 seql_seqid, node->no_id, seqbuf, seql_last_value);
		if (query_execute(node, loc_dbconn, &query1) < 0)
		{
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
	}
	PQclear(res1);

	if (ntuples1 > 0)
	{
		gettimeofday(&tv_now, NULL);
		slon_log(SLON_INFO, "remoteWorkerThread_%d: "
				 "%.3f seconds to copy sequences\n",
				 node->no_id,
				 TIMEVAL_DIFF(&tv_start2, &tv_now));
	}
	gettimeofday(&tv_start2, NULL);

	/*
	 * It depends on who is our data provider how we construct the initial
	 * setsync status.
	 */
	if (set_origin == node->no_id)
	{
		/*
		 * Our provider is the origin, so we have to construct the setsync
		 * from scratch.
		 *
		 * The problem at hand is that the data is something between two SYNC
		 * points. So to get to the next sync point, we'll have to take this
		 * and all
		 */
		(void) slon_mkquery(&query1,
							"select max(ev_seqno) as ssy_seqno "
							"from %s.sl_event "
							"where ev_origin = %d and ev_type = 'SYNC'; ",
							rtcfg_namespace, node->no_id);
		res1 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (PQntuples(res1) != 1)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "query \"%s\" did not return a result\n",
					 node->no_id, dstring_data(&query1));
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (PQgetisnull(res1, 0, 0))
		{
			/*
			 * No SYNC event found, so we initialize the setsync to the event
			 * point of the ENABLE_SUBSCRIPTION
			 */
			ssy_seqno = seqbuf;
			ssy_snapshot = event->ev_snapshot_c;

			slon_log(SLON_INFO, "remoteWorkerThread_%d: "
					 "copy_set no previous SYNC found, use enable event.\n",
					 node->no_id);

			(void) slon_mkquery(&query1,
								"select log_actionseq "
								"from %s.sl_log_1 where log_origin = %d "
								"union select log_actionseq "
								"from %s.sl_log_2 where log_origin = %d; ",
								rtcfg_namespace, node->no_id,
								rtcfg_namespace, node->no_id);
		}
		else
		{
			/*
			 * Use the last SYNC's snapshot information and set the action
			 * sequence list to all actions after that.
			 */
			(void) slon_mkquery(&query1,
								"select ev_seqno, ev_snapshot "
								"from %s.sl_event "
								"where ev_origin = %d and ev_seqno = '%s'; ",
					   rtcfg_namespace, node->no_id, PQgetvalue(res1, 0, 0));
			PQclear(res1);
			res1 = PQexec(pro_dbconn, dstring_data(&query1));
			if (PQresultStatus(res1) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
						 node->no_id, dstring_data(&query1),
						 PQresultErrorMessage(res1));
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
			if (PQntuples(res1) != 1)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "query \"%s\" did not return a result\n",
						 node->no_id, dstring_data(&query1));
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&lsquery);
				dstring_free(&indexregenquery);
				archive_terminate(node);
				return -1;
			}
			ssy_seqno = PQgetvalue(res1, 0, 0);
			ssy_snapshot = PQgetvalue(res1, 0, 1);

			(void) slon_mkquery(&query2,
					   "log_txid >= \"pg_catalog\".txid_snapshot_xmax('%s') "
				   "or (log_txid >= \"pg_catalog\".txid_snapshot_xmin('%s')",
								ssy_snapshot, ssy_snapshot);
			slon_appendquery(&query2, " and log_txid in (select * from \"pg_catalog\".txid_snapshot_xip('%s')))", ssy_snapshot);

			slon_log(SLON_INFO, "remoteWorkerThread_%d: "
					 "copy_set SYNC found, use event seqno %s.\n",
					 node->no_id, ssy_seqno);

			(void) slon_mkquery(&query1,
								"select log_actionseq "
							 "from %s.sl_log_1 where log_origin = %d and %s "
								"union select log_actionseq "
						   "from %s.sl_log_2 where log_origin = %d and %s; ",
						 rtcfg_namespace, node->no_id, dstring_data(&query2),
						rtcfg_namespace, node->no_id, dstring_data(&query2));
		}

		/*
		 * query1 now contains the selection for the ssy_action_list selection
		 * from both log tables. Fill the dstring.
		 */
		res2 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res2) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res2));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		ntuples1 = PQntuples(res2);
		dstring_init(&ssy_action_list);
		if (ntuples1 > 0)
		{
			dstring_addchar(&ssy_action_list, '\'');
			dstring_append(&ssy_action_list, PQgetvalue(res2, 0, 0));
			dstring_addchar(&ssy_action_list, '\'');
		}
		for (tupno1 = 1; tupno1 < ntuples1; tupno1++)
		{
			dstring_addchar(&ssy_action_list, ',');
			dstring_addchar(&ssy_action_list, '\'');
			dstring_append(&ssy_action_list, PQgetvalue(res2, tupno1, 0));
			dstring_addchar(&ssy_action_list, '\'');
		}
		dstring_terminate(&ssy_action_list);
		PQclear(res2);
	}
	else
	{
		/*
		 * Our provider is another subscriber, so we can copy the existing
		 * setsync from him.
		 */
		(void) slon_mkquery(&query1,
							"select ssy_seqno, ssy_snapshot, "
							"    ssy_action_list "
							"from %s.sl_setsync where ssy_setid = %d; ",
							rtcfg_namespace, set_id);
		res1 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query1),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		if (PQntuples(res1) != 1)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "sl_setsync entry for set %d not found on provider\n",
					 node->no_id, set_id);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&lsquery);
			dstring_free(&indexregenquery);
			archive_terminate(node);
			return -1;
		}
		dstring_init(&ssy_action_list);
		ssy_seqno = PQgetvalue(res1, 0, 0);
		ssy_snapshot = PQgetvalue(res1, 0, 1);
		dstring_append(&ssy_action_list, PQgetvalue(res1, 0, 4));
		dstring_terminate(&ssy_action_list);
	}

	/*
	 * Create our own initial setsync entry
	 */
	(void) slon_mkquery(&query1,
						"delete from %s.sl_setsync where ssy_setid = %d;"
						"insert into %s.sl_setsync "
						"    (ssy_setid, ssy_origin, ssy_seqno, "
						"     ssy_snapshot, ssy_action_list) "
						"    values ('%d', '%d', '%s', '%q', '%q'); ",
						rtcfg_namespace, set_id,
						rtcfg_namespace,
						set_id, node->no_id, ssy_seqno, ssy_snapshot,
						dstring_data(&ssy_action_list));
	dstring_free(&ssy_action_list);
	if (query_execute(node, loc_dbconn, &query1) < 0)
	{
		PQclear(res1);
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	PQclear(res1);
	gettimeofday(&tv_now, NULL);
	slon_log(SLON_INFO, "remoteWorkerThread_%d: "
			 "%.3f seconds to build initial setsync status\n",
			 node->no_id,
			 TIMEVAL_DIFF(&tv_start2, &tv_now));

	/*
	 * Roll back the transaction we used on the provider and close the
	 * database connection.
	 */
	(void) slon_mkquery(&query1, "rollback transaction");
	if (query_execute(node, pro_dbconn, &query1) < 0)
	{
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&lsquery);
		dstring_free(&indexregenquery);
		archive_terminate(node);
		return -1;
	}
	slon_disconnectdb(pro_conn);
	dstring_free(&query1);
	dstring_free(&query2);
	dstring_free(&query3);
	dstring_free(&lsquery);
	dstring_free(&indexregenquery);

	slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
			 "disconnected from provider DB\n",
			 node->no_id);

	gettimeofday(&tv_now, NULL);
	slon_log(SLON_INFO, "copy_set %d done in %.3f seconds\n", set_id,
			 TIMEVAL_DIFF(&tv_start, &tv_now));

	return 0;
}


/* ----------
 * sync_event
 * ----------
 */
static int
sync_event(SlonNode *node, SlonConn *local_conn,
		   WorkerGroupData *wd, SlonWorkMsg_event *event)
{
	ProviderInfo *provider;
	ProviderSet *pset;
	char		conn_symname[64];

	/* TODO: tab_forward array to know if we need to store the log */
	PGconn	   *local_dbconn = local_conn->dbconn;
	PGresult   *res1;
	int			ntuples1;
	int			num_sets = 0;
	int			num_providers_active = 0;
	int			num_errors;
	WorkerGroupLine *wgline;
	int			i;
	int			rc;
	char		seqbuf[64];
	struct timeval tv_start;
	struct timeval tv_now;

	SlonDString query;
	SlonDString lsquery;
	SlonDString *provider_query;
	SlonDString actionseq_subquery;

	int			actionlist_len;
	int64		min_ssy_seqno;
	PerfMon pm;

	gettimeofday(&tv_start, NULL);

	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: SYNC " INT64_FORMAT
			 " processing\n",
			 node->no_id, event->ev_seqno);

	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);
	dstring_init(&query);
	dstring_init(&lsquery);

	init_perfmon(&pm);

	/*
	 * If this slon is running in log archiving mode, open a temporary file
	 * for it.
	 */
	if (archive_dir)
	{
		rc = archive_open(node, seqbuf, local_dbconn);
		if (rc < 0)
		{
			dstring_free(&query);
			dstring_free(&lsquery);
			return 60;
		}
	}

	/*
	 * Establish all required data provider connections
	 */
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		if (provider->conn != NULL)
		{
			if (PQstatus(provider->conn->dbconn) != CONNECTION_OK)
			{
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
			}
		}

		if (provider->conn == NULL)
		{
			if (provider->pa_conninfo == NULL)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "No pa_conninfo for data provider %d\n",
						 node->no_id, provider->no_id);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return 10;
			}
			sprintf(conn_symname, "subscriber_%d_provider_%d",
					node->no_id, provider->no_id);


			provider->conn = slon_connectdb(provider->pa_conninfo,
											conn_symname);
			if (provider->conn == NULL)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "cannot connect to data provider %d on '%s'\n",
						 node->no_id, provider->no_id,
						 provider->pa_conninfo);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return provider->pa_connretry;
			}

			/*
			 * Listen on the special relation telling our node relationship
			 */
			(void) slon_mkquery(&query,
								"select %s.registerNodeConnection(%d); ",
								rtcfg_namespace, rtcfg_nodeid);
			start_monitored_event(&pm);
			if (query_execute(node, provider->conn->dbconn, &query) < 0)
			{
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
				return provider->pa_connretry;
			}
			monitor_provider_query(&pm);

			slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
					 "connected to data provider %d on '%s'\n",
					 node->no_id, provider->no_id,
					 provider->pa_conninfo);
		}
	}

	/*
	 * Check that all these providers have processed at least up to the SYNC
	 * event we're handling here.
	 */
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		/*
		 * We only need to explicitly check this if the data provider is
		 * neither the set origin, nor the node we received this event from.
		 */
		if (event->ev_origin != provider->no_id &&
			event->event_provider != provider->no_id)
		{
			int64		prov_seqno;

			prov_seqno = get_last_forwarded_confirm(event->ev_origin,
													provider->no_id);
			if (prov_seqno < 0)
			{
				slon_log(SLON_WARN, "remoteWorkerThread_%d: "
						 "don't know what ev_seqno node %d confirmed "
						 "for ev_origin %d\n",
						 node->no_id, provider->no_id,
						 event->ev_origin);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return 10;
			}
			if (prov_seqno < event->ev_seqno)
			{
				slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
						 "data provider %d only confirmed up to "
						 "ev_seqno " INT64_FORMAT " for ev_origin %d\n",
						 node->no_id, provider->no_id,
						 prov_seqno, event->ev_origin);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return 10;
			}
			slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
					 "data provider %d confirmed up to "
					 "ev_seqno %s for ev_origin %d - OK\n",
					 node->no_id, provider->no_id,
					 seqbuf, event->ev_origin);
		}
	}

	min_ssy_seqno = -1;
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		int			ntuples1;
		int			tupno1;
		PGresult   *res2;
		int			ntuples2;
		int			tupno2;
		int			ntables_total = 0;
		int			rc;
		int			need_union;
		int			sl_log_no;

		need_union = 0;
		provider_query = &(provider->helper_query);
		dstring_reset(provider_query);
		(void) slon_mkquery(provider_query,
							"declare LOG cursor for ");

		/*
		 * Get the current sl_log_status value for this provider
		 */
		(void) slon_mkquery(&query, "select last_value from %s.sl_log_status",
							rtcfg_namespace);

		start_monitored_event(&pm);
		res1 = PQexec(provider->conn->dbconn, dstring_data(&query));
		monitor_provider_query(&pm);

		rc = PQresultStatus(res1);
		if (rc != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR,
					 "remoteWorkerThread_%d: \"%s\" %s %s\n",
					 node->no_id, dstring_data(&query),
					 PQresStatus(rc),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			dstring_free(&query);
			dstring_free(&lsquery);
			archive_terminate(node);
			return 60;
		}
		if (PQntuples(res1) != 1)
		{
			slon_log(SLON_ERROR,
				 "remoteWorkerThread_%d: \"%s\" %s returned %d tuples\n",
					 node->no_id, dstring_data(&query),
					 PQresStatus(rc), PQntuples(res1));
			PQclear(res1);
			dstring_free(&query);
			dstring_free(&lsquery);
			archive_terminate(node);
			return 60;
		}
		provider->log_status = strtol(PQgetvalue(res1, 0, 0), NULL, 10);
		PQclear(res1);
		slon_log(SLON_DEBUG2,
			"remoteWorkerThread_%d_%d: current remote log_status = %d\n",
				 node->no_id, provider->no_id, provider->log_status);

		/*
		 * Select all sets we receive from this provider and which are not
		 * synced better than this SYNC already.
		 */
		(void) slon_mkquery(&query,
							"select SSY.ssy_setid, SSY.ssy_seqno, "
				  "    \"pg_catalog\".txid_snapshot_xmax(SSY.ssy_snapshot), "
							"    SSY.ssy_snapshot, "
							"    SSY.ssy_action_list "
							"from %s.sl_setsync SSY "
							"where SSY.ssy_seqno < '%s' "
							"    and SSY.ssy_setid in (",
							rtcfg_namespace, seqbuf);
		for (pset = provider->set_head; pset; pset = pset->next)
			slon_appendquery(&query, "%s%d",
							 (pset->prev == NULL) ? "" : ",",
							 pset->set_id);
		slon_appendquery(&query, "); ");

		start_monitored_event(&pm);
		res1 = PQexec(local_dbconn, dstring_data(&query));
		monitor_subscriber_query(&pm);

		slon_log(SLON_DEBUG1, "about to monitor_subscriber_query - pulling big actionid list for %d\n", provider->no_id);

		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			dstring_free(&query);
			dstring_free(&lsquery);
			archive_terminate(node);
			return 60;
		}

		/*
		 * For every set we receive from this provider
		 */
		ntuples1 = PQntuples(res1);
		if (ntuples1 == 0)
		{
			PQclear(res1);
			slon_appendquery(provider_query,"select * FROM %s.sl_log_1" 
							 " where false",rtcfg_namespace);
			continue;
		}
		num_sets += ntuples1;

		for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
		{
			int			sub_set = strtol(PQgetvalue(res1, tupno1, 0), NULL, 10);
			char	   *ssy_maxxid = PQgetvalue(res1, tupno1, 2);
			char	   *ssy_snapshot = PQgetvalue(res1, tupno1, 3);
			char	   *ssy_action_list = PQgetvalue(res1, tupno1, 4);
			int64		ssy_seqno;

			slon_scanint64(PQgetvalue(res1, tupno1, 1), &ssy_seqno);
			if (min_ssy_seqno < 0 || ssy_seqno < min_ssy_seqno)
				min_ssy_seqno = ssy_seqno;

			/*
			 * Select the tables in that set ...
			 */
			(void) slon_mkquery(&query,
								"select T.tab_id, T.tab_set, "
							"    %s.slon_quote_brute(PGN.nspname) || '.' || "
						"    %s.slon_quote_brute(PGC.relname) as tab_fqname "
								"from %s.sl_table T, "
								"    \"pg_catalog\".pg_class PGC, "
								"    \"pg_catalog\".pg_namespace PGN "
								"where T.tab_set = %d "
								"    and PGC.oid = T.tab_reloid "
								"    and PGC.relnamespace = PGN.oid; ",
								rtcfg_namespace,
								rtcfg_namespace,
								rtcfg_namespace,
								sub_set);

			start_monitored_event(&pm);
			res2 = PQexec(local_dbconn, dstring_data(&query));
			monitor_subscriber_query(&pm);

			if (PQresultStatus(res2) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
						 node->no_id, dstring_data(&query),
						 PQresultErrorMessage(res2));
				PQclear(res2);
				PQclear(res1);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return 60;
			}
			ntuples2 = PQntuples(res2);
			slon_log(SLON_INFO, "remoteWorkerThread_%d: "
					 "syncing set %d with %d table(s) from provider %d\n",
					 node->no_id, sub_set, ntuples2,
					 provider->no_id);

			if (ntuples2 == 0)
			{
				PQclear(res2);

				continue;
			}
			ntables_total += ntuples2;

			/*
			 * ... and build up a the provider query
			 */
			for (sl_log_no = 1; sl_log_no <= 2; sl_log_no++)
			{
				/*
				 * We only need to query sl_log_1 when log_status is
				 * 0 or during log switching (log_status 2 and 3).
				 */
				if (sl_log_no == 1 && provider->log_status == 1)
					continue;
				/*
				 * Likewise we only query sl_log_2 when log_status is
				 * 1, 2 or 3.
				 */
				if (sl_log_no == 2 && provider->log_status == 0)
					continue;


				if (need_union)
				{
					slon_appendquery(provider_query, " union all ");
				}
				need_union = 1;

				/*
				 * First for the big chunk that does the index
				 * scan with upper and lower bounds:
				 *
				 * select ... from sl_log_N 
				 *     where log_origin = X
				 *     and log_tableid in (<this set's tables>)
				 */
				slon_appendquery(provider_query,
					"select log_origin, log_txid, log_tableid, "
							"log_actionseq, log_cmdtype, "
							"octet_length(log_cmddata), "
							"case when octet_length(log_cmddata) <= %d "
								"then log_cmddata "
								"else null end "
						"from %s.sl_log_%d "
						"where log_origin = %d "
						"and log_tableid in (",
								sync_max_rowsize,
								rtcfg_namespace, sl_log_no,
								node->no_id);
				for (tupno2 = 0; tupno2 < ntuples2; tupno2++)
				{
					if (tupno2 > 0)
						dstring_addchar(provider_query, ',');
					dstring_append(provider_query, 
							PQgetvalue(res2, tupno2, 0));
				}
				dstring_append(provider_query, ") ");

				/*
				 * and log_txid >= '<maxxid_last_snapshot>'
				 * and log_txid < '<maxxid_this_snapshot>'
				 * and txit_visible_in_snapshot(log_txid, '<this_snapshot>')
				 */
				slon_appendquery(provider_query,
					 "and log_txid >= '%s' "
					 "and log_txid < '%s' "
					 "and \"pg_catalog\".txid_visible_in_snapshot(log_txid, '%s') ",
					 ssy_maxxid,
					 event->ev_maxtxid_c,
					 event->ev_snapshot_c);

				/*
				 * and (<actionseq_qual_on_first_sync>)
				 */
				actionlist_len = strlen(ssy_action_list);
				slon_log(SLON_DEBUG2, "remoteWorkerThread_%d_%d: "
						"ssy_action_list length: %d\n",
						node->no_id, provider->no_id,
						actionlist_len);
				slon_log(SLON_DEBUG4, "remoteWorkerThread_%d_%d: "
						"ssy_action_list value: %s\n",
						node->no_id, provider->no_id,
						ssy_action_list);
				if (actionlist_len > 0)
				{
					dstring_init(&actionseq_subquery);
					compress_actionseq(ssy_action_list, &actionseq_subquery);
					slon_appendquery(provider_query,
									 " and (%s)",
									 dstring_data(&actionseq_subquery));
					dstring_free(&actionseq_subquery);
				}

				/*
				 * Now do it all over again to get the log rows
				 * from in-progress transactions at snapshot one
				 * that have committed by the time of snapshot two.
				 * again, we do:
				 *
				 * select ... from sl_log_N 
				 *     where log_origin = X
				 *     and log_tableid in (<this set's tables>)
				 */
				slon_appendquery(provider_query,
					"union all "
					"select log_origin, log_txid, log_tableid, "
							"log_actionseq, log_cmdtype, "
							"octet_length(log_cmddata), "
							"case when octet_length(log_cmddata) <= %d "
								"then log_cmddata "
								"else null end "
						"from %s.sl_log_%d "
						"where log_origin = %d "
						"and log_tableid in (",
								sync_max_rowsize,
								rtcfg_namespace, sl_log_no,
								node->no_id);
				for (tupno2 = 0; tupno2 < ntuples2; tupno2++)
				{
					if (tupno2 > 0)
						dstring_addchar(provider_query, ',');
					dstring_append(provider_query, 
							PQgetvalue(res2, tupno2, 0));
				}
				dstring_append(provider_query, ") ");

				/*
				 * and log_txid in (select
				 *				txid_snapshot_xip('<last_snapshot>'))
				 * and txit_visible_in_snapshot(log_txid, '<this_snapshot>')
				 */
				slon_appendquery(provider_query,
					 "and log_txid in (select * from "
					 "\"pg_catalog\".txid_snapshot_xip('%s') "
					 "except "
					 "select * from "
					 "\"pg_catalog\".txid_snapshot_xip('%s') )",
					 ssy_snapshot,
					 event->ev_snapshot_c);

				/*
				 * and (<actionseq_qual_on_first_sync>)
				 */
				actionlist_len = strlen(ssy_action_list);
				if (actionlist_len > 0)
				{
					dstring_init(&actionseq_subquery);
					compress_actionseq(ssy_action_list, &actionseq_subquery);
					slon_appendquery(provider_query,
									 " and (%s)",
									 dstring_data(&actionseq_subquery));
					dstring_free(&actionseq_subquery);
				}
			}
		
			/* Remember info about the tables in the set */
			for (tupno2 = 0; tupno2 < ntuples2; tupno2++)
			{
				int			tab_id = strtol(PQgetvalue(res2, tupno2, 0), NULL, 10);
				int			tab_set = strtol(PQgetvalue(res2, tupno2, 1), NULL, 10);
				SlonSet    *rtcfg_set;

				/*
				 * Remember the fully qualified table name on the fly. This
				 * might have to become a hashtable someday.
				 */
				while (tab_id >= wd->tab_fqname_size)
				{
					wd->tab_fqname = (char **) realloc(wd->tab_fqname,
								   sizeof(char *) * wd->tab_fqname_size * 2);
					memset(&(wd->tab_fqname[wd->tab_fqname_size]), 0,
						   sizeof(char *) * wd->tab_fqname_size);
					wd->tab_forward = realloc(wd->tab_forward,
											  wd->tab_fqname_size * 2);
					memset(&(wd->tab_forward[wd->tab_fqname_size]), 0,
						   wd->tab_fqname_size);
					wd->tab_fqname_size *= 2;
				}
				wd->tab_fqname[tab_id] = strdup(PQgetvalue(res2, tupno2, 2));

				/*
				 * Also remember if the tables log data needs to be forwarded.
				 */
				for (rtcfg_set = rtcfg_set_list_head; rtcfg_set;
					 rtcfg_set = rtcfg_set->next)
				{
					if (rtcfg_set->set_id == tab_set)
					{
						wd->tab_forward[tab_id] = rtcfg_set->sub_forward;
						break;
					}
				}
			}

			PQclear(res2);
		}
		PQclear(res1);

		/*
		 * Finally add the order by clause.
		 */
		dstring_append(provider_query, " order by log_actionseq");
		dstring_terminate(provider_query);

		/*
		 * Check that we select something from the provider.
		 */
		if (!need_union)
		{
			/*
			 * This can happen when there are no tables in any of the
			 * sets that we subscribe from this node.
			 */
			slon_mkquery(provider_query,
				"declare LOG cursor for "
				"select log_origin, log_txid, log_tableid, "
						"log_actionseq, log_cmdtype, "
						"octet_length(log_cmddata), "
						"case when octet_length(log_cmddata) <= %d "
							"then log_cmddata "
							"else null end "
					"from %s.sl_log_1 "
					"where false",
					sync_max_rowsize, rtcfg_namespace);
		}
	}

	/*
	 * If we have found no sets needing sync at all, why bother the helpers?
	 */
	if (num_sets == 0)
	{
		slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
				 "no sets need syncing for this event\n",
				 node->no_id);
		dstring_free(&query);
		dstring_free(&lsquery);
		if (archive_dir)
		{
			rc = archive_close(node);
			if (rc < 0)
				slon_retry();
		}
		return 0;
	}

	/*
	 * Get the current sl_log_status
	 */
	(void) slon_mkquery(&query, "select last_value from %s.sl_log_status",
						rtcfg_namespace);
	start_monitored_event(&pm);
	res1 = PQexec(local_dbconn, dstring_data(&query));
	monitor_subscriber_query(&pm);

	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s\n",
				 node->no_id, dstring_data(&query),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		dstring_free(&query);
		dstring_free(&lsquery);
		archive_terminate(node);
		slon_disconnectdb(provider->conn);
		provider->conn = NULL;
		return 20;
	}
	ntuples1 = PQntuples(res1);
	if (ntuples1 != 1)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: cannot determine current log status\n",
				 node->no_id);
		PQclear(res1);
		dstring_free(&query);
		dstring_free(&lsquery);
		archive_terminate(node);
		slon_disconnectdb(provider->conn);
		provider->conn = NULL;
		return 20;
	}
	wd->active_log_table = (strtol(PQgetvalue(res1, 0, 0), NULL, 10) & 0x01) + 1;
	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "current local log_status is %d\n",
			 node->no_id, strtol(PQgetvalue(res1, 0, 0), NULL, 10));
	PQclear(res1);

	/*
	 * If we have a explain_interval, run the query through explain
	 * and output the query as well as the resulting query plan.
	 */
	if (explain_interval > 0)
	{
		struct timeval current_time;

		gettimeofday(&current_time, NULL);

		if (explain_lastsec + explain_interval <= current_time.tv_sec)
		{
			explain_thistime = true;
			explain_lastsec = current_time.tv_sec;
		}
		else
		{
			explain_thistime = false;
		}
	}

	/*
	 * Time to get the helpers busy.
	 */
	wd->workgroup_status = SLON_WG_BUSY;
	pthread_mutex_unlock(&(wd->workdata_lock));
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		pthread_mutex_lock(&(provider->helper_lock));
		slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
				 "activate helper %d\n",
				 node->no_id, provider->no_id);
		provider->helper_status = SLON_WG_BUSY;
		pthread_cond_signal(&(provider->helper_cond));
		pthread_mutex_unlock(&(provider->helper_lock));
		num_providers_active++;
	}

	num_errors = 0;
	while (num_providers_active > 0)
	{
		WorkerGroupLine *lines_head = NULL;
		WorkerGroupLine *lines_tail = NULL;
		WorkerGroupLine *wgnext = NULL;

		/*
		 * Consume the replication data from the providers
		 */
		pthread_mutex_lock(&(wd->workdata_lock));
		while (wd->repldata_head == NULL)
		{
			slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: waiting for log data\n",
					 node->no_id);
			pthread_cond_wait(&(wd->repldata_cond), &(wd->workdata_lock));
		}
		lines_head = wd->repldata_head;
		lines_tail = wd->repldata_tail;
		wd->repldata_head = NULL;
		wd->repldata_tail = NULL;
		pthread_mutex_unlock(&(wd->workdata_lock));

		for (wgline = lines_head; wgline; wgline = wgline->next)
		{
			/*
			 * Got a line ... process content
			 */
			switch (wgline->code)
			{
				case SLON_WGLC_ACTION:
					if (num_errors > 0)
						break;

					if (wgline->log.n_used > 0)
					{
						start_monitored_event(&pm);
						res1 = PQexec(local_dbconn, dstring_data(&(wgline->log)));
						monitor_subscriber_iud(&pm);

						if (PQresultStatus(res1) == PGRES_EMPTY_QUERY)
						{
							PQclear(res1);
							break;
						}
						if (PQresultStatus(res1) != PGRES_COMMAND_OK)
						{
							slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
									 "\"%s\" %s - query was: %s\n",
								  node->no_id, dstring_data(&(wgline->data)),
									 PQresultErrorMessage(res1),
									 dstring_data(&(wgline->provider->helper_query)));
							num_errors++;
						}
						PQclear(res1);
					}

					start_monitored_event(&pm);
					res1 = PQexec(local_dbconn, dstring_data(&(wgline->data)));
					monitor_subscriber_iud(&pm);

					if (PQresultStatus(res1) == PGRES_EMPTY_QUERY)
					{
						PQclear(res1);
						break;
					}
					if (PQresultStatus(res1) != PGRES_COMMAND_OK)
					{
						slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
								 "\"%s\" %s - query was: %s\n",
								 node->no_id, dstring_data(&(wgline->data)),
								 PQresultErrorMessage(res1),
								 dstring_data(&(wgline->provider->helper_query)));
						num_errors++;
					}
#ifdef SLON_CHECK_CMDTUPLES
					else
					{
						if (strtol(PQcmdTuples(res1), NULL, 10) != 1)
						{
							slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
									 "replication query did not affect "
									 "one data row (cmdTuples = %s) - "
								   "query was: %s - query was: %s\n",
									 node->no_id, PQcmdTuples(res1),
									 dstring_data(&(wgline->data)),
									 dstring_data(&(wgline->provider->helper_query)));
							num_errors++;
						}
						else
							slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: %s\n",
								 node->no_id, dstring_data(&(wgline->data)));
					}
#endif
					PQclear(res1);

					/*
					 * Add the user data modification part to the archive log.
					 */
					if (archive_dir)
					{
						rc = archive_append_ds(node, &(wgline->data));
						if (rc < 0)
							slon_retry();
					}
					break;

				case SLON_WGLC_DONE:
					provider = wgline->provider;
					slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
							 "helper %d finished\n",
							 node->no_id, wgline->provider->no_id);
					num_providers_active--;
					break;

				case SLON_WGLC_ERROR:
					provider = wgline->provider;
					slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
							 "helper %d finished with error\n",
							 node->no_id, wgline->provider->no_id);
					num_providers_active--;
					num_errors++;
					break;
			}
		}

		/*
		 * Put the line buffers back into the pool.
		 */
		slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: returning lines to pool\n",
				 node->no_id);
		pthread_mutex_lock(&(wd->workdata_lock));
		for (wgline = lines_head; wgline; wgline = wgnext)
		{
			wgnext = wgline->next;
			if (wgline->line_largemem > 0)
			{
				/*
				 * Really free the lines that contained large rows
				 */
				dstring_free(&(wgline->data));
				dstring_free(&(wgline->log));
				dstring_init(&(wgline->data));
				dstring_init(&(wgline->log));
				wd->workdata_largemem -= wgline->line_largemem;
				wgline->line_largemem = 0;
			}
			else
			{
				/*
				 * just reset (and allow to grow further) the small ones
				 */
				dstring_reset(&(wgline->data));
				dstring_reset(&(wgline->log));
			}
			DLLIST_ADD_HEAD(wd->linepool_head, wd->linepool_tail, wgline);
		}
		if (num_errors == 1)
			wd->workgroup_status = SLON_WG_ABORT;
		pthread_cond_broadcast(&(wd->linepool_cond));
		pthread_mutex_unlock(&(wd->workdata_lock));
	}

	/*
	 * Inform the helpers that the whole group is done with this SYNC.
	 */
	slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
			 "all helpers done.\n",
			 node->no_id);
	pthread_mutex_lock(&(wd->workdata_lock));
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: "
				 "changing helper %d to IDLE\n",
				 node->no_id, provider->no_id);
		pthread_mutex_lock(&(provider->helper_lock));
		provider->helper_status = SLON_WG_IDLE;
		pthread_cond_signal(&(provider->helper_cond));
		pthread_mutex_unlock(&(provider->helper_lock));
	}

	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: cleanup\n",
			 node->no_id);

	/*
	 * Cleanup
	 */
	for (i = 0; i < wd->tab_fqname_size; i++)
	{
		if (wd->tab_fqname[i] != NULL)
		{
			free(wd->tab_fqname[i]);
			wd->tab_fqname[i] = NULL;
		}
	}
	memset(wd->tab_forward, 0, wd->tab_fqname_size);

	/*
	 * If there have been any errors, abort the SYNC
	 */
	if (num_errors != 0)
	{
		dstring_free(&query);
		dstring_free(&lsquery);
		archive_terminate(node);
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: SYNC aborted\n",
				 node->no_id);
		return 10;
	}

	/*
	 * Get all sequence updates
	 */
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		int			ntuples1;
		int			tupno1;
		char		min_ssy_seqno_buf[64];

		sprintf(min_ssy_seqno_buf, INT64_FORMAT, min_ssy_seqno);

		(void) slon_mkquery(&query,
							"select SL.seql_seqid, max(SL.seql_last_value) "
							"	from %s.sl_seqlog SL, "
							"		%s.sl_sequence SQ "
							"	where SQ.seq_id = SL.seql_seqid "
							"		and SL.seql_origin = %d "
							"		and SL.seql_ev_seqno <= '%s' "
							"		and SL.seql_ev_seqno >= '%s' "
							"		and SQ.seq_set in (",
							rtcfg_namespace, rtcfg_namespace,
							node->no_id, seqbuf, min_ssy_seqno_buf);
		for (pset = provider->set_head; pset; pset = pset->next)
			slon_appendquery(&query, "%s%d",
							 (pset->prev == NULL) ? "" : ",",
							 pset->set_id);
		slon_appendquery(&query, ") "
						 "  group by 1; ");

		start_monitored_event(&pm);
		res1 = PQexec(provider->conn->dbconn, dstring_data(&query));
		monitor_provider_query(&pm);

		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s\n",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			dstring_free(&query);
			dstring_free(&lsquery);
			archive_terminate(node);
			slon_disconnectdb(provider->conn);
			provider->conn = NULL;
			return 20;
		}
		ntuples1 = PQntuples(res1);
		for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
		{
			char	   *seql_seqid = PQgetvalue(res1, tupno1, 0);
			char	   *seql_last_value = PQgetvalue(res1, tupno1, 1);

			(void) slon_mkquery(&query,
							 "select %s.sequenceSetValue(%s,%d,'%s','%s'); ",
								rtcfg_namespace,
						   seql_seqid, node->no_id, seqbuf, seql_last_value);
			start_monitored_event(&pm);
			if (query_execute(node, local_dbconn, &query) < 0)
			{
				PQclear(res1);
				dstring_free(&query);
				dstring_free(&lsquery);
				archive_terminate(node);
				return 60;
			}
			monitor_subscriber_iud(&pm);

			/*
			 * Add the sequence number adjust call to the archive log.
			 */
			if (archive_dir)
			{
				(void) slon_mkquery(&lsquery,
							"select %s.sequenceSetValue_offline(%s,'%s');\n",
									rtcfg_namespace,
									seql_seqid, seql_last_value);
				rc = archive_append_ds(node, &lsquery);
				if (rc < 0)
					slon_retry();
			}
		}
		PQclear(res1);
	}

	/*
	 * Light's are still green ... update the setsync status of all the sets
	 * we've just replicated ...
	 */
	(void) slon_mkquery(&query,
						"update %s.sl_setsync set "
						"    ssy_seqno = '%s', ssy_snapshot = '%s', "
						"    ssy_action_list = '' "
						"where ssy_setid in (",
						rtcfg_namespace,
						seqbuf, event->ev_snapshot_c);
	i = 0;
	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		for (pset = provider->set_head; pset; pset = pset->next)
		{
			slon_appendquery(&query, "%s%d", (i == 0) ? "" : ",",
							 pset->set_id);
			i++;
		}
	}

	if (i > 0)
	{
		/*
		 * ... if there could be any, that is.
		 */
		slon_appendquery(&query, ") and ssy_seqno < '%s'; ", seqbuf);
		
		start_monitored_event(&pm);
		res1 = PQexec(local_dbconn, dstring_data(&query));
		monitor_subscriber_query (&pm);

		if (PQresultStatus(res1) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			dstring_free(&query);
			dstring_free(&lsquery);
			archive_terminate(node);
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: SYNC aborted\n",
					 node->no_id);
			return 10;
		}
		PQclear(res1);
	}

	/*
	 * Add the final commit to the archive log, close it and rename the
	 * temporary file to the real log chunk filename.
	 */
	if (archive_dir)
	{
		rc = archive_close(node);
		if (rc < 0)
			slon_retry();
	}

	/*
	 * Good job!
	 */
	dstring_free(&query);
	dstring_free(&lsquery);
	gettimeofday(&tv_now, NULL);
	slon_log(SLON_INFO, "remoteWorkerThread_%d: SYNC "
			 INT64_FORMAT " done in %.3f seconds\n",
			 node->no_id, event->ev_seqno,
			 TIMEVAL_DIFF(&tv_start, &tv_now));

 	slon_log(SLON_DEBUG1, 
			 "remoteWorkerThread_%d: SYNC " INT64_FORMAT " sync_event timing: " 
			 " pqexec (s/count)" 
			 "- provider %.3f/%d " 
			 "- subscriber %.3f/%d " 
			 "- IUD %.3f/%d\n",
			 node->no_id, event->ev_seqno, 
			 pm.prov_query_t, pm.prov_query_c, 
			 pm.subscr_query_t, pm.prov_query_c,
			 pm.subscr_iud__t, pm.subscr_iud__c);

	return 0;
}


/* ----------
 * sync_helper
 * ----------
 */
static void *
sync_helper(void *cdata)
{
	ProviderInfo *provider = (ProviderInfo *) cdata;
	WorkerGroupData *wd = provider->wd;
	SlonNode   *node = wd->node;
	PGconn	   *dbconn;
	WorkerGroupLine *line = NULL;
	SlonDString query;
	SlonDString query2;
	int			errors;
	struct timeval tv_start;
	struct timeval tv_now;
	int			first_fetch;
	int			log_status;
	int			rc;

	WorkerGroupLine *data_line[SLON_DATA_FETCH_SIZE];
	int			data_line_alloc = 0;
	int			data_line_first = 0;
	int			data_line_last = 0;

	PGresult   *res = NULL;
	PGresult   *res2 = NULL;
	PGresult   *res3 = NULL;
	int			ntuples;
	int			tupno;

	int			line_no;
	int			line_ncmds;

	PerfMon pm;

	dstring_init(&query);
	dstring_init(&query2);

	for (;;)
	{
		pthread_mutex_lock(&(provider->helper_lock));
		while (provider->helper_status == SLON_WG_IDLE)
		{
			slon_log(SLON_DEBUG4, "remoteHelperThread_%d_%d: "
					 "waiting for work\n",
					 node->no_id, provider->no_id);

			pthread_cond_wait(&(provider->helper_cond), &(provider->helper_lock));
		}

		if (provider->helper_status == SLON_WG_EXIT)
		{
			dstring_free(&query);
			dstring_free(&query2);
			pthread_mutex_unlock(&(provider->helper_lock));
			pthread_exit(NULL);
		}
		if (provider->helper_status != SLON_WG_BUSY)
		{
			provider->helper_status = SLON_WG_IDLE;
			pthread_mutex_unlock(&(provider->helper_lock));
			continue;
		}

		/*
		 * OK, we got work to do.
		 */
		dbconn = provider->conn->dbconn;
		pthread_mutex_unlock(&(provider->helper_lock));

		slon_log(SLON_DEBUG4,
				 "remoteHelperThread_%d_%d: got work to do\n",
				 node->no_id, provider->no_id);

		errors = 0;
		do
		{
			init_perfmon(&pm);
			/*
			 * Start a transaction
			 */

			(void) slon_mkquery(&query, "start transaction; "
								"set enable_seqscan = off; "
								"set enable_indexscan = on; ");

			start_monitored_event(&pm);

			if (query_execute(node, dbconn, &query) < 0)
			{
				errors++;
				break;
			}
			monitor_subscriber_query (&pm);

			/*
			 * Get the current sl_log_status value
			 */
			(void) slon_mkquery(&query, "select last_value from %s.sl_log_status",
								rtcfg_namespace);

			start_monitored_event(&pm);
			res3 = PQexec(dbconn, dstring_data(&query));
			monitor_provider_query(&pm);

			rc = PQresultStatus(res3);
			if (rc != PGRES_TUPLES_OK)
			{
				slon_log(SLON_ERROR,
						 "remoteWorkerThread_%d: \"%s\" %s %s\n",
						 node->no_id, dstring_data(&query),
						 PQresStatus(rc),
						 PQresultErrorMessage(res3));
				PQclear(res3);
				errors++;
				break;
			}
			if (PQntuples(res3) != 1)
			{
				slon_log(SLON_ERROR,
					 "remoteWorkerThread_%d: \"%s\" %s returned %d tuples\n",
						 node->no_id, dstring_data(&query),
						 PQresStatus(rc), PQntuples(res3));
				PQclear(res3);
				errors++;
				break;
			}
			log_status = strtol(PQgetvalue(res3, 0, 0), NULL, 10);
			PQclear(res3);
			slon_log(SLON_DEBUG2,
				"remoteWorkerThread_%d_%d: current remote log_status = %d\n",
					 node->no_id, provider->no_id, log_status);

			/*
			 * See if we have to run the query through EXPLAIN first
			 */
			if (explain_thistime)
			{
				SlonDString		explain_query;

				/*
				 * Let Postgres EXPLAIN the query plan for the current
				 * log selection query
				 */
				dstring_init(&explain_query);
				slon_mkquery(&explain_query, "explain %s",
					dstring_data(&(provider->helper_query)));

				res = PQexec(dbconn, dstring_data(&explain_query));
				if (PQresultStatus(res) != PGRES_TUPLES_OK)
				{
					slon_log(SLON_ERROR, "remoteHelperThread_%d_%d: \"%s\" %s",
							 node->no_id, provider->no_id,
							 dstring_data(&explain_query),
							 PQresultErrorMessage(res));
					PQclear(res);
					dstring_free(&explain_query);
					errors++;
					break;
				}

				slon_log(SLON_INFO, 
					"remoteWorkerThread_%d_%d: "
					"Log selection query: %s\n",
					node->no_id, provider->no_id,
					dstring_data(&explain_query));
				slon_log(SLON_INFO, 
					"remoteWorkerThread_%d_%d: Query Plan:\n",
					node->no_id, provider->no_id);

				ntuples = PQntuples(res);
				for (tupno = 0; tupno < ntuples; tupno++)
				{
					slon_log(SLON_INFO, 
						"remoteWorkerThread_%d_%d: PLAN %s\n",
						node->no_id, provider->no_id,
						PQgetvalue(res, tupno, 0));
				}
				slon_log(SLON_INFO,
					"remoteWorkerThread_%d_%d: PLAN_END\n",
						node->no_id, provider->no_id);

				PQclear(res);
				dstring_free(&explain_query);
			}

			gettimeofday(&tv_start, NULL);
			first_fetch = true;
			res = NULL;

			/*
			 * Open a cursor that reads the log data.
			 */
			start_monitored_event(&pm);
			if (query_execute(node, dbconn, &(provider->helper_query)) < 0)
			{
				errors++;
				break;
			}
			monitor_provider_query(&pm);

			(void) slon_mkquery(&query, "fetch %d from LOG; ",
							  SLON_DATA_FETCH_SIZE * SLON_COMMANDS_PER_LINE);
			data_line_alloc = 0;
			data_line_first = 0;
			data_line_last = 0;

			res = NULL;
			ntuples = 0;
			tupno = 0;

			while (!errors)
			{
				/*
				 * Deliver filled line buffers to the worker process.
				 */
				if (data_line_last > data_line_first)
				{
					slon_log(SLON_DEBUG4,
					"remoteHelperThread_%d_%d: deliver %d lines to worker\n",
							 node->no_id, provider->no_id,
							 data_line_last - data_line_first);

					pthread_mutex_lock(&(wd->workdata_lock));
					while (data_line_first < data_line_last)
					{
						DLLIST_ADD_TAIL(wd->repldata_head, wd->repldata_tail,
										data_line[data_line_first]);
						data_line_first++;
					}
					pthread_cond_signal(&(wd->repldata_cond));
					pthread_mutex_unlock(&(wd->workdata_lock));
				}

				/*
				 * If we cycled through all the allocated line buffers, reset
				 * the indexes.
				 */
				if (data_line_first == data_line_alloc)
				{
					data_line_alloc = 0;
					data_line_first = 0;
					data_line_last = 0;
				}

				/*
				 * Make sure we are inside memory limits and that we have
				 * available line buffers.
				 */
				pthread_mutex_lock(&(wd->workdata_lock));
				if (data_line_alloc == 0 ||
					wd->workdata_largemem > sync_max_largemem)
				{
					/*
					 * First make sure that the overall memory usage is inside
					 * bounds.
					 */
					if (wd->workdata_largemem > sync_max_largemem)
					{
						slon_log(SLON_DEBUG4,
								 "remoteHelperThread_%d_%d: wait for oversize memory to free\n",
								 node->no_id, provider->no_id);

						while (wd->workdata_largemem > sync_max_largemem &&
							   wd->workgroup_status == SLON_WG_BUSY)
						{
							pthread_cond_wait(&(wd->linepool_cond), &(wd->workdata_lock));
						}
						if (wd->workgroup_status != SLON_WG_BUSY)
						{
							slon_log(SLON_DEBUG4,
							   "remoteHelperThread_%d_%d: abort operation\n",
									 node->no_id, provider->no_id);
							errors++;
							break;
						}
					}

					/*
					 * Second make sure that we have at least 1 line buffer.
					 */
					if (data_line_alloc == 0)
					{
						slon_log(SLON_DEBUG4,
						 "remoteHelperThread_%d_%d: allocate line buffers\n",
								 node->no_id, provider->no_id);
						while (data_line_alloc == 0 && !errors)
						{
							while (wd->linepool_head == NULL &&
								   wd->workgroup_status == SLON_WG_BUSY)
							{
								pthread_cond_wait(&(wd->linepool_cond), &(wd->workdata_lock));
							}
							if (wd->workgroup_status != SLON_WG_BUSY)
							{
								slon_log(SLON_DEBUG4,
								"remoteHelperThread_%d_%d: abort operation\n",
										 node->no_id, provider->no_id);
								errors++;
								break;
							}

							/*
							 * While we are at it, we can as well allocate up
							 * to FETCH_SIZE buffers.
							 */
							while (data_line_alloc < SLON_DATA_FETCH_SIZE &&
								   wd->linepool_head != NULL)
							{
								data_line[data_line_alloc] = wd->linepool_head;
								DLLIST_REMOVE(wd->linepool_head, wd->linepool_tail,
											  data_line[data_line_alloc]);
								data_line_alloc++;
							}
						}
					}
				}
				pthread_mutex_unlock(&(wd->workdata_lock));

				/*
				 * We are within memory limits and have allocated line
				 * buffers. Make sure that we have log lines fetched.
				 */
				if (tupno >= ntuples)
				{
					slon_log(SLON_DEBUG4,
							 "remoteHelperThread_%d_%d: fetch from cursor\n",
							 node->no_id, provider->no_id);
					if (res != NULL)
						PQclear(res);

					start_monitored_event(&pm);
					res = PQexec(dbconn, dstring_data(&query));
					monitor_provider_query(&pm);

					if (PQresultStatus(res) != PGRES_TUPLES_OK)
					{
						slon_log(SLON_ERROR, "remoteHelperThread_%d_%d: \"%s\" %s",
								 node->no_id, provider->no_id,
								 dstring_data(&query),
								 PQresultErrorMessage(res));
						errors++;
						break;
					}
					if (first_fetch)
					{
						gettimeofday(&tv_now, NULL);
						slon_log(SLON_DEBUG1,
								 "remoteHelperThread_%d_%d: %.3f seconds delay for first row\n",
								 node->no_id, provider->no_id,
								 TIMEVAL_DIFF(&tv_start, &tv_now));

						first_fetch = false;
					}

					ntuples = PQntuples(res);
					tupno = 0;

					slon_log(SLON_DEBUG4,
						   "remoteHelperThread_%d_%d: fetched %d log rows\n",
							 node->no_id, provider->no_id, ntuples);
				}

				/*
				 * If there are no more tuples, we're done
				 */
				if (ntuples == 0)
					break;

				/*
				 * Now move tuples from the fetch result into the line
				 * buffers.
				 */
				line_no = data_line_last++;
				line_ncmds = 0;

				line = data_line[line_no];
				line->code = SLON_WGLC_ACTION;
				line->provider = provider;
				dstring_reset(&(line->data));
				dstring_reset(&(line->log));

				while (tupno < ntuples && line_no < data_line_alloc)
				{
					char	   *log_origin = PQgetvalue(res, tupno, 0);
					char	   *log_txid = PQgetvalue(res, tupno, 1);
					int			log_tableid = strtol(PQgetvalue(res, tupno, 2),
													 NULL, 10);
					char	   *log_actionseq = PQgetvalue(res, tupno, 3);
					char	   *log_cmdtype = PQgetvalue(res, tupno, 4);
					int			log_cmdsize = strtol(PQgetvalue(res, tupno, 5),
													 NULL, 10);
					char	   *log_cmddata = PQgetvalue(res, tupno, 6);
					int			largemem = 0;

					tupno++;

					if (log_cmdsize >= sync_max_rowsize)
					{
						(void) slon_mkquery(&query2,
											"select log_cmddata "
											"from %s.sl_log_1 "
											"where log_origin = '%s' "
											"  and log_txid = '%s' "
											"  and log_actionseq = '%s' "
											"UNION ALL "
											"select log_cmddata "
											"from %s.sl_log_2 "
											"where log_origin = '%s' "
											"  and log_txid = '%s' "
											"  and log_actionseq = '%s'",
											rtcfg_namespace,
										 log_origin, log_txid, log_actionseq,
											rtcfg_namespace,
										log_origin, log_txid, log_actionseq);
						start_monitored_event(&pm);
						res2 = PQexec(dbconn, dstring_data(&query2));
						monitor_largetuples(&pm);

						if (PQresultStatus(res2) != PGRES_TUPLES_OK)
						{
							slon_log(SLON_ERROR, "remoteHelperThread_%d_%d: \"%s\" %s",
									 node->no_id, provider->no_id,
									 dstring_data(&query),
									 PQresultErrorMessage(res2));
							PQclear(res2);
							errors++;
							break;
						}
						if (PQntuples(res2) != 1)
						{
							slon_log(SLON_ERROR, "remoteHelperThread_%d_%d: large log_cmddata for actionseq %s not found\n",
									 node->no_id, provider->no_id,
									 log_actionseq);
							PQclear(res2);
							errors++;
							break;
						}

						log_cmddata = PQgetvalue(res2, 0, 0);
						largemem = log_cmdsize;
					}

					/*
					 * This can happen if the table belongs to a set that
					 * already has a better sync status than the event we're
					 * currently processing as a result from another SYNC
					 * occuring before we had started processing the copy_set.
					 */
					if (log_tableid >= wd->tab_fqname_size ||
						wd->tab_fqname[log_tableid] == NULL)
					{
						if (largemem > 0)
							PQclear(res2);
						continue;
					}

					/*
					 * If we are forwarding this set, add the insert into
					 * sl_log_?
					 */
					if (wd->tab_forward[log_tableid])
					{
						slon_appendquery(&(line->log),
										 "insert into %s.sl_log_%d "
								   "    (log_origin, log_txid, log_tableid, "
										 "     log_actionseq, log_cmdtype, "
										 "     log_cmddata) values "
							   "    ('%s', '%s', '%d', '%s', '%q', '%q');\n",
									   rtcfg_namespace, wd->active_log_table,
										 log_origin, log_txid, log_tableid,
									log_actionseq, log_cmdtype, log_cmddata);
						largemem *= 2;
					}

					/*
					 * Add the actual replicating command to the line buffer
					 */
					line->line_largemem += largemem;
					switch (*log_cmdtype)
					{
						case 'I':
							slon_appendquery(&(line->data),
											 "insert into %s %s;\n",
											 wd->tab_fqname[log_tableid],
											 log_cmddata);
							pm.num_inserts++;
							break;

						case 'U':
							slon_appendquery(&(line->data),
											 "update only %s set %s;\n",
											 wd->tab_fqname[log_tableid],
											 log_cmddata);
							pm.num_updates++;
							break;

						case 'D':
							slon_appendquery(&(line->data),
										   "delete from only %s where %s;\n",
											 wd->tab_fqname[log_tableid],
											 log_cmddata);
							pm.num_deletes++;
							break;
						case 'T':
							slon_appendquery(&(line->data),
											 "%s;\n",
											 log_cmddata);
							pm.num_truncates++;
							break;
					}
					line_ncmds++;

					if (line_ncmds >= SLON_COMMANDS_PER_LINE)
					{
						if (data_line_last >= data_line_alloc)
						{
							if (largemem > 0)
								PQclear(res2);
							break;
						}

						line_no = data_line_last++;

						line = data_line[line_no];
						line->code = SLON_WGLC_ACTION;
						line->provider = provider;
						dstring_reset(&(line->data));
						dstring_reset(&(line->log));

						line_ncmds = 0;
					}

					/*
					 * If this was a large log_cmddata entry (>
					 * sync_max_rowsize), add this to the memory usage of the
					 * workgroup and check if we are exceeding limits.
					 */
					if (largemem > 0)
					{
						PQclear(res2);
						pthread_mutex_lock(&(wd->workdata_lock));
						wd->workdata_largemem += largemem;
						if (wd->workdata_largemem >= sync_max_largemem)
						{
							/*
							 * This is it ... we exit the loop here and wait
							 * for the worker to apply enough of the large
							 * rows first.
							 */
							pthread_mutex_unlock(&(wd->workdata_lock));
							break;
						}
						pthread_mutex_unlock(&(wd->workdata_lock));
					}
				}

				/*
				 * Move one line back if we actually ran out of tuples on an
				 * exact SLON_COMMANDS_PER_LINE boundary.
				 */
				if (line_ncmds == 0)
				{
					data_line_last--;
				}
			}					/* Cursor returned EOF */
		} while (0);

		/*
		 * if there are still line buffers allocated, give them back.
		 */
		if (data_line_first < data_line_alloc)
		{
			slon_log(SLON_DEBUG4,
				 "remoteHelperThread_%d_%d: return %d unused line buffers\n",
					 node->no_id, provider->no_id,
					 data_line_alloc - data_line_first);
			pthread_mutex_lock(&(wd->workdata_lock));
			while (data_line_first < data_line_alloc)
			{
				data_line_alloc--;
				DLLIST_ADD_HEAD(wd->linepool_head, wd->linepool_tail,
								data_line[data_line_alloc]);
			}
			pthread_cond_broadcast(&(wd->linepool_cond));
			pthread_mutex_unlock(&(wd->workdata_lock));

			data_line_alloc = 0;
			data_line_first = 0;
			data_line_last = 0;
		}

		if (res != NULL)
		{
			PQclear(res);
			res = NULL;
		}

		/*
		 * Close the cursor and rollback the transaction.
		 */
		(void) slon_mkquery(&query, "close LOG; ");
		if (query_execute(node, dbconn, &query) < 0)
			errors++;
		(void) slon_mkquery(&query, "rollback transaction; "
							"set enable_seqscan = default; "
							"set enable_indexscan = default; ");
		if (query_execute(node, dbconn, &query) < 0)
			errors++;

		gettimeofday(&tv_now, NULL);
		slon_log(SLON_DEBUG1,
			   "remoteHelperThread_%d_%d: %.3f seconds until close cursor\n",
				 node->no_id, provider->no_id,
				 TIMEVAL_DIFF(&tv_start, &tv_now));

		slon_log(SLON_DEBUG1, "remoteHelperThread_%d_%d: inserts=%d updates=%d deletes=%d truncates=%d\n",
				 node->no_id, provider->no_id, pm.num_inserts, pm.num_updates, pm.num_deletes, pm.num_truncates);

		slon_log(SLON_DEBUG1, 
				 "remoteWorkerThread_%d: sync_helper timing: " 
				 " pqexec (s/count)" 
				 "- provider %.3f/%d " 
				 "- subscriber %.3f/%d\n",
				 node->no_id, 
				 pm.prov_query_t, pm.prov_query_c, 
				 pm.subscr_query_t, pm.prov_query_c);

		slon_log(SLON_DEBUG1, 
				 "remoteWorkerThread_%d: sync_helper timing: " 
				 " large tuples %.3f/%d\n", 
				 node->no_id, 
				 pm.large_tuples_t, pm.large_tuples_c);

		/*
		 * Change our helper status to DONE and tell the worker thread about
		 * it.
		 */
		slon_log(SLON_DEBUG4,
				 "remoteHelperThread_%d_%d: change helper thread status\n",
				 node->no_id, provider->no_id);
		pthread_mutex_lock(&(provider->helper_lock));
		provider->helper_status = SLON_WG_DONE;
		dstring_reset(&provider->helper_query);
		pthread_mutex_unlock(&(provider->helper_lock));

		slon_log(SLON_DEBUG4,
				 "remoteHelperThread_%d_%d: send DONE/ERROR line to worker\n",
				 node->no_id, provider->no_id);
		pthread_mutex_lock(&(wd->workdata_lock));
		while (wd->linepool_head == NULL)
		{
			pthread_cond_wait(&(wd->linepool_cond), &(wd->workdata_lock));
		}
		line = wd->linepool_head;
		DLLIST_REMOVE(wd->linepool_head, wd->linepool_tail, line);
		if (errors)
			line->code = SLON_WGLC_ERROR;
		else
			line->code = SLON_WGLC_DONE;
		line->provider = provider;
		DLLIST_ADD_HEAD(wd->repldata_head, wd->repldata_tail, line);
		pthread_cond_signal(&(wd->repldata_cond));
		pthread_mutex_unlock(&(wd->workdata_lock));

		/*
		 * Wait for the whole workgroup to be done.
		 */
		pthread_mutex_lock(&(provider->helper_lock));
		while (provider->helper_status == SLON_WG_DONE)
		{
			slon_log(SLON_DEBUG3, "remoteHelperThread_%d_%d: "
					 "waiting for workgroup to finish\n",
					 node->no_id, provider->no_id);

			pthread_cond_wait(&(provider->helper_cond), &(provider->helper_lock));
		}
		pthread_mutex_unlock(&(provider->helper_lock));
	}
}

/* ----------
 * Functions for processing log archives...
 *
 * - First, you open the log archive using archive_open()
 *
 * - Second, you generate the header using generate_archive_header()
 *
 * ========  Here Ends The Header of the Log Shipping Archive ========
 *
 * Then come the various queries (inserts/deletes/updates) that
 * comprise the "body" of the SYNC.  Probably submitted using
 * submit_query_to_archive().
 *
 * ========  Here Ends The Body of the Log Shipping Archive ========
 *
 * Finally, the log ends, notably with a COMMIT statement, generated
 * using close_log_archive(), which closes the file and renames it
 * from ".tmp" form to the final name.
 * ----------
 */


/* ----------
 * archive_open
 *
 * Stores the archive name in archive_name (as .sql name) and
 * archive_tmp (.tmp file)
 * ----------
 */
static int
archive_open(SlonNode *node, char *seqbuf, PGconn *dbconn)
{
	SlonDString query;
	PGresult   *res;
	int			i;
	int			rc;

	if (!archive_dir)
		return 0;

	if (node->archive_name == NULL)
	{
		node->archive_name = malloc(SLON_MAX_PATH);
		node->archive_temp = malloc(SLON_MAX_PATH);
		node->archive_counter = malloc(64);
		node->archive_timestamp = malloc(256);
		if (node->archive_name == NULL || node->archive_temp == NULL ||
			node->archive_counter == NULL || node->archive_timestamp == NULL)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
					 "Out of memory in archive_open()\n",
					 node->no_id);
			return -1;
		}
	}

	if (node->archive_fp != NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "archive_open() called - archive is already opened\n",
				 node->no_id);
		return -1;
	}

	dstring_init(&query);
	slon_mkquery(&query,
				 "update %s.sl_archive_counter "
				 "    set ac_num = ac_num + 1, "
				 "        ac_timestamp = CURRENT_TIMESTAMP; "
				 "select ac_num, ac_timestamp from %s.sl_archive_counter; ",
				 rtcfg_namespace, rtcfg_namespace);
	res = PQexec(dbconn, dstring_data(&query));
	if ((rc = PQresultStatus(res)) != PGRES_TUPLES_OK)
	{
		/* see what kind of error it is... */
#define CONCUPDATEMSG "ERROR:  could not serialize access due to concurrent update"
		if (strncmp(CONCUPDATEMSG, PQresultErrorMessage(res), strlen(CONCUPDATEMSG)) == 0)
		{
			slon_log(SLON_WARN, "serialization problem updating sl_archive_counter: restarting slon\n");
			slon_mkquery(&query,
						 "notify \"_%s_Restart\"; ",
						 rtcfg_cluster_name);
			PQexec(dbconn, dstring_data(&query));
		}
		else
		{

			slon_log(SLON_WARN, "error message was [%s]\n", PQresultErrorMessage(res));
			slon_log(SLON_ERROR,
					 "remoteWorkerThread_%d: \"%s\" %s %s\n",
					 node->no_id, dstring_data(&query),
					 PQresStatus(rc),
					 PQresultErrorMessage(res));
		}
		PQclear(res);
		dstring_free(&query);
		return -1;
	}
	if ((rc = PQntuples(res)) != 1)
	{
		slon_log(SLON_ERROR,
				 "remoteWorkerThread_%d: expected 1 row in sl_archive_counter - found %d",
				 node->no_id, rc);
		PQclear(res);
		dstring_free(&query);
		return -1;
	}
	strcpy(node->archive_counter, PQgetvalue(res, 0, 0));
	strcpy(node->archive_timestamp, PQgetvalue(res, 0, 1));
	PQclear(res);
	dstring_free(&query);

	sprintf(node->archive_name, "%s/slony1_log_%d_", archive_dir,
			rtcfg_nodeid);
	for (i = strlen(node->archive_counter); i < 20; i++)
		strcat(node->archive_name, "0");
	strcat(node->archive_name, node->archive_counter);
	strcat(node->archive_name, ".sql");
	strcpy(node->archive_temp, node->archive_name);
	strcat(node->archive_temp, ".tmp");
	node->archive_fp = fopen(node->archive_temp, "w");
	if (node->archive_fp == NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot open archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	rc = fprintf(node->archive_fp,
	   "------------------------------------------------------------------\n"
				 "-- Slony-I log shipping archive\n"
				 "-- Node %d, Event %s\n"
	   "------------------------------------------------------------------\n"
				 "set session_replication_role to replica;\n"
				 "start transaction;\n",
				 node->no_id, seqbuf);
	if (rc < 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}
	rc = fprintf(node->archive_fp,
				 "select %s.archiveTracking_offline('%s', '%s');\n"
				 "-- end of log archiving header\n"
	   "------------------------------------------------------------------\n"
				 "-- start of Slony-I data\n"
	  "------------------------------------------------------------------\n",
			rtcfg_namespace, node->archive_counter, node->archive_timestamp);
	if (rc < 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	return 0;
}

/* ----------
 * archive_close
 * ----------
 */
static int
archive_close(SlonNode *node)
{
	int			rc = 0;

	if (!archive_dir)
		return 0;

	if (node->archive_fp == NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot close archive file %s - not open\n",
				 node->no_id, node->archive_temp);
		return -1;
	}

	rc = fprintf(node->archive_fp,
	 "\n------------------------------------------------------------------\n"
				 "-- End Of Archive Log\n"
	   "------------------------------------------------------------------\n"
				 "commit;\n"
				 "vacuum analyze %s.sl_archive_tracking;\n",
				 rtcfg_namespace);
	if (rc < 0)
	{
		archive_terminate(node);
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	rc = fclose(node->archive_fp);
	node->archive_fp = NULL;
	if (rc != 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot close archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	rc = rename(node->archive_temp, node->archive_name);
	if (rc != 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot rename archive file %s to %s - %s\n",
				 node->no_id, node->archive_temp, node->archive_name,
				 strerror(errno));
		return -1;
	}

	if (command_on_logarchive)
	{
		char		command[1024];

		sprintf(command, "%s %s", command_on_logarchive, node->archive_name);
		slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: Run Archive Command %s\n",
				 node->no_id, command);
		system(command);
	}

	return 0;
}

/* ----------
 * archive_terminate
 * ----------
 */
static void
archive_terminate(SlonNode *node)
{
	if (node->archive_fp != NULL)
	{
		fclose(node->archive_fp);
		node->archive_fp = NULL;
	}
}

/* ----------
 * archive_append_ds
 * ----------
 */
static int
archive_append_ds(SlonNode *node, SlonDString *ds)
{
	int			rc;

	if (!archive_dir)
		return 0;

	if (node->archive_fp == NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - not open\n",
				 node->no_id, node->archive_temp);
		return -1;
	}

	rc = fprintf(node->archive_fp, "%s\n", dstring_data(ds));
	if (rc < 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	return 0;
}

/* ----------
 * archive_append_str
 * ----------
 */
static int
archive_append_str(SlonNode *node, const char *s)
{
	int			rc;

	if (!archive_dir)
		return 0;

	if (node->archive_fp == NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - not open\n",
				 node->no_id, node->archive_temp);
		return -1;
	}

	rc = fprintf(node->archive_fp, "%s\n", s);
	if (rc < 0)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	return 0;
}

/* ----------
 * archive_append_data
 *
 * Raw form used for COPY where we don't want any extra cr/lf output
 * ----------
 */
static int
archive_append_data(SlonNode *node, const char *s, int len)
{
	int			rc;

	if (!archive_dir)
		return 0;

	if (node->archive_fp == NULL)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - not open\n",
				 node->no_id, node->archive_temp);
		return -1;
	}

	rc = fwrite(s, len, 1, node->archive_fp);
	if (rc != 1)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s\n",
				 node->no_id, node->archive_temp, strerror(errno));
		return -1;
	}

	return 0;
}

/* ----------
 * given a string consisting of a list of actionseq values, return a
 * string that compresses this into a set of log_actionseq ranges
 *
 * Thus, "'13455','13456','13457','13458','13459','13460','13462'"
 * compresses into...
 *
 * log_actionseq not between '13455' and '13460' and
 * log_actionseq <> '13462'
 *
 * There is an expectation that the actionseq values are being
 * returned more or less in order; if that is even somewhat loosely
 * the case, this will lead to a pretty spectacular compression of
 * values if the SUBSCRIBE_SET runs for a long time thereby leading to
 * there being Really A Lot of log entries to exclude.
 * ----------
 */

typedef enum
{
		START_STATE,
		COLLECTING_DIGITS,
		BETWEEN_NUMBERS,
		DONE
} CompressState;

#define MINMAXINITIAL -1

/* ----------
 * compress_actionseq
 * ----------
 */
void
compress_actionseq(const char *ssy_actionlist, SlonDString *action_subquery)
{
	CompressState			state;
	int64			curr_number,
				curr_min,
				curr_max;
	int			curr_digit;
	int			first_subquery;
	char		curr_char;

	curr_number = 0;
	curr_min = MINMAXINITIAL;
	curr_max = MINMAXINITIAL;
	first_subquery = 1;
	state = START_STATE;
	(void) slon_mkquery(action_subquery, " ");

	slon_log(SLON_DEBUG4, "compress_actionseq(list,subquery) Action list: %s\n", ssy_actionlist);
	while (state != DONE)
	{
		curr_char = *ssy_actionlist;
		switch (curr_char)
		{
			case '\0':
				state = DONE;
				break;
			case '0':
				curr_digit = 0;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '1':
				curr_digit = 1;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '2':
				curr_digit = 2;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '3':
				curr_digit = 3;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '4':
				curr_digit = 4;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '5':
				curr_digit = 5;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '6':
				curr_digit = 6;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '7':
				curr_digit = 7;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '8':
				curr_digit = 8;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '9':
				curr_digit = 9;
				if (state == COLLECTING_DIGITS)
				{
					curr_number = curr_number * 10 + curr_digit;
				}
				else
				{
					state = COLLECTING_DIGITS;
					curr_number = curr_digit;
				}
				break;
			case '\'':
			case ',':
				if (state == COLLECTING_DIGITS)
				{
					/* Finished another number... Fold it into the ranges... */
					slon_log(SLON_DEBUG4, "Finished number: %lld\n", curr_number);

					/*
					 * If we haven't a range, then the range is the current
					 * number
					 */
					if (curr_min == MINMAXINITIAL)
					{
						curr_min = curr_number;
					}
					if (curr_max == MINMAXINITIAL)
					{
						curr_max = curr_number;
					}

					/*
					 * If the number pushes the range outwards by 1, then
					 * shift the range by 1...
					 */
					if (curr_number == curr_min - 1)
					{
						curr_min--;
					}
					if (curr_number == curr_max + 1)
					{
						curr_max++;
					}

					/* If the number is inside the range, do nothing */
					if ((curr_number >= curr_min) && (curr_number <= curr_max))
					{
						/* Do nothing - inside the range */
					}

					/*
					 * If the number is outside the range, then generate a
					 * subquery based on the range, and have the new number
					 * become the new range
					 */
					if ((curr_number < curr_min - 1) || (curr_number > curr_max + 1))
					{
						if (first_subquery)
						{
							first_subquery = 0;
						}
						else
						{
							slon_appendquery(action_subquery, " and ");
						}
						if (curr_max == curr_min)
						{
							slon_log(SLON_DEBUG4, "simple entry - %lld\n", curr_max);
							slon_appendquery(action_subquery,
										" log_actionseq <> '%L' ", curr_max);
						}
						else
						{
							slon_log(SLON_DEBUG4, "between entry - %lld %lld\n",
									 curr_min, curr_max);
							slon_appendquery(action_subquery,
								 " log_actionseq not between '%L' and '%L' ",
											 curr_min, curr_max);
						}
						curr_min = curr_number;
						curr_max = curr_number;
					}
				}
				state = BETWEEN_NUMBERS;
				curr_number = 0;

		}
		ssy_actionlist++;
	}
	/* process last range, if it exists */
	if (curr_min || curr_max)
	{
		if (first_subquery)
		{
			first_subquery = 0;
		}
		else
		{
			slon_appendquery(action_subquery, " and ");
		}
		if (curr_max == curr_min)
		{
			slon_log(SLON_DEBUG4, "simple entry - %lld\n", curr_max);
			slon_appendquery(action_subquery,
							 " log_actionseq <> '%L' ", curr_max);
		}
		else
		{
			slon_log(SLON_DEBUG4, "between entry - %lld %lld\n",
					 curr_min, curr_max);
			slon_appendquery(action_subquery,
							 " log_actionseq not between '%L' and '%L' ",
							 curr_min, curr_max);
		}


	}
	slon_log(SLON_DEBUG4, " compressed actionseq subquery... %s\n", dstring_data(action_subquery));
}

/**
 * Checks to see if the node specified is a member of the set.
 *
 */
static int
check_set_subscriber(int set_id, int node_id, PGconn *local_dbconn)
{
	SlonDString query1;
	PGresult   *res;

	dstring_init(&query1);

	slon_appendquery(&query1, "select 1 from %s.sl_subscribe WHERE sub_set=%d AND sub_receiver=%d for update"
					 ,rtcfg_namespace, set_id, node_id);
	res = PQexec(local_dbconn, dstring_data(&query1));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: DDL preparation can not check set membership"
				 ,node_id);
		dstring_free(&query1);
		slon_retry();
	}
	dstring_free(&query1);
	if (PQntuples(res) == 0)
	{
		PQclear(res);
		return 0;
	}
	PQclear(res);
	return 1;
}

static void init_perfmon(PerfMon *perf_info) {
  perf_info->prov_query_t = 0.0;
  perf_info->prov_query_c = 0;
  perf_info->subscr_query_t = 0.0;
  perf_info->subscr_query_c = 0;
  perf_info->subscr_iud__t = 0.0;
  perf_info->subscr_iud__c = 0;
  perf_info->large_tuples_t = 0;
  perf_info->large_tuples_c = 0;
  perf_info->num_inserts = 0;
  perf_info->num_updates = 0;
  perf_info->num_deletes = 0;
  perf_info->num_truncates = 0;
}
static void start_monitored_event(PerfMon *perf_info) {
  gettimeofday(&(perf_info->prev_t), NULL);
}
static void monitor_subscriber_query(PerfMon *perf_info) {
  double diff;
  gettimeofday(&(perf_info->now_t), NULL);
  diff = TIMEVAL_DIFF(&(perf_info->prev_t), &(perf_info->now_t)); 
  (perf_info->subscr_query_t) += diff;
  (perf_info->subscr_query_c) ++;
}
static void monitor_provider_query(PerfMon *perf_info) {
  double diff;
  gettimeofday(&(perf_info->now_t), NULL);
  diff = TIMEVAL_DIFF(&(perf_info->prev_t), &(perf_info->now_t)); 
  (perf_info->prov_query_t) += diff;
  (perf_info->prov_query_c) ++;
}
static void monitor_subscriber_iud(PerfMon *perf_info) {
  double diff;
  gettimeofday(&(perf_info->now_t), NULL);
  diff = TIMEVAL_DIFF(&(perf_info->prev_t), &(perf_info->now_t)); 
  (perf_info->subscr_iud__t) += diff;
  (perf_info->subscr_iud__c) ++;
}

static void monitor_largetuples(PerfMon *perf_info) {
  double diff;
  gettimeofday(&(perf_info->now_t), NULL);
  diff = TIMEVAL_DIFF(&(perf_info->prev_t), &(perf_info->now_t)); 
  (perf_info->large_tuples_t) += diff;
  (perf_info->large_tuples_c) ++;
}
