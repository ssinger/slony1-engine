/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: cleanup_thread.c,v 1.11 2004-06-23 19:49:18 wieck Exp $
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

	slon_log(SLON_DEBUG1, "cleanupThread: thread starts\n");

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
	slon_mkquery(&query1, 
			"select %s.cleanupEvent_1(); "
			"select %s.cleanupEvent_2(); "
			"select %s.cleanupEvent_3();", 
			rtcfg_namespace,
			rtcfg_namespace,
			rtcfg_namespace);

	/*
	 * Build the query string for vacuuming replication
	 * runtime data and event tables
	 */
	dstring_init(&query2);
	slon_mkquery(&query2, 
			"vacuum analyze %s.sl_event; "
			"vacuum analyze %s.sl_confirm; "
			"vacuum analyze %s.sl_setsync; "
			"vacuum analyze %s.sl_log_1; "
			"vacuum analyze %s.sl_log_2;"
			"vacuum analyze %s.sl_seqlog;",
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace);

	/*
	 * Loop until shutdown time arrived
	 */
	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 300000) == SCHED_STATUS_OK)
	{
		/*
		 * Call the stored procedure cleanupEvent()
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, dstring_data(&query1));
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_FATAL,
					"cleanupThread: \"%s\" - %s",
					dstring_data(&query1), PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
		slon_log(SLON_DEBUG1,
				"cleanupThread: %8.3f seconds for cleanupEvent()\n",
				TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Detain the usual suspects (vacuum event and log data)
		 */
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, dstring_data(&query2));
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_FATAL,
					"cleanupThread: \"%s\" - %s",
					dstring_data(&query2), PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
		slon_log(SLON_DEBUG2,
				"cleanupThread: %8.3f seconds for vacuuming\n",
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
	slon_log(SLON_DEBUG1, "cleanupThread: thread done\n");
	pthread_exit(NULL);
}


