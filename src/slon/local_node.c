/*-------------------------------------------------------------------------
 * local_node.c
 *
 *	Handling of events occuring on the local node.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: local_node.c,v 1.1 2003-12-13 17:02:03 wieck Exp $
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

	if ((conn = slon_connectdb(local_conninfo, "local_cleanup")) == NULL)
	{
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}

	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 60000) == 0)
	{
printf("localCleanupThread: should do cleanup\n");
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

	if ((conn = slon_connectdb(local_conninfo, "local_cleanup")) == NULL)
	{
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}

	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 10000) == 0)
	{
printf("localSyncThread: should check for activity\n");
	}

	slon_disconnectdb(conn);
	pthread_exit(NULL);
}


