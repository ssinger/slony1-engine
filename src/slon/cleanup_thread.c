/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: cleanup_thread.c,v 1.3 2004-02-20 15:28:49 wieck Exp $
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



/* ----------
 * cleanupThread_main
 *
 *	Periodically calls the stored procedure to remove old events
 *	and log data and vacuums those tables.
 * ----------
 */
void *
cleanupThread_main(void *dummy)
{
	SlonConn   *conn;
	SlonDString	query1;
	SlonDString	query2;
	PGconn	   *dbconn;
	PGresult   *res;
	struct timeval	tv_start;
	struct timeval	tv_end;

	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "local_cleanup")) == NULL)
	{
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
	}
	dbconn = conn->dbconn;

	/*
	 * Build the query string for calling the cleanupEvent()
	 * stored procedure
	 */
	dstring_init(&query1);
	slon_mkquery(&query1, "select %s.cleanupEvent();", rtcfg_namespace);

	/*
	 * Build the query string for vacuuming replication
	 * runtime data and event tables
	 */
	dstring_init(&query2);
	slon_mkquery(&query2, 
			"vacuum %s.sl_event; "
			"vacuum %s.sl_confirm; "
			"vacuum %s.sl_log_1; "
			"vacuum %s.sl_log_2;",
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace);

	/*
	 * Loop until shutdown time arrived
	 */
	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 60000) == 0)
	{
		/*
		 * Call the stored procedure cleanupEvent()
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, dstring_data(&query1));
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "cleanupThread_main: \"%s\" - %s",
					dstring_data(&query1), PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
printf("cleanupThread_main: %8.3f seconds for cleanupEvent()\n",
TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Detain the usual suspects (vacuum event and log data)
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, dstring_data(&query2));
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "cleanupThread_main: \"%s\" - %s",
					dstring_data(&query2), PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
printf("cleanupThread_main: %8.3f seconds for vacuuming\n",
TIMEVAL_DIFF(&tv_start, &tv_end));
	}

	/*
	 * Free Resources
	 */
	dstring_free(&query1);
	dstring_free(&query2);

	/*
	 * Disconnect from the database
	 */
	slon_disconnectdb(conn);

	/*
	 * Terminate this thread
	 */
	pthread_exit(NULL);
}


