/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: cleanup_thread.c,v 1.13.2.5 2005-01-12 03:15:36 darcyb Exp $
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
	SlonDString	query3;
	PGconn	   *dbconn;
	PGresult   *res;
	PGresult   *res2;
	struct timeval	tv_start;
	struct timeval	tv_end;
	int			n, t;
	int			vac_count = 0;

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
	slon_mkquery(&query1, "select %s.cleanupEvent(); ", rtcfg_namespace);
	dstring_init(&query2);

	/*
	 * Build the query string for vacuuming replication
	 * runtime data and event tables
	 */
	dstring_init(&query3);
	slon_mkquery(&query3, 
			"vacuum analyze %s.sl_event; "
			"vacuum analyze %s.sl_confirm; "
			"vacuum analyze %s.sl_setsync; "
			"vacuum analyze %s.sl_log_1; "
			"vacuum analyze %s.sl_log_2;"
			"vacuum analyze %s.sl_seqlog;"
			"vacuum analyze pg_catalog.pg_listener;",
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace, 
			rtcfg_namespace);

	/*
	 * Loop until shutdown time arrived
	 */
	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, SLON_CLEANUP_SLEEP * 1000) == SCHED_STATUS_OK)
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
		 * Clean up the logs
		 */
		gettimeofday(&tv_start, NULL);
		slon_mkquery(&query2,
				"select ev_origin, ev_seqno, ev_minxid "
				"from %s.sl_event "
				"where (ev_origin, ev_seqno) in "
				"    (select ev_origin, min(ev_seqno) "
				"     from %s.sl_event "
				"     where ev_type = 'SYNC' "
				"     group by ev_origin); ",
				rtcfg_namespace, rtcfg_namespace);
		res = PQexec(dbconn, dstring_data(&query2));
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_FATAL,
					"cleanupThread: \"%s\" - %s",
					dstring_data(&query2), PQresultErrorMessage(res));
			PQclear(res);
			slon_abort();
			break;
		}
		n = PQntuples(res);
		for (t = 0; t < n; t++)
		{
			slon_mkquery(&query2,
					"delete from %s.sl_log_1 "
					"where log_origin = '%s' "
					"and log_xid < '%s'; "
					"delete from %s.sl_log_2 "
					"where log_origin = '%s' "
					"and log_xid < '%s'; "
					"delete from %s.sl_seqlog "
					"where seql_origin = '%s' "
					"and seql_ev_seqno < '%s'; ",
					rtcfg_namespace, PQgetvalue(res, t, 0),
								PQgetvalue(res, t, 2),
					rtcfg_namespace, PQgetvalue(res, t, 0),
								PQgetvalue(res, t, 2),
					rtcfg_namespace, PQgetvalue(res, t, 0),
								PQgetvalue(res, t, 1));
			res2 = PQexec(dbconn, dstring_data(&query2));
			if (PQresultStatus(res2) != PGRES_COMMAND_OK)
			{
				slon_log(SLON_FATAL,
						"cleanupThread: \"%s\" - %s",
						dstring_data(&query2), PQresultErrorMessage(res2));
				PQclear(res);
				PQclear(res2);
				slon_abort();
				break;
			}
			PQclear(res2);
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
		slon_log(SLON_DEBUG1,
				"cleanupThread: %8.3f seconds for delete logs\n",
				TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Detain the usual suspects (vacuum event and log data)
		 */
		if (++vac_count >= SLON_VACUUM_FREQUENCY)
		{
			vac_count = 0;

			gettimeofday(&tv_start, NULL);
			res = PQexec(dbconn, dstring_data(&query3));
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				slon_log(SLON_FATAL,
						"cleanupThread: \"%s\" - %s",
						dstring_data(&query3), PQresultErrorMessage(res));
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
	}

	/*
	 * Free Resources
	 */
	dstring_free(&query1);
	dstring_free(&query2);
	dstring_free(&query3);

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


/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */

