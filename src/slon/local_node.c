/*-------------------------------------------------------------------------
 * local_node.c
 *
 *	Handling of events occuring on the local node.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: local_node.c,v 1.2 2003-12-16 17:00:34 wieck Exp $
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
	char		query[256];
	PGresult   *res;
	PGnotify   *notify;
	int			rc;
	int			have_notify;

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
	snprintf(query, sizeof(query), "listen \"_%s_Event\"", local_cluster_name);
	res = PQexec(conn->dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "cannot %s - %s", query,
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
printf("localEventThread: need to scan for configuration changes\n");

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


