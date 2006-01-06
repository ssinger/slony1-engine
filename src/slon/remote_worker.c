/*-------------------------------------------------------------------------
 * remote_worker.c
 *
 *	Implementation of the thread processing remote events.
 *
 *	Copyright (c) 2003-2006, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: remote_worker.c,v 1.86.2.12 2006-01-06 17:07:47 cbbrowne Exp $
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
#include "c.h"

#include "slon.h"
#include "confoptions.h"


/*
 * ---------- Local definitions ----------
 */

/*
 * Internal message types
 */
#define WMSG_EVENT		0
#define WMSG_WAKEUP		1
#define WMSG_CONFIRM	2


/*
 * Message structure resulting from a remote event
 */
typedef struct SlonWorkMsg_event_s SlonWorkMsg_event;
struct SlonWorkMsg_event_s
{
	int			msg_type;
	SlonWorkMsg_event *prev;
	SlonWorkMsg_event *next;

	int			event_provider;

	int			ev_origin;
	int64		ev_seqno;
	char	   *ev_timestamp_c;
	char	   *ev_minxid_c;
	char	   *ev_maxxid_c;
	char	   *ev_xip;
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
	int			msg_type;
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
	int			msg_type;
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
}	WorkGroupStatus;


typedef enum
{
	SLON_WGLC_ACTION,
	SLON_WGLC_DONE,
	SLON_WGLC_ERROR
}	WorkGroupLineCode;


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
	SlonDString helper_qualification;

	ProviderSet *set_head;
	ProviderSet *set_tail;

	ProviderInfo *prev;
	ProviderInfo *next;
};


struct WorkerGroupData_s
{
	SlonNode   *node;

	char	   *tab_forward;
	char	  **tab_fqname;
	int			tab_fqname_size;

	ProviderInfo *provider_head;
	ProviderInfo *provider_tail;

	pthread_mutex_t workdata_lock;
	WorkGroupStatus workgroup_status;

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
pthread_mutex_t node_confirm_lock = PTHREAD_MUTEX_INITIALIZER;

int sync_group_maxsize;

int last_sync_group_size;
int next_sync_group_size;

int desired_sync_time;
int ideal_sync ;
struct timeval  sync_start;
struct timeval  sync_end;
int last_sync_length;
int max_sync;
int min_sync;
/*
 * ---------- Local functions ----------
 */
static void adjust_provider_info(SlonNode * node,
					 WorkerGroupData * wd, int cleanup);
static int query_execute(SlonNode * node, PGconn *dbconn,
			  SlonDString * dsp);
static void query_append_event(SlonDString * dsp,
				   SlonWorkMsg_event * event);
static void store_confirm_forward(SlonNode * node, SlonConn * conn,
					  SlonWorkMsg_confirm * confirm);
static int64 get_last_forwarded_confirm(int origin, int receiver);
static int copy_set(SlonNode * node, SlonConn * local_conn, int set_id,
		 SlonWorkMsg_event * event);
static int sync_event(SlonNode * node, SlonConn * local_conn,
		   WorkerGroupData * wd, SlonWorkMsg_event * event);
static void *sync_helper(void *cdata);

static char archive_name[SLON_MAX_PATH];
static char archive_tmp[SLON_MAX_PATH];
static FILE *archive_fp = NULL;
static int open_log_archive (int node_id, char *seqbuf);
static int close_log_archive ();
static void terminate_log_archive ();
static int generate_archive_header (int node_id, const char *seqbuf);
static int submit_query_to_archive(SlonDString *ds);
static int submit_string_to_archive (const char *s);
static int submit_raw_data_to_archive (const char *s);
static int logarchive_tracking (const char *namespace, int sub_set, const char *firstseq, 
				const char *seqbuf, const char *timestamp);
static void write_void_log (int node_id, char *seqbuf, const char *message);

#define TERMINATE_QUERY_AND_ARCHIVE dstring_free(&query); terminate_log_archive();

/*
 * ---------- slon_remoteWorkerThread
 *
 * Listen for events on the local database connection. This means, events
 * generated by the local node only. ----------
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
	SlonWorkMsg *msg;
	SlonWorkMsg_event *event;
	int			check_config = true;
	int64		curr_config = -1;
	char		seqbuf [64];
	int			event_ok;
	int			need_reloadListen = false;
	int rc;

	slon_log(SLON_DEBUG1,
			 "remoteWorkerThread_%d: thread starts\n",
			 node->no_id);

	/*
	 * Initialize local data
	 */
	wd = (WorkerGroupData *) malloc(sizeof(WorkerGroupData));
	memset(wd, 0, sizeof(WorkerGroupData));

	pthread_mutex_init(&(wd->workdata_lock), NULL);
	pthread_cond_init(&(wd->repldata_cond), NULL);
	pthread_cond_init(&(wd->linepool_cond), NULL);
	pthread_mutex_lock(&(wd->workdata_lock));
	wd->workgroup_status = SLON_WG_IDLE;
	wd->node = node;

	wd->tab_fqname_size = SLON_MAX_PATH;
	wd->tab_fqname = (char **)malloc(sizeof(char *) * wd->tab_fqname_size);
	memset(wd->tab_fqname, 0, sizeof(char *) * wd->tab_fqname_size);
	wd->tab_forward = malloc(wd->tab_fqname_size);
	memset(wd->tab_forward, 0, wd->tab_fqname_size);

	dstring_init(&query1);
	dstring_init(&query2);

	/*
	 * Connect to the local database
	 */
	if ((local_conn = slon_connectdb(rtcfg_conninfo, "remote_worker")) == NULL)
		slon_abort();
	local_dbconn = local_conn->dbconn;

	/*
	 * Put the connection into replication mode
	 */
	slon_mkquery(&query1,
				 "select %s.setSessionRole('_%s', 'slon'); ",
				 rtcfg_namespace, rtcfg_cluster_name);
	if (query_execute(node, local_dbconn, &query1) < 0)
		slon_abort();

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
				break;
			if (node->worker_status != SLON_TSTAT_RUNNING)
				break;

			if (curr_config != rtcfg_seq_get())
			{
				adjust_provider_info(node, wd, false);
				curr_config = rtcfg_seq_get();
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
				slon_abort();
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
			slon_abort();
		}
		event_ok = true;
		event = (SlonWorkMsg_event *) msg;
		sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);

		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "Received event %d,%s %s\n",
				 node->no_id, event->ev_origin, seqbuf,
				 event->ev_type);

		/*
		 * Construct the queries to begin a transaction, notify on the event
		 * and confirm relations, insert the event into our local sl_event
		 * table and confirm it in our local sl_confirm table. When this
		 * transaction commits, every other remote node listening for events
		 * with us as a provider will pick up the news.
		 */
		slon_mkquery(&query1,
					 "begin transaction; "
					 "set transaction isolation level serializable; ");

		/*
		 * Event type specific processing
		 */
		if (strcmp(event->ev_type, "SYNC") == 0)
		{
			SlonWorkMsg_event *sync_group[100];
			int			sync_group_size;

			int			seconds;
			int			rc;
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
				if (last_sync_group_size > 100)
					last_sync_group_size = 1;

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
					if (desired_sync_time != 0) {
						ideal_sync = (last_sync_group_size * desired_sync_time) / last_sync_length;
					} else {
						ideal_sync = sync_group_maxsize;
					}
					max_sync = ((last_sync_group_size * 110) / 100) + 1;
					next_sync_group_size = ideal_sync;
					if (next_sync_group_size > max_sync)
						next_sync_group_size = max_sync;
					if (next_sync_group_size < 1)
						next_sync_group_size = 1;
					slon_log(SLON_DEBUG3, "calc sync size - last time: %d last length: %d ideal: %d proposed size: %d\n",
						 last_sync_group_size, last_sync_length, ideal_sync, next_sync_group_size);
				}

				gettimeofday(&sync_start, NULL);

				pthread_mutex_lock(&(node->message_lock));
				while (sync_group_size < next_sync_group_size && node->message_head != NULL)
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
					last_sync_group_size ++;
				}
				pthread_mutex_unlock(&(node->message_lock));
			}
			while (true)
			{
				/*
				 * Execute the forwarding and notify stuff, but do not commit
				 * the transaction yet.
				 */
				if (query_execute(node, local_dbconn, &query1) < 0)
					slon_abort();

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
				slon_mkquery(&query2, "rollback transaction");
				if (query_execute(node, local_dbconn, &query2) < 0)
					slon_abort();

				if ((rc = sched_msleep(node, seconds * 1000)) != SCHED_STATUS_OK)
					break;
			}
			if (rc != SCHED_STATUS_OK)
				break;

			/*
			 * replace query1 with the forwarding of all the grouped sync
			 * events and a commit. Also free all the WMSG structres except
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
				slon_abort();
		}
		else
		{
			/*
			 * Avoid deadlock problems during configuration changes by locking
			 * the central confiuration lock right from the start.
			 */
			slon_appendquery(&query1,
							 "lock table %s.sl_config_lock; ",
							 rtcfg_namespace);

			/*
			 * Simple configuration events. Call the corresponding runtime
			 * config function, add the query to call the configuration event
			 * specific stored procedure.
			 */
			if (strcmp(event->ev_type, "STORE_NODE") == 0)
			{
				int			no_id = (int)strtol(event->ev_data1, NULL, 10);
				char	   *no_comment = event->ev_data2;
				char	   *no_spool = event->ev_data3;

				if (no_id != rtcfg_nodeid)
					rtcfg_storeNode(no_id, no_comment);

				slon_appendquery(&query1,
						 "select %s.storeNode_int(%d, '%q', '%s'); ",
						 rtcfg_namespace,
						 no_id, no_comment, no_spool);
				
				need_reloadListen = true;
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- STORE_NODE");

			}
			else if (strcmp(event->ev_type, "ENABLE_NODE") == 0)
			{
				int			no_id = (int)strtol(event->ev_data1, NULL, 10);

				if (no_id != rtcfg_nodeid)
					rtcfg_enableNode(no_id);

				slon_appendquery(&query1,
								 "select %s.enableNode_int(%d); ",
								 rtcfg_namespace,
								 no_id);

				need_reloadListen = true;

				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- ENABLE_NODE");
			}
			else if (strcmp(event->ev_type, "DROP_NODE") == 0)
			{
				int			no_id = (int)strtol(event->ev_data1, NULL, 10);

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
						slon_abort();

					slon_mkquery(&query1, "select %s.uninstallNode(); ",
								 rtcfg_namespace);
					if (query_execute(node, local_dbconn, &query1) < 0)
						slon_abort();

					slon_mkquery(&query1, "drop schema %s cascade; ",
								 rtcfg_namespace);
					query_execute(node, local_dbconn, &query1);

					slon_abort();
				}

				/*
				 * this is a remote node. Arrange for daemon restart.
				 */
				slon_appendquery(&query1,
								 "notify \"_%s_Restart\"; ",
								 rtcfg_cluster_name);

				need_reloadListen = true;
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- DROP_NODE");
			}
			else if (strcmp(event->ev_type, "STORE_PATH") == 0)
			{
				int			pa_server = (int)strtol(event->ev_data1, NULL, 10);
				int			pa_client = (int)strtol(event->ev_data2, NULL, 10);
				char	   *pa_conninfo = event->ev_data3;
				int			pa_connretry = (int)strtol(event->ev_data4, NULL, 10);

				if (pa_client == rtcfg_nodeid)
					rtcfg_storePath(pa_server, pa_conninfo, pa_connretry);

				slon_appendquery(&query1,
							   "select %s.storePath_int(%d, %d, '%q', %d); ",
								 rtcfg_namespace,
							pa_server, pa_client, pa_conninfo, pa_connretry);

				need_reloadListen = true;
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- STORE_PATH");
			}
			else if (strcmp(event->ev_type, "DROP_PATH") == 0)
			{
				int			pa_server = (int)strtol(event->ev_data1, NULL, 10);
				int			pa_client = (int)strtol(event->ev_data2, NULL, 10);

				if (pa_client == rtcfg_nodeid)
					rtcfg_dropPath(pa_server);

				slon_appendquery(&query1,
								 "select %s.dropPath_int(%d, %d); ",
								 rtcfg_namespace,
								 pa_server, pa_client);

				need_reloadListen = true;
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- DROP_PATH");
			}
			else if (strcmp(event->ev_type, "STORE_LISTEN") == 0)
			{
				int			li_origin = (int)strtol(event->ev_data1, NULL, 10);
				int			li_provider = (int)strtol(event->ev_data2, NULL, 10);
				int			li_receiver = (int)strtol(event->ev_data3, NULL, 10);

				if (li_receiver == rtcfg_nodeid)
					rtcfg_storeListen(li_origin, li_provider);

				slon_appendquery(&query1,
								 "select %s.storeListen_int(%d, %d, %d); ",
								 rtcfg_namespace,
								 li_origin, li_provider, li_receiver);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- STORE_LISTEN");
			}
			else if (strcmp(event->ev_type, "DROP_LISTEN") == 0)
			{
				int			li_origin = (int)strtol(event->ev_data1, NULL, 10);
				int			li_provider = (int)strtol(event->ev_data2, NULL, 10);
				int			li_receiver = (int)strtol(event->ev_data3, NULL, 10);

				if (li_receiver == rtcfg_nodeid)
					rtcfg_dropListen(li_origin, li_provider);

				slon_appendquery(&query1,
								 "select %s.dropListen_int(%d, %d, %d); ",
								 rtcfg_namespace,
								 li_origin, li_provider, li_receiver);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- DROP_LISTEN");
			}
			else if (strcmp(event->ev_type, "STORE_SET") == 0)
			{
				int			set_id = (int)strtol(event->ev_data1, NULL, 10);
				int			set_origin = (int)strtol(event->ev_data2, NULL, 10);
				char	   *set_comment = event->ev_data3;

				if (set_origin != rtcfg_nodeid)
					rtcfg_storeSet(set_id, set_origin, set_comment);

				slon_appendquery(&query1,
								 "select %s.storeSet_int(%d, %d, '%q'); ",
								 rtcfg_namespace,
								 set_id, set_origin, set_comment);

				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- STORE_SET");
			}
			else if (strcmp(event->ev_type, "DROP_SET") == 0)
			{
				int			set_id = (int)strtol(event->ev_data1, NULL, 10);

				rtcfg_dropSet(set_id);

				slon_appendquery(&query1,
						 "select %s.dropSet_int(%d); ",
						 rtcfg_namespace, set_id);

				/* The table deleted needs to be
				 * dropped from log shipping too */
				if (archive_dir) {
				    rc = open_log_archive(rtcfg_nodeid, seqbuf);
				    rc = generate_archive_header(rtcfg_nodeid, seqbuf);
				    slon_mkquery(&query1, 
						 "delete from %s.sl_setsync_offline "
						 "  where ssy_setid= %d;",
						 rtcfg_namespace, set_id);
				    rc = submit_query_to_archive(&query1);
				    rc = close_log_archive();
				}
			}
			else if (strcmp(event->ev_type, "MERGE_SET") == 0)
			{
				int set_id = (int)strtol(event->ev_data1, NULL, 10);
				int add_id = (int)strtol(event->ev_data2, NULL, 10);
				rtcfg_dropSet(add_id);

				slon_appendquery(&query1,
						 "select %s.mergeSet_int(%d, %d); ",
						 rtcfg_namespace,
						 set_id, add_id);
				
				/* Log shipping gets the change here
				 * that we need to delete the table
				 * being merged from the set being
				 * maintained. */
				if (archive_dir) {
				    rc = open_log_archive(rtcfg_nodeid, seqbuf);
				    rc = generate_archive_header(rtcfg_nodeid, seqbuf);
				    rc = slon_mkquery(&query1, 
						      "delete from %s.sl_setsync_offline "
						      "  where ssy_setid= %d;",
						      rtcfg_namespace, add_id);
				    rc = submit_query_to_archive(&query1);
				    rc = close_log_archive();
				}
			}
			else if (strcmp(event->ev_type, "SET_ADD_TABLE") == 0)
			{
				/*
				 * Nothing to do ATM ... we don't support adding tables to
				 * subscribed sets yet and table information is not maintained
				 * in the runtime configuration.
				 */
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_ADD_TABLE");
			}
			else if (strcmp(event->ev_type, "SET_ADD_SEQUENCE") == 0)
			{
				/*
				 * Nothing to do ATM ... we don't support adding sequences to
				 * subscribed sets yet and sequences information is not
				 * maintained in the runtime configuration.
				 */
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_ADD_SEQUENCE");
			}
			else if (strcmp(event->ev_type, "SET_DROP_TABLE") == 0)
			{
				int			tab_id = (int)strtol(event->ev_data1, NULL, 10);

				slon_appendquery(&query1, "select %s.setDropTable_int(%d);",
								 rtcfg_namespace,
								 tab_id);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_DROP_TABLE");
			}
			else if (strcmp(event->ev_type, "SET_DROP_SEQUENCE") == 0)
			{
				int			seq_id = (int)strtol(event->ev_data1, NULL, 10);

				slon_appendquery(&query1, "select %s.setDropSequence_int(%d);",
								 rtcfg_namespace,
								 seq_id);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_DROP_SEQUENCE");
			}
			else if (strcmp(event->ev_type, "SET_MOVE_TABLE") == 0)
			{
				int			tab_id = (int)strtol(event->ev_data1, NULL, 10);
				int			new_set_id = (int)strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1, "select %s.setMoveTable_int(%d, %d);",
								 rtcfg_namespace,
								 tab_id, new_set_id);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_MOVE_TABLE");
			}
			else if (strcmp(event->ev_type, "SET_MOVE_SEQUENCE") == 0)
			{
				int			seq_id = (int)strtol(event->ev_data1, NULL, 10);
				int			new_set_id = (int)strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1, "select %s.setMoveSequence_int(%d, %d);",
								 rtcfg_namespace,
								 seq_id, new_set_id);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SET_MOVE_SEQUENCE");
			}
			else if (strcmp(event->ev_type, "STORE_TRIGGER") == 0)
			{
				int			trig_tabid = (int)strtol(event->ev_data1, NULL, 10);
				char	   *trig_tgname = event->ev_data2;

				slon_appendquery(&query1,
								 "select %s.storeTrigger_int(%d, '%q'); ",
								 rtcfg_namespace,
								 trig_tabid, trig_tgname);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- STORE_TRIGGER");
			}
			else if (strcmp(event->ev_type, "DROP_TRIGGER") == 0)
			{
				int			trig_tabid = (int)strtol(event->ev_data1, NULL, 10);
				char	   *trig_tgname = event->ev_data2;

				slon_appendquery(&query1,
								 "select %s.dropTrigger_int(%d, '%q'); ",
								 rtcfg_namespace,
								 trig_tabid, trig_tgname);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- DROP_TRIGGER");
			}
			else if (strcmp(event->ev_type, "ACCEPT_SET") == 0)
			{
				int 		set_id, old_origin, 
						new_origin, event_no;
				PGresult	*res;

				slon_log(SLON_DEBUG2, "start processing ACCEPT_SET\n");
				set_id = (int) strtol(event->ev_data1, NULL, 10);
				slon_log(SLON_DEBUG2, "ACCEPT: set=%d\n", set_id);
				old_origin = (int) strtol(event->ev_data2, NULL, 10);
				slon_log(SLON_DEBUG2, "ACCEPT: old origin=%d\n", old_origin);
				new_origin = (int) strtol(event->ev_data3, NULL, 10);
				slon_log(SLON_DEBUG2, "ACCEPT: new origin=%d\n", new_origin);
				event_no = event->ev_seqno;
				slon_log(SLON_DEBUG2, "ACCEPT: move set seq=%d\n", event_no);

				slon_log(SLON_DEBUG2, "got parms ACCEPT_SET\n");
				
			    /* If we're a remote node, and haven't yet
			     * received the MOVE/FAILOVER_SET event from the
			     * old origin, then we'll need to sleep a
			     * bit...  This avoids a race condition
			     * where new SYNCs take place on the new
			     * origin, and are ignored on some
			     * subscribers (and their children)
			     * because the MOVE_SET wasn't yet
			     * received and processed  */

			    if ((rtcfg_nodeid != old_origin) && (rtcfg_nodeid != new_origin)) {
				    slon_log(SLON_DEBUG2, "ACCEPT_SET - node not origin\n");
				    slon_mkquery(&query2, 
						 "select 1 from %s.sl_event "
						 "where "
						 "     (ev_origin = %d and "
						 "      ev_type = 'MOVE_SET' and "
						 "      ev_data1 = '%d' and "
						 "      ev_data2 = '%d' and "
						 "      ev_data3 = '%d') "
						 "or "
						 "     (ev_origin = %d and "
						 "      ev_type = 'FAILOVER_SET' and "
						 "      ev_data1 = '%d' and "
						 "      ev_data2 = '%d' and "
						 "      ev_data3 = '%d'); ",
						 
						 rtcfg_namespace, 
						 old_origin, set_id, old_origin, new_origin,
						 old_origin, old_origin, new_origin, set_id);

				    res = PQexec(local_dbconn, dstring_data(&query2));
					while (PQntuples(res) == 0)
					{
						slon_log(SLON_DEBUG2, "ACCEPT_SET - MOVE_SET or FAILOVER_SET not received yet - sleep\n");
						if (sched_msleep(node, 10000) != SCHED_STATUS_OK)
							slon_abort();
						PQclear(res);
						res = PQexec(local_dbconn, dstring_data(&query2));
					}
					PQclear(res);
					slon_log(SLON_DEBUG2, "ACCEPT_SET - MOVE_SET or FAILOVER_SET exists - done\n");

					slon_appendquery(&query1,
									 "notify \"_%s_Restart\"; ",
									 rtcfg_cluster_name);
					query_append_event(&query1, event);
					slon_appendquery(&query1, "commit transaction;");
					query_execute(node, local_dbconn, &query1);
					slon_abort();

					need_reloadListen = true;
			    } else {
				    slon_log(SLON_DEBUG2, "ACCEPT_SET - on origin node...\n");
			    }

			}
			else if (strcmp(event->ev_type, "MOVE_SET") == 0)
			{
				int			set_id = (int)strtol(event->ev_data1, NULL, 10);
				int			old_origin = (int)strtol(event->ev_data2, NULL, 10);
				int			new_origin = (int)strtol(event->ev_data3, NULL, 10);
				int			sub_provider = -1;
				PGresult   *res;

				/*
				 * Move set is a little more tricky. The stored proc does a
				 * lot of rearranging in the provider chain. To catch up with
				 * that, we need to execute it now and select the resulting
				 * provider for us.
				 */

				slon_appendquery(&query1,
						 "select %s.moveSet_int(%d, %d, %d); ",
						 rtcfg_namespace,
						 set_id, old_origin, new_origin);
				if (query_execute(node, local_dbconn, &query1) < 0)
					slon_abort();

				slon_mkquery(&query1,
							 "select sub_provider from %s.sl_subscribe "
							 "	where sub_receiver = %d",
							 rtcfg_namespace, rtcfg_nodeid);
				res = PQexec(local_dbconn, dstring_data(&query1));
				if (PQresultStatus(res) != PGRES_TUPLES_OK)
				{
					slon_log(SLON_FATAL, "remoteWorkerThread_%d: \"%s\" %s",
							 node->no_id, dstring_data(&query1),
							 PQresultErrorMessage(res));
					PQclear(res);
					slon_abort();
				}
				if (PQntuples(res) == 1)
				{
					sub_provider =
						(int)strtol(PQgetvalue(res, 0, 0), NULL, 10);
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
				int			failed_node = (int)strtol(event->ev_data1, NULL, 10);
				int			backup_node = (int)strtol(event->ev_data2, NULL, 10);
				int			set_id = (int)strtol(event->ev_data3, NULL, 10);

				rtcfg_storeSet(set_id, backup_node, NULL);

				slon_appendquery(&query1,
								 "select %s.failoverSet_int(%d, %d, %d); ",
								 rtcfg_namespace,
								 failed_node, backup_node, set_id);

				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- FAILOVER_SET");
				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "SUBSCRIBE_SET") == 0)
			{
				int			sub_set = (int)strtol(event->ev_data1, NULL, 10);
				int			sub_provider = (int)strtol(event->ev_data2, NULL, 10);
				int			sub_receiver = (int)strtol(event->ev_data3, NULL, 10);
				char	   *sub_forward = event->ev_data4;

				if (sub_receiver == rtcfg_nodeid)
					rtcfg_storeSubscribe(sub_set, sub_provider, sub_forward);

				slon_appendquery(&query1,
						 "select %s.subscribeSet_int(%d, %d, %d, '%q'); ",
						 rtcfg_namespace,
						 sub_set, sub_provider, sub_receiver, sub_forward);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- SUBSCRIBE_SET");
				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "ENABLE_SUBSCRIPTION") == 0)
			{
				int			sub_set = (int)strtol(event->ev_data1, NULL, 10);
				int			sub_provider = (int)strtol(event->ev_data2, NULL, 10);
				int			sub_receiver = (int)strtol(event->ev_data3, NULL, 10);
				char	   *sub_forward = event->ev_data4;

				/*
				 * Do the actual enabling of the set only if we are the
				 * receiver.
				 */
				if (sub_receiver == rtcfg_nodeid &&
					event->ev_origin == node->no_id)
				{
					int			sched_rc;
					int			sleeptime = 15;

					slon_mkquery(&query2, "rollback transaction");
					check_config = true;

					while (true)
					{
						/*
						 * If we received this event from another node than
						 * the data provider, wait until the data provider has
						 * synced up far enough.
						 */
						if (event->event_provider != sub_provider)
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
								sched_rc = sched_msleep(node, 5000);
								if (sched_rc != SCHED_STATUS_OK)
								{
									event_ok = false;
									break;
								}
								continue;
							}
						}

						/*
						 * Execute the config changes so far, but don't commit
						 * the transaction yet. We have to copy the data now
						 * ...
						 */
						if (query_execute(node, local_dbconn, &query1) < 0)
							slon_abort();

						/*
						 * If the copy succeeds, exit the loop and let the
						 * transaction commit.
						 */
						if (copy_set(node, local_conn, sub_set, event) == 0)
						{
							rtcfg_enableSubscription(sub_set, sub_provider, sub_forward);
							slon_mkquery(&query1,
								"select %s.enableSubscription(%d, %d, %d); ",
										 rtcfg_namespace,
										 sub_set, sub_provider, sub_receiver);
							sched_rc = SCHED_STATUS_OK;
							break;
						}

						/*
						 * Data copy for new enabled set has failed. Rollback
						 * the transaction, sleep and try again.
						 */
						slon_log(SLON_WARN, "remoteWorkerThread_%d: "
								 "data copy for set %d failed - "
								 "sleep %d seconds\n",
								 node->no_id, sub_set, sleeptime);
						if (query_execute(node, local_dbconn, &query2) < 0)
							slon_abort();
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
				/* Note: No need to do anything based
				   on archive_dir here; copy_set does
				   that nicely already.  */
				need_reloadListen = true;
			}
			else if (strcmp(event->ev_type, "UNSUBSCRIBE_SET") == 0)
			{
				int			sub_set = (int)strtol(event->ev_data1, NULL, 10);
				int			sub_receiver = (int)strtol(event->ev_data2, NULL, 10);

				/*
				 * All the real work is done when the config utility calls
				 * unsubscribeSet() itself. Just propagate the event here.
				 */
				slon_appendquery(&query1,
								 "select %s.unsubscribeSet_int(%d, %d); ",
								 rtcfg_namespace,
								 sub_set, sub_receiver);

				need_reloadListen = true;
				if (archive_dir) {
					rc = open_log_archive(rtcfg_nodeid, seqbuf);
					rc = generate_archive_header(rtcfg_nodeid, seqbuf);
					slon_mkquery(&query1, 
						     "delete from %s.sl_setsync_offline "
						     "  where ssy_setid= %d;",
						     rtcfg_namespace, sub_set);
					rc = submit_query_to_archive(&query1);
					rc = close_log_archive();
				}
			}
			else if (strcmp(event->ev_type, "DDL_SCRIPT") == 0)
			{
				int			ddl_setid = (int)strtol(event->ev_data1, NULL, 10);
				char	   *ddl_script = event->ev_data2;
				int			ddl_only_on_node = (int)strtol(event->ev_data3, NULL, 10);
				
				
				slon_appendquery(&query1,
						 "select %s.ddlScript_int(%d, '%q', %d); ",
						 rtcfg_namespace,
						 ddl_setid, ddl_script, ddl_only_on_node);
				
				/* DDL_SCRIPT needs to be turned into a log shipping script */
				if (archive_dir) {
				    if ((ddl_only_on_node < 1) || (ddl_only_on_node == rtcfg_nodeid)) {
					
					rc = open_log_archive(node->no_id, seqbuf);
					if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Could not open DDL archive file %s - %s",
					       node->no_id, archive_tmp, strerror(errno));
					    slon_abort();
					}
					generate_archive_header(node->no_id, seqbuf);
					if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Could not generate DDL archive header %s - %s",
						     node->no_id, archive_tmp, strerror(errno));
					    slon_abort();
					}
					rc = logarchive_tracking(rtcfg_namespace, ddl_setid, seqbuf, seqbuf, event->ev_timestamp_c);
					if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Could not generate DDL archive tracker %s - %s",
						     node->no_id, archive_tmp, strerror(errno));
					    slon_abort();
					}
					rc = submit_string_to_archive(ddl_script);
					if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Could not submit DDL Script %s - %s",
						     node->no_id, archive_tmp, strerror(errno));
					    slon_abort();
					}
					
					rc = close_log_archive();
					if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Could not close DDL Script %s - %s",
						     node->no_id, archive_tmp, strerror(errno));
					    slon_abort();
					}
				    }
				}
			}
			else if (strcmp(event->ev_type, "RESET_CONFIG") == 0)
			{
				int			reset_config_setid = (int)strtol(event->ev_data1, NULL, 10);
				int			reset_configonly_on_node = (int)strtol(event->ev_data2, NULL, 10);

				slon_appendquery(&query1,
								 "select %s.updateReloid(%d, '%q', %d); ",
								 rtcfg_namespace,
							   reset_config_setid, reset_configonly_on_node);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- RESET_CONFIG");
			}
			else
			{
				printf("TODO: ********** remoteWorkerThread: node %d - EVENT %d," INT64_FORMAT " %s - unknown event type\n",
					   node->no_id, event->ev_origin, event->ev_seqno, event->ev_type);
				if (archive_dir)
					write_void_log (rtcfg_nodeid, seqbuf, "-- UNHANDLED EVENT!!!");
			}

			/*
			 * All simple configuration events fall through here. Commit the
			 * transaction.
			 */
			if (event_ok)
			{
				query_append_event(&query1, event);
				slon_appendquery(&query1, "commit transaction;");
			}
			else
			{
				slon_mkquery(&query1, "rollback transaction;");
			}
			if (query_execute(node, local_dbconn, &query1) < 0)
				slon_abort();

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
	free(wd->tab_fqname);
	free(wd->tab_forward);
#ifdef SLON_MEMDEBUG
	local_conn = NULL;
	memset(wd, 66, sizeof(WorkerGroupData));
#endif
	free(wd);

	slon_log(SLON_DEBUG1,
			 "remoteWorkerThread_%d: thread done\n",
			 node->no_id);
	pthread_exit(NULL);
}


static void
adjust_provider_info(SlonNode * node, WorkerGroupData * wd, int cleanup)
{
	ProviderInfo *provider;
	ProviderInfo *provnext;
	ProviderSet *pset;
	SlonNode   *rtcfg_node;
	SlonSet    *rtcfg_set;
	int			i;

	slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: "
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
					dstring_init(&(provider->helper_qualification));
					provider->helper_status = SLON_WG_IDLE;
					if (pthread_create(&(provider->helper_thread), NULL,
									   sync_helper, (void *)provider) != 0)
					{
						slon_log(SLON_FATAL, "remoteWorkerThread_%d: ",
								 "pthread_create() - %s\n",
								 node->no_id, strerror(errno));
						slon_abort();
					}
					slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
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

				slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: "
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

			slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
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
				slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
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
			dstring_free(&(provider->helper_qualification));
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
				slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
						 "disconnecting from data provider %d\n",
						 node->no_id, provider->no_id);
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
			}
			if (provider->pa_conninfo != NULL)
				free(provider->pa_conninfo);
			if ((rtcfg_node != NULL) || (rtcfg_node->pa_conninfo == NULL))
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


/*
 * ---------- remoteWorker_event
 *
 * Used by the remoteListeThread to forward events selected from the event
 * provider database to the remote nodes worker thread. ----------
 */
void
remoteWorker_event(int event_provider,
				   int ev_origin, int64 ev_seqno,
				   char *ev_timestamp,
				   char *ev_minxid, char *ev_maxxid, char *ev_xip,
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
	int			len_minxid;
	int			len_maxxid;
	int			len_xip;
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
		slon_log(SLON_DEBUG2,
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
		+ (len_timestamp = strlen(ev_timestamp) + 1)
		+ (len_minxid = strlen(ev_minxid) + 1)
		+ (len_maxxid = strlen(ev_maxxid) + 1)
		+ (len_xip = strlen(ev_xip) + 1)
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
		slon_abort();
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
	msg->ev_minxid_c = cp;
	strcpy(cp, ev_minxid);
	cp += len_minxid;
	msg->ev_maxxid_c = cp;
	strcpy(cp, ev_maxxid);
	cp += len_maxxid;
	msg->ev_xip = cp;
	strcpy(cp, ev_xip);
	cp += len_xip;
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


/*
 * ---------- remoteWorker_wakeup
 *
 * Send a special WAKEUP message to a worker, causing it to recheck the runmode
 * and the runtime configuration. ----------
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
		slon_log(SLON_DEBUG2,
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


/*
 * ---------- remoteWorker_confirm
 *
 * Add a confirm message to the remote worker message queue ----------
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
				}
				pthread_mutex_unlock(&(node->message_lock));
				return;
			}
		}
	}

	/*
	 * No message found. Create a new one and add it to the queue.
	 */
	msg = (SlonWorkMsg_confirm *)
		malloc(sizeof(SlonWorkMsg_confirm));
	msg->msg_type = WMSG_CONFIRM;

	msg->con_origin = con_origin;
	msg->con_received = con_received;
	msg->con_seqno = con_seqno;
	strcpy(msg->con_timestamp_c, con_timestamp_c);

	DLLIST_ADD_TAIL(node->message_head, node->message_tail,
					(SlonWorkMsg *) msg);

	/*
	 * Send a condition signal to the worker thread in case it is waiting for
	 * new messages.
	 */
	pthread_cond_signal(&(node->message_cond));
	pthread_mutex_unlock(&(node->message_lock));
}


/*
 * ---------- query_execute
 *
 * Execute a query string that does not return a result set. ----------
 */
static int
query_execute(SlonNode * node, PGconn *dbconn, SlonDString * dsp)

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


/*
 * ---------- query_append_event
 *
 * Add queries to a dstring that notify for Event and Confirm and that insert a
 * duplicate of an event record as well as the confirmation for it.
 * ----------
 */
static void
query_append_event(SlonDString * dsp, SlonWorkMsg_event * event)
{
	char		seqbuf [64];

	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);

	slon_appendquery(dsp,
					 "notify \"_%s_Event\"; "
					 "notify \"_%s_Confirm\"; "
					 "insert into %s.sl_event "
					 "    (ev_origin, ev_seqno, ev_timestamp, "
					 "     ev_minxid, ev_maxxid, ev_xip, ev_type ",
					 rtcfg_cluster_name, rtcfg_cluster_name,
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
					 "    ) values ('%d', '%s', '%s', '%s', '%s', '%q', '%s'",
					 event->ev_origin, seqbuf, event->ev_timestamp_c,
					 event->ev_minxid_c, event->ev_maxxid_c, event->ev_xip,
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


/*
 * ---------- store_confirm_forward
 *
 * Call the forwardConfirm() stored procedure. ----------
 */
static void
store_confirm_forward(SlonNode * node, SlonConn * conn,
					  SlonWorkMsg_confirm * confirm)
{
	SlonDString query;
	PGresult   *res;
	char		seqbuf [64];
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

	slon_mkquery(&query,
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


/*
 * ---------- get_last_forwarded_confirm
 *
 * Look what confirmed event seqno we forwarded last for a given origin+receiver
 * pair. ----------
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


static int
copy_set(SlonNode * node, SlonConn * local_conn, int set_id,
		 SlonWorkMsg_event * event)
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
	SlonDString indexregenquery;
	int			ntuples1;
	int			ntuples2;
	int			ntuples3;
	int			tupno1;
	int			tupno2;
	PGresult   *res1;
	PGresult   *res2;
	PGresult   *res3;
	PGresult   *res4;
	int        nodeon73;
	int			rc;
	int			set_origin = 0;
	SlonNode   *sub_node;
	int			sub_provider = 0;
	char	   *ssy_seqno = NULL;
	char	   *ssy_minxid = NULL;
	char	   *ssy_maxxid = NULL;
	char	   *ssy_xip = NULL;
	SlonDString ssy_action_list;
	char		seqbuf [64];
	
#ifdef HAVE_PQPUTCOPYDATA
	char	   *copydata = NULL;
	
#else
	char		copybuf[8192];
	int			copydone;
#endif
	struct timeval tv_start;
	struct timeval tv_start2;
	struct timeval tv_now;
	
	slon_log(SLON_DEBUG1, "copy_set %d\n", set_id);
	gettimeofday(&tv_start, NULL);
	
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
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: node %d "
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
	slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
		 "connected to provider DB\n",
		 node->no_id);
	
	pro_dbconn = pro_conn->dbconn;
	loc_dbconn = local_conn->dbconn;
	dstring_init(&query1);
	dstring_init(&query2);
	dstring_init(&query3);
	dstring_init(&indexregenquery);
	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);
	

	/* Log Shipping Support begins... */
	/*  - Open the log, put the header in
	    Isn't it convenient that seqbuf was just populated???  :-)
	*/
	if (archive_dir) {
		rc = open_log_archive(rtcfg_nodeid, seqbuf);
		if (rc < 0) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Could not open COPY SET archive file %s - %s",
				 node->no_id, archive_tmp, strerror(errno));
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		rc = generate_archive_header(rtcfg_nodeid, seqbuf);
		if (rc < 0) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Could not generate COPY SET archive header %s - %s",
				 node->no_id, archive_tmp, strerror(errno));
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}
	/*
	 * Register this connection in sl_nodelock
	 */
	slon_mkquery(&query1,
		     "select %s.registerNodeConnection(%d); ",
		     rtcfg_namespace, rtcfg_nodeid);
	if (query_execute(node, pro_dbconn, &query1) < 0)
	{
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&indexregenquery);
		terminate_log_archive();
		return -1;
	}

	/*
	 * Begin a serialized transaction and check if our xmin in the snapshot is
	 * > than ev_maxxid. This ensures that all transactions that have been in
	 * progress when the subscription got enabled (which is after the triggers
	 * on the tables have been defined), have finished. Otherwise a long
	 * running open transaction would not have the trigger definitions yet,
	 * and an insert would not get logged. But if it still runs when we start
	 * to copy the set, then we don't see the row either and it would get
	 * lost.
	 */
	if (sub_provider == set_origin)
	{
		slon_mkquery(&query1,
			     "start transaction; "
			     "set transaction isolation level serializable; "
			     "select %s.getMinXid() <= '%s'::%s.xxid; ",
			     rtcfg_namespace, event->ev_maxxid_c, rtcfg_namespace);
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		if (*(PQgetvalue(res1, 0, 0)) == 't')
		{
			slon_log(SLON_WARN, "remoteWorkerThread_%d: "
				 "transactions earlier than XID %s are still in progress\n",
				 node->no_id, event->ev_maxxid_c);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		PQclear(res1);
	}
	else
	{
		slon_mkquery(&query1,
			     "start transaction; "
			     "set transaction isolation level serializable; ");
		if (query_execute(node, pro_dbconn, &query1) < 0)
		{
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}

	/* check tables/sequences in set to make sure they are there
	 * and in good order.  Don't copy any data yet; we want to
	 * just do a first pass that finds "bozo errors" */

        /* Check tables and sequences in set to make sure they are all
	 * appropriately configured... */

	/*
	 * Select the list of all tables the provider currently has in the set.
	 */
	slon_mkquery(&query1,
		     "select T.tab_id, "
		     "    %s.slon_quote_input('\"' || PGN.nspname || '\".\"' || "
		     "    PGC.relname || '\"') as tab_fqname, "
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
		dstring_free(&indexregenquery);
		terminate_log_archive();
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
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "prepare to copy table %s\n",
			 node->no_id, tab_fqname);

		/*
		 * Find out if the table we're copying has the special slony serial
		 * number key on the provider DB
		 */
		slon_mkquery(&query1,
			     "select %s.tableHasSerialKey('%q');",
			     rtcfg_namespace, tab_fqname);
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		rc = *PQgetvalue(res2, 0, 0) == 't';
		PQclear(res2);

		if (rc)
		{
			/*
			 * It has, check if the table has this on the local DB too.
			 */
			slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
				 "table %s will require Slony-I serial key\n",
				 node->no_id, tab_fqname);
			res2 = PQexec(loc_dbconn, dstring_data(&query1));
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
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
			rc = *PQgetvalue(res2, 0, 0) == 't';
			PQclear(res2);

			if (!rc)
			{
				slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
					 "table %s Slony-I serial key to be added local\n",
					 node->no_id, tab_fqname);
			}
		}
		else
		{
			slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
				 "table %s does not require Slony-I serial key\n",
				 node->no_id, tab_fqname);
			slon_mkquery(&query3, "select * from %s limit 0;",
				     tab_fqname);
			res2 = PQexec(loc_dbconn, dstring_data(&query3));
			if (PQresultStatus(res2) != PGRES_TUPLES_OK) {
				slon_log (SLON_ERROR, "remoteWorkerThread_%d: Could not find table %s "
					  "on subscriber\n", node->no_id, tab_fqname);
				PQclear(res2);
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query3);
				terminate_log_archive();
				return -1;
			}
		}
	}
	PQclear(res1);

	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
		 "all tables for set %d found on subscriber\n",
		 node->no_id, set_id);
	/*
	 * Add in the sequences contained in the set
	 */
	slon_mkquery(&query1,
		     "select SQ.seq_id, "
		     "    %s.slon_quote_input('\"' || PGN.nspname || '\".\"' || "
		     "    PGC.relname || '\"') as tab_fqname, "
		     "		SQ.seq_comment "
		     "	from %s.sl_sequence SQ, "
		     "		\"pg_catalog\".pg_class PGC, "
		     "		\"pg_catalog\".pg_namespace PGN "
		     "	where SQ.seq_set = %d "
		     "		and PGC.oid = SQ.seq_reloid "
		     "		and PGN.oid = PGC.relnamespace; ",
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
		dstring_free(&indexregenquery);
		terminate_log_archive();
		return -1;
	}
	ntuples1 = PQntuples(res1);
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		char	   *seq_id = PQgetvalue(res1, tupno1, 0);
		char	   *seq_fqname = PQgetvalue(res1, tupno1, 1);
		char	   *seq_comment = PQgetvalue(res1, tupno1, 2);

		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "copy sequence %s\n",
			 node->no_id, seq_fqname);

		slon_mkquery(&query1,
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}
	PQclear(res1);


	/*
	 * Select the list of all tables the provider currently has in the set.
	 */
	slon_mkquery(&query1,
		     "select T.tab_id, "
		     "    %s.slon_quote_input('\"' || PGN.nspname || '\".\"' || "
		     "    PGC.relname || '\"') as tab_fqname, "
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
		dstring_free(&indexregenquery);
		terminate_log_archive();
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
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "copy table %s\n",
			 node->no_id, tab_fqname);

		/*
		 * Find out if the table we're copying has the special slony serial
		 * number key on the provider DB
		 */
		slon_mkquery(&query1,
			     "select %s.tableHasSerialKey('%q');",
			     rtcfg_namespace, tab_fqname);
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		rc = *PQgetvalue(res2, 0, 0) == 't';
		PQclear(res2);

		if (rc)
		{
			/*
			 * It has, check if the table has this on the local DB too.
			 */
			slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
				 "table %s requires Slony-I serial key\n",
				 node->no_id, tab_fqname);
			res2 = PQexec(loc_dbconn, dstring_data(&query1));
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
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
			rc = *PQgetvalue(res2, 0, 0) == 't';
			PQclear(res2);

			if (!rc)
			{
				/*
				 * Nope, so we gotta add the key here.
				 */
				slon_mkquery(&query1,
					     "select %s.tableAddKey('%q'); "
					     "select %s.determineAttkindSerial('%q'); ",
					     rtcfg_namespace, tab_fqname,
					     rtcfg_namespace, tab_fqname);
				if (query_execute(node, loc_dbconn, &query1) < 0)
				{
					PQclear(res1);
					slon_disconnectdb(pro_conn);
					dstring_free(&query1);
					dstring_free(&query2);
					dstring_free(&query3);
					dstring_free(&indexregenquery);
					terminate_log_archive();
					return -1;
				}
				slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
					 "table %s Slony-I serial key added local\n",
					 node->no_id, tab_fqname);
			}
			else
			{
				slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
					 "local table %s already has Slony-I serial key\n",
					 node->no_id, tab_fqname);
			}
		}
		else
		{
			slon_log(SLON_DEBUG3, "remoteWorkerThread_%d: "
				 "table %s does not require Slony-I serial key\n",
				 node->no_id, tab_fqname);
		}


		/*
		 * Call the setAddTable_int() stored procedure. Up to now, while we
		 * have not been subscribed to the set, this should have been
		 * suppressed.
		 */
		slon_mkquery(&query1,
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}

		/*
		 * Copy the content of sl_trigger for this table
		 */
		slon_mkquery(&query1,
			     "select trig_tgname from %s.sl_trigger "
			     "where trig_tabid = %d; ",
			     rtcfg_namespace, tab_id);
		res2 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res2) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s\n",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res2));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		ntuples2 = PQntuples(res2);
		for (tupno2 = 0; tupno2 < ntuples2; tupno2++)
		{
			slon_mkquery(&query1,
				     "select %s.storeTrigger(%d, '%q'); ",
				     rtcfg_namespace, tab_id, PQgetvalue(res2, tupno2, 0));
			if (query_execute(node, loc_dbconn, &query1) < 0)
			{
				PQclear(res2);
				PQclear(res1);
				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
		}
		PQclear(res2);


		/*
		 * Begin a COPY from stdin for the table on the local DB
		 */
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "Begin COPY of table %s\n",
			 node->no_id, tab_fqname);

		slon_mkquery(&query2, "select %s.copyFields(%d);",
			     rtcfg_namespace, tab_id);

		res3 = PQexec(pro_dbconn, dstring_data(&query2));

		if (PQresultStatus(res3) != PGRES_TUPLES_OK) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" - %s\n",
				 node->no_id, dstring_data(&query2),
				 PQresultErrorMessage(res3));
			PQclear(res3);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}

		slon_mkquery(&query2, "select %s.pre74();",
			     rtcfg_namespace);
		res4 = PQexec(loc_dbconn, dstring_data(&query2));
		
		if (PQresultStatus(res4) != PGRES_TUPLES_OK) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s\n",
				 node->no_id, dstring_data(&query2),
				 PQresultErrorMessage(res4));
			PQclear(res4);
			PQclear(res3);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}

		/* Are we running on < PG 7.4???  result =  */
		nodeon73 = atoi(PQgetvalue(res4, 0, 0));

		slon_mkquery(&query1,
			     "select %s.prepareTableForCopy(%d); "
			     "copy %s %s from stdin; ",
			     rtcfg_namespace,
			     tab_id, tab_fqname,
			     nodeon73 ? "" : PQgetvalue(res3, 0, 0)
			);
		res2 = PQexec(loc_dbconn, dstring_data(&query1));
		if (PQresultStatus(res2) != PGRES_COPY_IN)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s %s\n",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res2),
				 PQerrorMessage(loc_dbconn));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		if (archive_dir) {
			slon_mkquery(&query1,
				     "delete from %s;copy %s %s from stdin;", tab_fqname, tab_fqname,
				     nodeon73 ? "" : PQgetvalue(res3, 0, 0));
			rc = submit_query_to_archive(&query1);
			if (rc < 0) {
				slon_log(SLON_ERROR, "remoteWorkerThread_d: "
					 "Could not generate copy_set request for %s - %s",
					 node->no_id, tab_fqname, strerror(errno));

				slon_disconnectdb(pro_conn);
				dstring_free(&query1);
				dstring_free(&query2);
				dstring_free(&query3);
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
		}
		/*
		 * Begin a COPY to stdout for the table on the provider DB
		 */
		slon_mkquery(&query1,
			     "copy %s %s to stdout; ", tab_fqname, PQgetvalue(res3, 0, 0));
		PQclear(res3);
		res3 = PQexec(pro_dbconn, dstring_data(&query1));
		if (PQresultStatus(res3) != PGRES_COPY_OUT)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s %s\n",
				 node->no_id, dstring_data(&query1),
				 PQresultErrorMessage(res2),
				 PQerrorMessage(pro_dbconn));
#ifdef HAVE_PQPUTCOPYDATA
			PQputCopyEnd(loc_dbconn, "Slony-I: copy set operation failed");
#else
			PQputline(loc_dbconn, "\\.\n");
			PQendcopy(loc_dbconn);
#endif
			PQclear(res3);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}

		/*
		 * Copy the data over
		 */
#ifdef HAVE_PQPUTCOPYDATA
		while ((rc = PQgetCopyData(pro_dbconn, &copydata, 0)) > 0)
		{
			int len = strlen(copydata);

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
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
			if (archive_dir) {
				rc = fwrite(copydata, 1, len, archive_fp);
				if (rc != len) {
					slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "PQputCopyData() - log shipping - %s",
						 node->no_id, strerror(errno));
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
					dstring_free(&indexregenquery);
					terminate_log_archive();
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}

		/*
		 * Check that the COPY to stdout on the provider node finished
		 * successful.
		 */
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		if (archive_dir) {
			rc = submit_string_to_archive("\\.");
		}
#else							/* ! HAVE_PQPUTCOPYDATA */
		copydone = false;
		while (!copydone)
		{
			rc = PQgetline(pro_dbconn, copybuf, sizeof(copybuf));

			if (copybuf[0] == '\\' &&
			    copybuf[1] == '.' &&
			    copybuf[2] == '\0')
			{
				copydone = true;
			}
			else
			{
				switch (rc)
				{
				case EOF:
					copydone = true;
					break;
				case 0:
					PQputline(loc_dbconn, copybuf);
					PQputline(loc_dbconn, "\n");
					if (archive_dir)
						submit_string_to_archive(copybuf);
					break;
				case 1:
					PQputline(loc_dbconn, copybuf);
					if (archive_dir)
						submit_raw_data_to_archive(copybuf);
					break;

				}
			}
		}
		PQputline(loc_dbconn, "\\.\n");
		if (archive_dir) {
			rc = submit_string_to_archive("\\.");
		}
		/*
		 * End the COPY to stdout on the provider
		 */
		if (PQendcopy(pro_dbconn) != 0)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "PGendcopy() on provider%s",
				 node->no_id, PQerrorMessage(pro_dbconn));
			PQclear(res3);
			PQendcopy(loc_dbconn);
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		PQclear(res3);

		/*
		 * Check that the COPY to stdout on the provider node finished
		 * successful.
		 */

		/*
		 * End the COPY from stdin on the local node with success
		 */
		if (PQendcopy(loc_dbconn) != 0)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "PGendcopy() on local DB%s",
				 node->no_id, PQerrorMessage(loc_dbconn));
			PQclear(res2);
			PQclear(res1);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
#endif   /* HAVE_PQPUTCOPYDATA */

		PQclear(res2);
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 INT64_FORMAT " bytes copied for table %s\n",
			 node->no_id, copysize, tab_fqname);

		/*
		 * Analyze the table to update statistics
		 */
		slon_mkquery(&query1, "select %s.finishTableAfterCopy(%d); "
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		if (archive_dir) {
			submit_query_to_archive(&query1);
		}

		gettimeofday(&tv_now, NULL);
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "%.3f seconds to copy table %s\n",
			 node->no_id,
			 TIMEVAL_DIFF(&tv_start2, &tv_now), tab_fqname);
	}
	PQclear(res1);

	gettimeofday(&tv_start2, NULL);

	/*
	 * Copy over the sequence last_value corresponding to the
	 * ENABLE_SUBSCRIPTION event.
	 */
	slon_mkquery(&query1,
		     "select SL.seql_seqid, SL.seql_last_value, "
		     "    %s.slon_quote_input('\"' || PGN.nspname || '\".\"' || "
		     "    PGC.relname || '\"') as tab_fqname "
		     "	from %s.sl_sequence SQ, %s.sl_seqlog SL, "
		     "		\"pg_catalog\".pg_class PGC, "
		     "		\"pg_catalog\".pg_namespace PGN "
		     "	where SQ.seq_set = %d "
		     "		and SL.seql_seqid = SQ.seq_id "
		     "		and SL.seql_ev_seqno = '%s' "
		     "		and PGC.oid = SQ.seq_reloid "
		     "		and PGN.oid = PGC.relnamespace; ",
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
		dstring_free(&indexregenquery);
		terminate_log_archive();
		return -1;
	}
	ntuples1 = PQntuples(res1);
	for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
	{
		char	   *seql_seqid = PQgetvalue(res1, tupno1, 0);
		char	   *seql_last_value = PQgetvalue(res1, tupno1, 1);
		char	   *seq_fqname = PQgetvalue(res1, tupno1, 2);

		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
			 "set last_value of sequence %s (%s) to %s\n",
			 node->no_id, seql_seqid, seq_fqname, seql_last_value);

		

		/*
		 * sequence with ID 0 is a nodes rowid ... only remember in seqlog.
		 */
		if (strtol(seql_seqid, NULL, 10) != 0)
		{
			slon_mkquery(&query1,
				     "select \"pg_catalog\".setval('%q', '%s'); ",
				     seq_fqname, seql_last_value);

			if (archive_dir) {
				submit_query_to_archive(&query1);
			}
		}
		else
			dstring_reset(&query1);
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}
	PQclear(res1);

	if (ntuples1 > 0)
	{
		gettimeofday(&tv_now, NULL);
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
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
		slon_mkquery(&query1,
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		if (PQgetisnull(res1, 0, 0))
		{
			/*
			 * No SYNC event found, so we initialize the setsync to the event
			 * point of the ENABLE_SUBSCRIPTION
			 */
			ssy_seqno = seqbuf;
			ssy_minxid = event->ev_minxid_c;
			ssy_maxxid = event->ev_maxxid_c;
			ssy_xip = event->ev_xip;

			slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "copy_set no previous SYNC found, use enable event.\n",
				 node->no_id);

			slon_mkquery(&query1,
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
			slon_mkquery(&query1,
				     "select ev_seqno, ev_minxid, ev_maxxid, ev_xip "
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
				dstring_free(&indexregenquery);
				terminate_log_archive();
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
				dstring_free(&indexregenquery);
				terminate_log_archive();
				return -1;
			}
			ssy_seqno = PQgetvalue(res1, 0, 0);
			ssy_minxid = PQgetvalue(res1, 0, 1);
			ssy_maxxid = PQgetvalue(res1, 0, 2);
			ssy_xip = PQgetvalue(res1, 0, 3);

			slon_mkquery(&query2,
				     "log_xid >= '%s' or (log_xid >= '%s'",
				     ssy_maxxid, ssy_minxid);
			if (strlen(ssy_xip) != 0)
				slon_appendquery(&query2, " and log_xid in (%s))", ssy_xip);
			else
				slon_appendquery(&query2, ")");

			slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "copy_set SYNC found, use event seqno %s.\n",
				 node->no_id, ssy_seqno);

			slon_mkquery(&query1,
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
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
		slon_mkquery(&query1,
			     "select ssy_seqno, ssy_minxid, ssy_maxxid, "
			     "    ssy_xip, ssy_action_list "
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
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
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
		dstring_init(&ssy_action_list);
		ssy_seqno = PQgetvalue(res1, 0, 0);
		ssy_minxid = PQgetvalue(res1, 0, 1);
		ssy_maxxid = PQgetvalue(res1, 0, 2);
		ssy_xip = PQgetvalue(res1, 0, 3);
		dstring_append(&ssy_action_list, PQgetvalue(res1, 0, 4));
		dstring_terminate(&ssy_action_list);
	}

	/*
	 * Create our own initial setsync entry
	 */
	slon_mkquery(&query1,
		     "insert into %s.sl_setsync "
		     "    (ssy_setid, ssy_origin, ssy_seqno, "
		     "     ssy_minxid, ssy_maxxid, ssy_xip, ssy_action_list) "
		     "    values ('%d', '%d', '%s', '%s', '%s', '%q', '%q'); ",
		     rtcfg_namespace,
		     set_id, node->no_id, ssy_seqno, ssy_minxid, ssy_maxxid, ssy_xip,
		     dstring_data(&ssy_action_list));
	PQclear(res1);
	dstring_free(&ssy_action_list);
	if (query_execute(node, loc_dbconn, &query1) < 0)
	{
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&indexregenquery);
		terminate_log_archive();
		return -1;
	}
	if (archive_dir) {
		slon_mkquery(&query1,
			     "insert into %s.sl_setsync_offline (ssy_setid, ssy_seqno) "
			     "values ('%d', '%d');",
			     rtcfg_namespace, set_id, ssy_seqno);
		rc = submit_query_to_archive(&query1);
		if (rc < 0) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 " could not insert to sl_setsync_offline",
				 node->no_id);
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}
	gettimeofday(&tv_now, NULL);
	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
		 "%.3f seconds to build initial setsync status\n",
		 node->no_id,
		 TIMEVAL_DIFF(&tv_start2, &tv_now));
		
	if (archive_dir) {
		rc = close_log_archive();
		if (rc < 0) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 " could not close archive log %s - %s",
				 node->no_id, archive_tmp, strerror(errno));
			slon_disconnectdb(pro_conn);
			dstring_free(&query1);
			dstring_free(&query2);
			dstring_free(&query3);
			dstring_free(&indexregenquery);
			terminate_log_archive();
			return -1;
		}
	}
		

	/*
	 * Roll back the transaction we used on the provider and close the
	 * database connection.
	 */
	slon_mkquery(&query1, "rollback transaction");
	if (query_execute(node, pro_dbconn, &query1) < 0)
	{
		slon_disconnectdb(pro_conn);
		dstring_free(&query1);
		dstring_free(&query2);
		dstring_free(&query3);
		dstring_free(&indexregenquery);
		terminate_log_archive();
		return -1;
	}
	slon_disconnectdb(pro_conn);
	dstring_free(&query1);
	dstring_free(&query2);
	dstring_free(&query3);
	dstring_free(&indexregenquery);

	slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
		 "disconnected from provider DB\n",
		 node->no_id);
		
	gettimeofday(&tv_now, NULL);
	slon_log(SLON_DEBUG1, "copy_set %d done in %.3f seconds\n", set_id,
		 TIMEVAL_DIFF(&tv_start, &tv_now));
	return 0;
}

static int
sync_event(SlonNode * node, SlonConn * local_conn,
		   WorkerGroupData * wd, SlonWorkMsg_event * event)
{
	ProviderInfo *provider;
	ProviderSet *pset;
	char		conn_symname[64];

	/* TODO: tab_forward array to know if we need to store the log */
	PGconn	   *local_dbconn = local_conn->dbconn;
	PGresult   *res1;
	int			num_sets = 0;
	int			num_providers_active = 0;
	int			num_errors;
	WorkerGroupLine *wgline;
	int			i;
	int rc;
	char		seqbuf [64];
	struct timeval tv_start;
	struct timeval tv_now;

	SlonDString new_qual;
	SlonDString query;
	SlonDString *provider_qual;

	gettimeofday(&tv_start, NULL);
	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: SYNC " INT64_FORMAT
			 " processing\n",
			 node->no_id, event->ev_seqno);

	sprintf(seqbuf, INT64_FORMAT, event->ev_seqno);
	dstring_init(&query);

	/*
	 * If this slon is running in log archiving mode, open a
	 * temporary file for it.
	 */
	if (archive_dir)
	{
		rc = open_log_archive(node->no_id, seqbuf);
		if (rc == -1) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot open archive file %s - %s\n",
				 node->no_id, archive_tmp, strerror(errno));
			dstring_free(&query);
			return 60;
		}
		rc = generate_archive_header(node->no_id, seqbuf);
		if (rc < 0) {
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				 "Cannot write to archive file %s - %s",
				 node->no_id, archive_tmp, strerror(errno));
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
				TERMINATE_QUERY_AND_ARCHIVE;
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
				TERMINATE_QUERY_AND_ARCHIVE;
				return provider->pa_connretry;
			}

			/*
			 * Listen on the special relation telling our node relationship
			 */
			slon_mkquery(&query,
						 "select %s.registerNodeConnection(%d); ",
						 rtcfg_namespace, rtcfg_nodeid);
			if (query_execute(node, provider->conn->dbconn, &query) < 0)
			{
				TERMINATE_QUERY_AND_ARCHIVE;
				slon_disconnectdb(provider->conn);
				provider->conn = NULL;
				return provider->pa_connretry;
			}
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
				TERMINATE_QUERY_AND_ARCHIVE;
				return 10;
			}
			if (prov_seqno < event->ev_seqno)
			{
				slon_log(SLON_DEBUG1, "remoteWorkerThread_%d: "
						 "data provider %d only confirmed up to "
						 "ev_seqno " INT64_FORMAT " for ev_origin %d\n",
						 node->no_id, provider->no_id,
						 prov_seqno, event->ev_origin);
				TERMINATE_QUERY_AND_ARCHIVE;
				return 10;
			}
			slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
					 "data provider %d confirmed up to "
					 "ev_seqno %s for ev_origin %d - OK\n",
					 node->no_id, provider->no_id,
					 seqbuf, event->ev_origin);
		}
	}

	dstring_init(&new_qual);

	if (strlen(event->ev_xip) != 0)
		slon_mkquery(&new_qual,
					 "(log_xid < '%s' and "
					 "%s.xxid_lt_snapshot(log_xid, '%s:%s:%q'))",
					 event->ev_maxxid_c,
					 rtcfg_namespace,
					 event->ev_minxid_c, event->ev_maxxid_c, event->ev_xip);
	else
		slon_mkquery(&new_qual,
					 "(log_xid < '%s')",
					 event->ev_maxxid_c);

	for (provider = wd->provider_head; provider; provider = provider->next)
	{
		int			ntuples1;
		int			tupno1;
		PGresult   *res2;
		int			ntuples2;
		int			tupno2;
		int			ntables_total = 0;
		int			added_or_to_provider = 0;

		provider_qual = &(provider->helper_qualification);
		dstring_reset(provider_qual);
		slon_mkquery(provider_qual,
					 "where log_origin = %d and ( ",
					 node->no_id);

		/*
		 * Select all sets we receive from this provider and which are not
		 * synced better than this SYNC already.
		 */
		slon_mkquery(&query,
					 "select SSY.ssy_setid, SSY.ssy_seqno, "
					 "    SSY.ssy_minxid, SSY.ssy_maxxid, SSY.ssy_xip, "
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

		res1 = PQexec(local_dbconn, dstring_data(&query));
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			dstring_free(&new_qual);
			TERMINATE_QUERY_AND_ARCHIVE;
			return 60;
		}

		/*
		 * For every set we receive from this provider
		 */
		ntuples1 = PQntuples(res1);
		if (ntuples1 == 0)
		{
			PQclear(res1);
			continue;
		}
		num_sets += ntuples1;

		for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
		{
			int			sub_set = strtol(PQgetvalue(res1, tupno1, 0), NULL, 10);
			char	   *ssy_minxid = PQgetvalue(res1, tupno1, 2);
			char	   *ssy_maxxid = PQgetvalue(res1, tupno1, 3);
			char	   *ssy_xip = PQgetvalue(res1, tupno1, 4);
			char	   *ssy_action_list = PQgetvalue(res1, tupno1, 5);

			/*
			 * Select the tables in that set ...
			 */
			slon_mkquery(&query,
				     "select T.tab_id, T.tab_set, "
				     "    %s.slon_quote_input('\"' || PGN.nspname || '\".\"' || "
				     "    PGC.relname || '\"') as tab_fqname "
				     "from %s.sl_table T, "
				     "    \"pg_catalog\".pg_class PGC, "
				     "    \"pg_catalog\".pg_namespace PGN "
				     "where T.tab_set = %d "
				     "    and PGC.oid = T.tab_reloid "
				     "    and PGC.relnamespace = PGN.oid; ",
				     rtcfg_namespace, 
				     rtcfg_namespace, 
				     sub_set);
			res2 = PQexec(local_dbconn, dstring_data(&query));
			if (PQresultStatus(res2) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
						 node->no_id, dstring_data(&query),
						 PQresultErrorMessage(res2));
				PQclear(res2);
				PQclear(res1);
				dstring_free(&new_qual);
				TERMINATE_QUERY_AND_ARCHIVE;
				return 60;
			}
			ntuples2 = PQntuples(res2);
			slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
					 "syncing set %d with %d table(s) from provider %d\n",
					 node->no_id, sub_set, ntuples2,
					 provider->no_id);

			if (ntuples2 == 0)
			{
				PQclear(res2);

				if (!added_or_to_provider)
				{
					slon_appendquery(provider_qual, " ( false ) ");
					added_or_to_provider = 1;
				}

				continue;
			}
			ntables_total += ntuples2;

			/*
			 * ... and build up a query qualification that is
			 *
			 * and ( (log_tableid in (<tables_in_set>) and
			 * (<snapshot_qual_of_new_sync>) and (<snapshot_qual_of_setsync>)
			 * ) OR ( <next_set_from_this_provider> ) )
			 *
			 * If we were using AND's there then no rows will ever end up being
			 * selected when you have multiple sets.
			 */

			if (added_or_to_provider)
			{
				slon_appendquery(provider_qual, "or (\n    log_tableid in (");
			}
			else
			{
				slon_appendquery(provider_qual, " (\n log_tableid in (");
				added_or_to_provider = 1;
			}

			/* the <tables_in_set> tab_id list */
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
					wd->tab_fqname = (char **)realloc(wd->tab_fqname,
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

				if (tupno2 > 0)
					dstring_addchar(provider_qual, ',');
				dstring_append(provider_qual, PQgetvalue(res2, tupno2, 0));
			}

			/* add the <snapshot_qual_of_new_sync> */
			slon_appendquery(provider_qual,
							 ")\n    and %s\n    and ",
							 dstring_data(&new_qual));

			/* add the <snapshot_qual_of_setsync> */
			if (strlen(ssy_xip) != 0)
				slon_appendquery(provider_qual,
								 "(log_xid >= '%s' and "
								 "%s.xxid_ge_snapshot(log_xid, '%s:%s:%q'))",
								 ssy_minxid,
								 rtcfg_namespace,
								 ssy_minxid, ssy_maxxid, ssy_xip);
			else
				slon_appendquery(provider_qual,
								 "(log_xid >= '%s')",
								 ssy_maxxid);
			if (strlen(ssy_action_list) != 0)
				slon_appendquery(provider_qual,
								 " and log_actionseq not in (%s)\n) ",
								 ssy_action_list);
			else
				slon_appendquery(provider_qual, "\n) ");

			PQclear(res2);

			/*
			 * Add a call to the setsync tracking function to
			 * the archive log. This function ensures that all
			 * archive log files are applied in the right order.
			 */
			if (archive_dir)
			{
				slon_log(SLON_DEBUG2, "writing archive log...\n");
				fflush(stderr);
				fflush(stdout);
				rc = logarchive_tracking(rtcfg_namespace, sub_set, 
							 PQgetvalue(res1, tupno1, 1), seqbuf, 
							 event->ev_timestamp_c);
				if (rc < 0) {
					slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						 "Cannot write to archive file %s - %s",
						 node->no_id, archive_tmp, strerror(errno));
					return 60;
				}
			}
		}
		PQclear(res1);

		/*
		 * We didn't add anything good in the provider clause. That shouldn't
		 * be!
		 */
		if (added_or_to_provider)
		{
			/* close out our OR block */
			slon_appendquery(provider_qual, ")");
		}
		else
		{
			slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: Didn't add or to provider\n", node->no_id);
		}
	}

	dstring_free(&new_qual);

	/*
	 * If we have found no sets needing sync at all, why bother the helpers?
	 */
	if (num_sets == 0)
	{
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "no sets need syncing for this event\n",
				 node->no_id);
		dstring_free(&query);
		if (archive_dir) {
		  rc = close_log_archive();
		  if (rc < 0) {
		    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
			     "Could not close out archive file %s - %s",
			     node->no_id, archive_tmp, strerror(errno));
		    return 60;
		  }
		}
		return 0;
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
						res1 = PQexec(local_dbconn, dstring_data(&(wgline->log)));
						if (PQresultStatus(res1) == PGRES_EMPTY_QUERY)
						{
							PQclear(res1);
							break;
						}
						if (PQresultStatus(res1) != PGRES_COMMAND_OK)
						{
							slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
									 "\"%s\" %s - qualification was: %s\n",
									 node->no_id, dstring_data(&(wgline->data)),
									 PQresultErrorMessage(res1),
									 dstring_data(&(wgline->provider->helper_qualification)));
							num_errors++;
						}
						PQclear(res1);
					}

					res1 = PQexec(local_dbconn, dstring_data(&(wgline->data)));
					if (PQresultStatus(res1) == PGRES_EMPTY_QUERY)
					{
						PQclear(res1);
						break;
					}
					if (PQresultStatus(res1) != PGRES_COMMAND_OK)
					{
						slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
								 "\"%s\" %s - qualification was: %s\n",
								 node->no_id, dstring_data(&(wgline->data)),
								 PQresultErrorMessage(res1),
								 dstring_data(&(wgline->provider->helper_qualification)));
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
								   "query was: %s - qualification was: %s\n",
									 node->no_id, PQcmdTuples(res1),
									 dstring_data(&(wgline->data)),
									 dstring_data(&(wgline->provider->helper_qualification)));
							num_errors++;
						}
						else
							slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: %s\n",
								 node->no_id, dstring_data(&(wgline->data)));
					}
#endif
					PQclear(res1);

					/*
					 * Add the user data modification part to
					 * the archive log.
					 */
					if (archive_dir) {
					  rc = submit_string_to_archive(dstring_data(&(wgline->data)));
					  /* rc = fprintf(archive_fp, "%s", dstring_data(&(wgline->data))); */
					  if (rc < 0) {
					    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
						     "Cannot write to archive file %s - %s",
						     node->no_id, archive_tmp, strerror(errno));
					    return 60;
					  }
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
			dstring_reset(&(wgline->data));
			dstring_reset(&(wgline->log));
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

	slon_log(SLON_DEBUG4, "remoteWorkerThread_%d: cleanup\n",
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
		TERMINATE_QUERY_AND_ARCHIVE;
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

		slon_mkquery(&query,
					 "select SL.seql_seqid, SL.seql_last_value "
					 "	from %s.sl_seqlog SL, "
					 "		%s.sl_sequence SQ "
					 "	where SQ.seq_id = SL.seql_seqid "
					 "		and SL.seql_origin = %d "
					 "		and SL.seql_ev_seqno = '%s' "
					 "		and SQ.seq_set in (",
					 rtcfg_namespace, rtcfg_namespace,
					 node->no_id, seqbuf);
		for (pset = provider->set_head; pset; pset = pset->next)
			slon_appendquery(&query, "%s%d",
							 (pset->prev == NULL) ? "" : ",",
							 pset->set_id);
		slon_appendquery(&query, "); ");

		res1 = PQexec(provider->conn->dbconn, dstring_data(&query));
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			TERMINATE_QUERY_AND_ARCHIVE;
			slon_disconnectdb(provider->conn);
			provider->conn = NULL;
			return 20;
		}
		ntuples1 = PQntuples(res1);
		for (tupno1 = 0; tupno1 < ntuples1; tupno1++)
		{
			char	   *seql_seqid = PQgetvalue(res1, tupno1, 0);
			char	   *seql_last_value = PQgetvalue(res1, tupno1, 1);

			slon_mkquery(&query,
						 "select %s.sequenceSetValue(%s,%d,'%s','%s'); ",
						 rtcfg_namespace,
						 seql_seqid, node->no_id, seqbuf, seql_last_value);
			if (query_execute(node, local_dbconn, &query) < 0)
			{
				PQclear(res1);
				TERMINATE_QUERY_AND_ARCHIVE;
				return 60;
			}

			/*
			 * Add the sequence number adjust call to the archive log.
			 */
			if (archive_dir)
			{
			  slon_mkquery(&query,
				       "select %s.sequenceSetValue_offline(%s,'%s');\n",
				       rtcfg_namespace,
				       seql_seqid, seql_last_value);
			  rc = submit_query_to_archive(&query);
			  if (rc < 0) {
			    slon_log(SLON_ERROR, "remoteWorkerThread_%d: "
				     "Cannot write to archive file %s - %s",
				     node->no_id, archive_tmp, strerror(errno));
			    return 60;
			  }
			}
		}
		PQclear(res1);
	}

	/*
	 * Light's are still green ... update the setsync status of all the sets
	 * we've just replicated ...
	 */
	slon_mkquery(&query,
				 "update %s.sl_setsync set "
			   "    ssy_seqno = '%s', ssy_minxid = '%s', ssy_maxxid = '%s', "
				 "    ssy_xip = '%q', ssy_action_list = '' "
				 "where ssy_setid in (",
				 rtcfg_namespace,
				 seqbuf, event->ev_minxid_c, event->ev_maxxid_c,
				 event->ev_xip);
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
		res1 = PQexec(local_dbconn, dstring_data(&query));
		if (PQresultStatus(res1) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
					 node->no_id, dstring_data(&query),
					 PQresultErrorMessage(res1));
			PQclear(res1);
			TERMINATE_QUERY_AND_ARCHIVE;
			slon_log(SLON_ERROR, "remoteWorkerThread_%d: SYNC aborted\n",
					 node->no_id);
			return 10;
		}
		PQclear(res1);
	}

	/*
	 * Get the nodes rowid sequence at that sync time just in case we are
	 * later on asked to restore the node after a failover.
	 */
	slon_mkquery(&query,
				 "select seql_last_value from %s.sl_seqlog "
				 "	where seql_seqid = 0 "
				 "	and seql_origin = %d "
				 "	and seql_ev_seqno = '%s'; ",
				 rtcfg_namespace, node->no_id,
				 seqbuf);
	res1 = PQexec(wd->provider_head->conn->dbconn, dstring_data(&query));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "remoteWorkerThread_%d: \"%s\" %s",
				 node->no_id, dstring_data(&query),
				 PQresultErrorMessage(res1));
		PQclear(res1);
		TERMINATE_QUERY_AND_ARCHIVE;
		return 60;
	}
	if (PQntuples(res1) > 0)
	{
		slon_mkquery(&query,
					 "insert into %s.sl_seqlog "
			   "	(seql_seqid, seql_origin, seql_ev_seqno, seql_last_value) "
					 "	values (0, %d, '%s', '%s'); ",
					 rtcfg_namespace, node->no_id,
					 seqbuf, PQgetvalue(res1, 0, 0));
		if (query_execute(node, local_dbconn, &query) < 0)
		{
			PQclear(res1);
			TERMINATE_QUERY_AND_ARCHIVE;
			return 60;
		}
		slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: "
				 "new sl_rowid_seq value: %s\n",
				 node->no_id, PQgetvalue(res1, 0, 0));
	}
	PQclear(res1);

	/*
	 * Add the final commit to the archive log, close it and rename
	 * the temporary file to the real log chunk filename.
	 */
	if (archive_dir)
	{
	  close_log_archive();
	}

	/*
	 * Good job!
	 */
	dstring_free(&query);
	gettimeofday(&tv_now, NULL);
	slon_log(SLON_DEBUG2, "remoteWorkerThread_%d: SYNC "
			 INT64_FORMAT " done in %.3f seconds\n",
			 node->no_id, event->ev_seqno,
			 TIMEVAL_DIFF(&tv_start, &tv_now));
	return 0;
}


static void *
sync_helper(void *cdata)
{
	ProviderInfo *provider = (ProviderInfo *) cdata;
	WorkerGroupData *wd = provider->wd;
	SlonNode   *node = wd->node;
	PGconn	   *dbconn;
	WorkerGroupLine *line = NULL;
	int			line_no;
	SlonDString query;
	PGresult   *res;
	int			ntuples;
	int			tupno;
	int			errors;
	WorkerGroupLine *data_line[SLON_DATA_FETCH_SIZE];
	int			alloc_lines = 0;
	struct timeval tv_start;
	struct timeval tv_now;
	int			first_fetch;

	dstring_init(&query);

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

		errors = 0;
		do
		{
			/*
			 * Start a transaction
			 */
			slon_mkquery(&query, "start transaction; "
						 "set enable_seqscan = off; "
						 "set enable_indexscan = on; ");
			if (query_execute(node, dbconn, &query) < 0)
			{
				errors++;
				break;
			}

			/*
			 * Open a cursor that reads the log data.
			 *
			 * TODO: need to change this into a conditional sl_log_n selection
			 * depending on the logstatus.
			 */
			slon_mkquery(&query,
						 "declare LOG cursor for select "
						 "    log_origin, log_xid, log_tableid, "
						 "    log_actionseq, log_cmdtype, log_cmddata "
						 "from %s.sl_log_1 %s order by log_actionseq; ",
						 rtcfg_namespace,
						 dstring_data(&(provider->helper_qualification)));

			gettimeofday(&tv_start, NULL);
			first_fetch = true;

			if (query_execute(node, dbconn, &query) < 0)
			{
				errors++;
				break;
			}

			/*
			 * Now fetch the log data and forward it via the line pool to the
			 * main worker who pushes it into the local database.
			 */
			alloc_lines = 0;
			while (errors == 0)
			{
				/*
				 * Allocate at least some lines - ideally the whole fetch
				 * size.
				 */
				while (alloc_lines == 0 && !errors)
				{
					slon_log(SLON_DEBUG4,
							 "remoteHelperThread_%d_%d: allocate lines\n",
							 node->no_id, provider->no_id);

					/*
					 * Wait until there are lines available in the pool.
					 */
					pthread_mutex_lock(&(wd->workdata_lock));
					while (wd->linepool_head == NULL &&
						   wd->workgroup_status == SLON_WG_BUSY)
					{
						pthread_cond_wait(&(wd->linepool_cond), &(wd->workdata_lock));
					}

					/*
					 * If any error occured somewhere in the group, the main
					 * worker will set the status to ABORT.
					 */
					if (wd->workgroup_status != SLON_WG_BUSY)
					{
						slon_log(SLON_DEBUG4,
							   "remoteHelperThread_%d_%d: abort operation\n",
								 node->no_id, provider->no_id);
						pthread_mutex_unlock(&(wd->workdata_lock));
						errors++;
						break;
					}

					/*
					 * So far so good. Fill our array of lines from the pool.
					 */
					while (alloc_lines < SLON_DATA_FETCH_SIZE &&
						   wd->linepool_head != NULL)
					{
						data_line[alloc_lines] = wd->linepool_head;
						DLLIST_REMOVE(wd->linepool_head, wd->linepool_tail,
									  data_line[alloc_lines]);
						alloc_lines++;
					}
					pthread_mutex_unlock(&(wd->workdata_lock));
				}

				if (errors)
					break;

				slon_log(SLON_DEBUG4,
						 "remoteHelperThread_%d_%d: have %d line buffers\n",
						 node->no_id, provider->no_id, alloc_lines);

				/*
				 * Now that we have allocated some buffer space, try to fetch
				 * that many rows from the cursor.
				 */
				slon_mkquery(&query, "fetch %d from LOG; ",
							 alloc_lines * SLON_COMMANDS_PER_LINE);
				res = PQexec(dbconn, dstring_data(&query));
				if (PQresultStatus(res) != PGRES_TUPLES_OK)
				{
					slon_log(SLON_ERROR, "remoteHelperThread_%d_%d: \"%s\" %s",
							 node->no_id, provider->no_id,
							 dstring_data(&query),
							 PQresultErrorMessage(res));
					PQclear(res);
					errors++;
					break;
				}
				if (first_fetch)
				{
					gettimeofday(&tv_now, NULL);
					slon_log(SLON_DEBUG2,
							 "remoteHelperThread_%d_%d: %.3f seconds delay for first row\n",
							 node->no_id, provider->no_id,
							 TIMEVAL_DIFF(&tv_start, &tv_now));

					first_fetch = false;
				}

				/*
				 * Fill the line buffers with queries from the retrieved log
				 * rows.
				 */
				line_no = 0;
				ntuples = PQntuples(res);
				slon_log(SLON_DEBUG3,
						 "remoteHelperThread_%d_%d: got %d log rows\n",
						 node->no_id, provider->no_id, ntuples);
				for (tupno = 0; tupno < ntuples; tupno++)
				{
					char	   *log_origin = PQgetvalue(res, tupno, 0);
					char	   *log_xid = PQgetvalue(res, tupno, 1);
					int			log_tableid = strtol(PQgetvalue(res, tupno, 2),
											  NULL, 10);
					char	   *log_actionseq = PQgetvalue(res, tupno, 3);
					char	   *log_cmdtype = PQgetvalue(res, tupno, 4);
					char	   *log_cmddata = PQgetvalue(res, tupno, 5);

					if (tupno % SLON_COMMANDS_PER_LINE == 0)
					{
						line = data_line[line_no++];
						line->code = SLON_WGLC_ACTION;
						line->provider = provider;
						dstring_reset(&(line->data));
						dstring_reset(&(line->log));
					}

					/*
					 * This can happen if the table belongs to a set that
					 * already has a better sync status than the event we're
					 * currently processing as a result from another SYNC
					 * occuring before we had started processing the copy_set.
					 */
					if (log_tableid >= wd->tab_fqname_size ||
						wd->tab_fqname[log_tableid] == NULL)
						continue;

					if (wd->tab_forward[log_tableid])
					{
						slon_appendquery(&(line->log),
								 "insert into %s.sl_log_1 "
								 "    (log_origin, log_xid, log_tableid, "
								 "     log_actionseq, log_cmdtype, "
								 "     log_cmddata) values "
								 "    ('%s', '%s', '%d', '%s', '%q', '%q');\n",
								 rtcfg_namespace,
								 log_origin, log_xid, log_tableid,
								 log_actionseq, log_cmdtype, log_cmddata);
					}
					switch (*log_cmdtype)
					{
						case 'I':
							slon_appendquery(&(line->data),
									 "insert into %s %s;\n",
									 wd->tab_fqname[log_tableid],
									 log_cmddata);
							break;

						case 'U':
							slon_appendquery(&(line->data),
									 "update only %s set %s;\n",
									 wd->tab_fqname[log_tableid],
									 log_cmddata);
							break;

						case 'D':
							slon_appendquery(&(line->data),
									 "delete from only %s where %s;\n",
									 wd->tab_fqname[log_tableid],
									 log_cmddata);
							break;
					}
				}
				PQclear(res);

				/*
				 * Now put all the line buffers back. Filled ones into the
				 * repldata, unused ones into the pool.
				 */
				pthread_mutex_lock(&(wd->workdata_lock));
				for (tupno = 0; tupno < alloc_lines; tupno++)
				{
					if (tupno < line_no)
						DLLIST_ADD_TAIL(wd->repldata_head, wd->repldata_tail,
										data_line[tupno]);
					else
						DLLIST_ADD_HEAD(wd->linepool_head, wd->linepool_tail,
										data_line[tupno]);
				}
				if (line_no > 0)
					pthread_cond_signal(&(wd->repldata_cond));
				if (line_no < alloc_lines)
					pthread_cond_broadcast(&(wd->linepool_cond));
				pthread_mutex_unlock(&(wd->workdata_lock));

				slon_log(SLON_DEBUG3,
					   "remoteHelperThread_%d_%d: %d log buffers delivered\n",
						 node->no_id, provider->no_id, line_no);

				if (line_no < alloc_lines)
				{
					slon_log(SLON_DEBUG4,
							 "remoteHelperThread_%d_%d: no more log rows\n",
							 node->no_id, provider->no_id);
					alloc_lines = 0;
					break;
				}
				alloc_lines = 0;
			}
		} while (0);

		/*
		 * if there are still line buffers allocated, give them back.
		 */
		if (alloc_lines > 0)
		{
			slon_log(SLON_DEBUG4,
					 "remoteHelperThread_%d_%d: return unused line buffers\n",
					 node->no_id, provider->no_id);
			pthread_mutex_lock(&(wd->workdata_lock));
			while (alloc_lines > 0)
			{
				alloc_lines--;
				DLLIST_ADD_HEAD(wd->linepool_head, wd->linepool_tail,
								data_line[alloc_lines]);
			}
			pthread_cond_broadcast(&(wd->linepool_cond));
			pthread_mutex_unlock(&(wd->workdata_lock));
		}

		/*
		 * Close the cursor and rollback the transaction.
		 */
		slon_mkquery(&query, "close LOG; ");
		if (query_execute(node, dbconn, &query) < 0)
			errors++;
		slon_mkquery(&query, "rollback transaction; "
					 "set enable_seqscan = default; "
					 "set enable_indexscan = default; ");
		if (query_execute(node, dbconn, &query) < 0)
			errors++;

		gettimeofday(&tv_now, NULL);
		slon_log(SLON_DEBUG2,
			   "remoteHelperThread_%d_%d: %.3f seconds until close cursor\n",
				 node->no_id, provider->no_id,
				 TIMEVAL_DIFF(&tv_start, &tv_now));

		/*
		 * Change our helper status to DONE and tell the worker thread about
		 * it.
		 */
		slon_log(SLON_DEBUG4,
				 "remoteHelperThread_%d_%d: change helper thread status\n",
				 node->no_id, provider->no_id);
		pthread_mutex_lock(&(provider->helper_lock));
		provider->helper_status = SLON_WG_DONE;
		dstring_reset(&provider->helper_qualification);
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

/* Functions for processing log archives...

   - First, you open the log archive using open_log_archive()

   - Second, you generate the header using generate_archive_header()

   - Third, you need to set up the sync tracking function in the log
     using logarchive_tracking()

   =============  Here Ends The Header of the Log Shipping Archive ==================

   Then come the various queries (inserts/deletes/updates) that
   comprise the "body" of the SYNC.  Probably submitted using
   submit_query_to_archive().

   =============  Here Ends The Body of the Log Shipping Archive ==================

   Finally, the log ends, notably with a COMMIT statement, generated
   using close_log_archive(), which closes the file and renames it
   from ".tmp" form to the final name.
*/


/* Stores the archive name in archive_name (as .sql name) and archive_tmp (.tmp file) */
int open_log_archive (int node_id, char *seqbuf) {
	int i;
	sprintf(archive_name, "%s/slony1_log_%d_", archive_dir, node_id);
	for (i = strlen(seqbuf); i < 20; i++)
		strcat(archive_name, "0");
	strcat(archive_name, seqbuf);
	strcat(archive_name, ".sql");
	strcpy(archive_tmp, archive_name);
	strcat(archive_tmp, ".tmp");
	archive_fp = fopen(archive_tmp, "w");
	if (archive_fp == NULL) {
		return -1;
	} else {
		return 0;
	}
}

int close_log_archive () {
	int rc = 0;
	if (archive_dir) {
		rc = fprintf(archive_fp, 
			     "\n------------------------------------------------------------------\n"
			     "-- End Of Archive Log\n"
			     "------------------------------------------------------------------\n"
			     "commit;\n"
			     "vacuum analyze %s.sl_setsync_offline;\n", 
			     rtcfg_namespace);
		if ( rc < 0 ) 
			return -1;
		rc = fclose(archive_fp);
		archive_fp = NULL;
		if ( rc < 0 ) 
			return -1;
		rc = rename(archive_tmp, archive_name);
	}
	return rc;
}

int logarchive_tracking (const char *namespace, int sub_set, const char *firstseq, 
			 const char *seqbuf, const char *timestamp) {
	return fprintf(archive_fp, "\nselect %s.setsyncTracking_offline(%d, '%s', '%s', '%s');\n"
		       "-- end of log archiving header\n"
		       "------------------------------------------------------------------\n"
		       "-- start of Slony-I data\n"
		       "------------------------------------------------------------------\n",
		       namespace, sub_set, firstseq, seqbuf, timestamp);
}

int submit_query_to_archive(SlonDString *ds) {
	return fprintf(archive_fp, "%s\n", ds->data);
}

int submit_string_to_archive (const char *s) {
	return fprintf(archive_fp, "%s\n", s);
}

/* Raw form used for COPY where we don't want any extra cr/lf output */
int submit_raw_data_to_archive (const char *s) {
	return fprintf(archive_fp, "%s", s);
}

void terminate_log_archive () {
	if (archive_fp) { 
		fclose(archive_fp); 
	}
}

int generate_archive_header (int node_id, const char *seqbuf) {
	return fprintf(archive_fp, 
		       "-- Slony-I log shipping archive\n"
		       "-- Node %d, Event %s\n"
		       "start transaction;\n",
		       node_id, seqbuf);
}

/* write_void_log() writes out a "void" log consisting of the message
 * which must either be a valid SQL query or a SQL comment. */

void write_void_log (int node_id, char *seqbuf, const char *message) {
	open_log_archive(node_id, seqbuf);
	generate_archive_header(node_id, seqbuf);
	submit_string_to_archive(message);
	close_log_archive();
}
