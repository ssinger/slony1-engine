/*-------------------------------------------------------------------------
 * local_node.c
 *
 *	Handling of events occuring on the local node.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: local_node.c,v 1.3 2003-12-17 21:21:13 wieck Exp $
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
 * slon_localEventThread
 *
 *	Listens on the local database for configuration changes. Uses its
 *	own database connection to be independant from any long running
 *	replication or cleanup activity in order to respond fast.
 * ----------
 */
void *
slon_localEventThread(void *dummy)
{
	SlonConn   *conn;
	char		query1[1024];
	PGresult   *res;
	PGnotify   *notify;
	int			rc;
	int			have_notify;
	int			i, n;

	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(local_conninfo, "local_listen")) == NULL)
	{
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}

	/*
	 * Execute the statement 'listen "_@CLUSTER_NAME@_Event"'
	 */
	snprintf(query1, sizeof(query1),
			"listen \"_%s_Event\"",
			local_cluster_name);
	res = PQexec(conn->dbconn, query1);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "cannot %s - %s", query1,
				PQresultErrorMessage(res));
		PQclear(res);
		slon_disconnectdb(conn);

		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}
	PQclear(res);

	/*
	 * Loop forever
	 */
	for (;;)
	{
		/*
		 * Drain old notifications
		 */
		rc = PQconsumeInput(conn->dbconn);
		if (rc != 1)
		{
			fprintf(stderr, "EVENT: %s", PQerrorMessage(conn->dbconn));
			slon_disconnectdb(conn);

			kill(getpid(), SIGTERM);
			pthread_exit(NULL);
		}
		while ((notify = PQnotifies(conn->dbconn)) != NULL)
			free(notify);

		/*
		 * Process all configuration change events we have not
		 * seen yet.
		 */
		have_notify = 0;

		snprintf(query1, sizeof(query1),
				"select ev_seqno, ev_type, "
				"		ev_data1, ev_data2, ev_data3, ev_data4, "
				"		ev_data5, ev_data6, ev_data7, ev_data8 "
				"	from %s.sl_event "
				"	where ev_origin = %d and ev_seqno > '%s' "
				"   order by ev_origin, ev_seqno",
				local_namespace, local_nodeid, local_lastevent);
		res = PQexec(conn->dbconn, query1);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "slon_localEventThread: cannot %s - %s", query1,
					PQresultErrorMessage(res));
			PQclear(res);
			slon_disconnectdb(conn);

			kill(getpid(), SIGTERM);
			pthread_exit(NULL);
		}
		for (i = 0, n = PQntuples(res); i < n; i++)
		{
			char	   *ev_seqno = PQgetvalue(res, i, 0);
			char	   *ev_type  = PQgetvalue(res, i, 1);
			char	   *ev_data1 = PQgetvalue(res, i, 2);
			char	   *ev_data2 = PQgetvalue(res, i, 3);
			char	   *ev_data3 = PQgetvalue(res, i, 4);
			char	   *ev_data4 = PQgetvalue(res, i, 5);

			/* Not needed yet
			char	   *ev_data5 = PQgetvalue(res, i, 6);
			char	   *ev_data6 = PQgetvalue(res, i, 7);
			char	   *ev_data7 = PQgetvalue(res, i, 8);
			char	   *ev_data8 = PQgetvalue(res, i, 9);
			*/

			if (strcmp(ev_type, "SYNC") != 0)
				/*
				 * We don't have any use for local SYNC events,
				 * but we don't want to compare them against every case
				 * as they are the far most often event in production.
				 */
			if (strcmp(ev_type, "STORE_NODE") == 0)
			{
				int		no_id		= strtol(ev_data1, NULL, 10);
				char   *no_comment	= ev_data2;

printf("slon_localEventThread: ev_seqno=%s %s no_id=%d no_comment='%s'\n", 
ev_seqno, ev_type, no_id, no_comment);

				slon_storeNode(no_id, no_comment);
			}
			else if (strcmp(ev_type, "ENABLE_NODE") == 0)
			{
				int		no_id		= strtol(ev_data1, NULL, 10);

printf("slon_localEventThread: ev_seqno=%s %s no_id=%d\n", 
ev_seqno, ev_type, no_id);

				slon_enableNode(no_id);
			}
			else if (strcmp(ev_type, "STORE_PATH") == 0)
			{
				int		pa_server	= strtol(ev_data1, NULL, 10);
				int		pa_client	= strtol(ev_data2, NULL, 10);
				char   *pa_conninfo	= ev_data3;
				int		pa_connretry = strtol(ev_data4, NULL, 10);

if (pa_client == local_nodeid)
printf("slon_localEventThread: ev_seqno=%s %s pa_server=%s pa_client=%s pa_conninfo=%s pa_connretry=%s\n", 
ev_seqno, ev_type, ev_data1, ev_data2, ev_data3, ev_data4);

				if (pa_client == local_nodeid)
					slon_storePath(pa_server, pa_conninfo, pa_connretry);
			}
			else if (strcmp(ev_type, "STORE_LISTEN") == 0)
			{
				int		li_origin	= strtol(ev_data1, NULL, 10);
				int		li_provider	= strtol(ev_data2, NULL, 10);
				int		li_receiver	= strtol(ev_data3, NULL, 10);

if (li_receiver == local_nodeid)
printf("slon_localEventThread: ev_seqno=%s %s li_origin=%s li_provider=%s li_receiver=%s\n",
ev_seqno, ev_type, ev_data1, ev_data2, ev_data3);

				if (li_receiver == local_nodeid)
					slon_storeListen(li_origin, li_provider);
			}
			else if (strcmp(ev_type, "STORE_SET") == 0)
			{
				int		set_id		= strtol(ev_data1, NULL, 10);
				int		set_origin	= strtol(ev_data2, NULL, 10);
				char   *set_comment	= ev_data3;

printf("slon_localEventThread: ev_seqno=%s %s set_id=%s set_origin=%s set_comment=%s\n",
ev_seqno, ev_type, ev_data1, ev_data2, ev_data3);

				slon_storeSet(set_id, set_origin, set_comment);
			}
			else
			{
printf("slon_localEventThread: ev_seqno=%s unknown event type %s\n", 
ev_seqno, ev_type);
			}

			if (i == n - 1)
			{
				free(local_lastevent);
				local_lastevent = strdup(ev_seqno);
			}
		}
		PQclear(res);

		/*
		 * Check for new notifications.
		 */
		rc = PQconsumeInput(conn->dbconn);
		if (rc != 1)
		{
			fprintf(stderr, "EVENT: %s", PQerrorMessage(conn->dbconn));
			slon_disconnectdb(conn);

			kill(getpid(), SIGTERM);
			pthread_exit(NULL);
		}
		while ((notify = PQnotifies(conn->dbconn)) != NULL)
		{
			free(notify);
			have_notify = 1;
		}
		if (have_notify != 0)
			continue;

		/*
		 * No more unprocessed events and no notifications either.
		 * Wait for a new asynchronous notify.
		 */
		if (sched_wait_conn(conn, SCHED_WAIT_SOCK_READ) < 0)
			break;
	}

	/*
	 * If we reach here sched_wait_conn() told us to commit suicide.
	 * Close the database connection and terminate the thread.
	 */
	slon_disconnectdb(conn);
	pthread_exit(NULL);
}


/* ----------
 * slon_localCleanupThread
 *
 *	Periodically calls the stored procedure to remove old events
 *	and log data and vacuums those tables.
 * ----------
 */
void *
slon_localCleanupThread(void *dummy)
{
	SlonConn   *conn;
	char		query1[1024];
	char		query2[1024];
	PGconn	   *dbconn;
	PGresult   *res;
	struct timeval	tv_start;
	struct timeval	tv_end;

	if ((conn = slon_connectdb(local_conninfo, "local_cleanup")) == NULL)
	{
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}
	dbconn = conn->dbconn;

	snprintf(query1, sizeof(query1), "select %s.cleanupEvent();", 
			local_namespace);
	snprintf(query2, sizeof(query2),
			"vacuum %s.sl_event; "
			"vacuum %s.sl_confirm; "
			"vacuum %s.sl_log_1; "
			"vacuum %s.sl_log_2;",
			local_namespace, 
			local_namespace, 
			local_namespace, 
			local_namespace);

	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 60000) == 0)
	{
		/*
		 * Call the stored procedure cleanupEvent()
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, query1);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "slon_localCleanupThread: \"%s\" - %s",
					query1, PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
printf("slon_localCleanupThread: %8.3f seconds for cleanupEvent()\n",
TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Detain the usual suspects
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, query2);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "slon_localCleanupThread: \"%s\" - %s",
					query2, PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
printf("slon_localCleanupThread: %8.3f seconds for vacuuming\n",
TIMEVAL_DIFF(&tv_start, &tv_end));

	}

	slon_disconnectdb(conn);
	pthread_exit(NULL);
}


/* ----------
 * slon_localSyncThread
 *
 *	Generate SYNC event if local database activity created new log info.
 * ----------
 */
void *
slon_localSyncThread(void *dummy)
{
	SlonConn   *conn;
	char		last_actseq_buf[64];
	char		query1[1024];
	char		query2[1024];
	PGconn	   *dbconn;
	PGresult   *res;

	if ((conn = slon_connectdb(local_conninfo, "local_cleanup")) == NULL)
	{
		slon_abort();
		pthread_exit(NULL);
	}
	dbconn = conn->dbconn;

	last_actseq_buf[0] = '\0';
	snprintf(query1, sizeof(query1),
			"start transaction;"
			"set transaction isolation level serializable;"
			"select last_value from %s.sl_action_seq;",
			local_namespace);
	snprintf(query2, sizeof(query2),
			"select %s.createEvent('_%s', 'SYNC', NULL);"
			"select currval('%s.sl_event_seq');",
			local_namespace, local_cluster_name, local_namespace);

	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 10000) == 0)
	{
		/*
		 * Start a serializable transaction and get the last value
		 * from the action sequence number.
		 */
		res = PQexec(dbconn, query1);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "slon_localSyncThread: \"%s\" - %s",
					query1, PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}

		/*
		 * Check if it's identical to the last know seq.
		 */
		if (strcmp(last_actseq_buf, PQgetvalue(res, 0, 0)) != 0)
		{
			/*
			 * Action sequence has changed, generate a SYNC event
			 * and read the resulting currval of the event sequence.
			 */
			strcpy(last_actseq_buf, PQgetvalue(res, 0, 0));

			PQclear(res);
			res = PQexec(dbconn, query2);
			if (PQresultStatus(res) != PGRES_TUPLES_OK)
			{
				fprintf(stderr, "slon_localSyncThread: \"%s\" - %s",
						query2, PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
				break;
			}
printf("slon_localSyncThread: new sl_action_seq %s - SYNC %s\n", 
last_actseq_buf, PQgetvalue(res, 0, 0));
			PQclear(res);
			
			/*
			 * Commit the transaction
			 */
			res = PQexec(dbconn, "commit transaction;");
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "slon_localSyncThread: \"commit transaction;\" - %s",
						PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
				break;
			}
			PQclear(res);
		}
		else
		{
			/*
			 * No database activity detected - rollback.
			 */
			res = PQexec(dbconn, "rollback transaction;");
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "slon_localSyncThread: \"rollback transaction;\" - %s",
						PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
				break;
			}
			PQclear(res);
		}
	}

	slon_disconnectdb(conn);
	pthread_exit(NULL);
}


