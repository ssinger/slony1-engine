/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: cleanup_thread.c,v 1.20 2005-03-07 23:27:03 cbbrowne Exp $
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


/*
 * ---------- Global data ----------
 */
int			vac_frequency = SLON_VACUUM_FREQUENCY;
static unsigned long earliest_xid = 0;
static unsigned long get_earliest_xid (PGconn *dbconn);
/*
 * ---------- cleanupThread_main
 *
 * Periodically calls the stored procedure to remove old events and log data and
 * vacuums those tables. ----------
 */
void *
cleanupThread_main(void *dummy)
{
	SlonConn   *conn;
	SlonDString query1;
	SlonDString query2;
	SlonDString query3;

	PGconn	   *dbconn;
	PGresult   *res;
	PGresult   *res2;
	struct timeval tv_start;
	struct timeval tv_end;
	int			n	  ,
				t;
	int			vac_count = 0;
	char *vacuum_action;

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
	 * Build the query string for calling the cleanupEvent() stored procedure
	 */
	dstring_init(&query1);
	slon_mkquery(&query1, "select %s.cleanupEvent(); ", rtcfg_namespace);
	dstring_init(&query2);

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
		if (vac_frequency != 0 && ++vac_count >= vac_frequency)
		{
        unsigned long latest_xid;
			vac_count = 0;
			latest_xid = get_earliest_xid(dbconn);
			if (earliest_xid != latest_xid) {
			  vacuum_action = "vacuum analyze";
			} else {
			  vacuum_action = "analyze";
			  slon_log(SLON_DEBUG4, "cleanupThread: xid %d still active - analyze instead\n",
				   earliest_xid);
			}
            earliest_xid = latest_xid;
			/*
			 * Build the query string for vacuuming replication runtime data
			 * and event tables
			 */
			dstring_init(&query3);
			slon_mkquery(&query3,
				     "%s %s.sl_event; "
				     "%s %s.sl_confirm; "
				     "%s %s.sl_setsync; "
				     "%s %s.sl_log_1; "
				     "%s %s.sl_log_2;"
				     "%s %s.sl_seqlog;"
				     "%s pg_catalog.pg_listener;",
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action,
				     rtcfg_namespace,
				     vacuum_action
				     );

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

			/*
			 * Free Resources
			 */
			dstring_free(&query3);

		}
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


static unsigned long get_earliest_xid (PGconn *dbconn) {
	unsigned long lo = 2147483647;
	unsigned long minhi = -1;
	unsigned long minlo = lo;
	unsigned long xid;
	long n,t;
	PGresult   *res;
	SlonDString query1;
	dstring_init(&query1);
	slon_mkquery(&query1, "select transaction from pg_catalog.pg_locks where transaction is not null;");
	res = PQexec(dbconn, dstring_data(&query1));
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		slon_log(SLON_FATAL, "cleanupThread: could not read locks from pg_locks!");
		PQclear(res);
		slon_abort();
		return -1;
	} else {
		n = PQntuples(res);
		for (t = 0; t < n; t++) {
			xid = atoi(PQgetvalue(res, t, 0));
			printf ("xid: %d\n", xid);
			if (xid > lo) {
				if (xid < minlo)
					minlo = xid;
			} else {
				if (xid < minhi)
					minhi = xid;
			}
		}
	}
	printf("minhi: %d minlo: %d\n", minlo, minhi);
	if ((minhi - lo) < minlo)
		return minlo;
	else 
		return minhi;
}
