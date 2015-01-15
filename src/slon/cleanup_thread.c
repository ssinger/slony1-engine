/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
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
#include <sys/time.h>
#include <unistd.h>
#endif

#include "slon.h"


/* ----------
 * Global data
 * ----------
 */
int			vac_frequency = SLON_VACUUM_FREQUENCY;
char	   *cleanup_interval;

static unsigned long earliest_xid = 0;
static unsigned long get_earliest_xid(PGconn *dbconn);

/* ----------
 * cleanupThread_main
 *
 * Periodically calls the stored procedure to remove old events and log data and
 * vacuums those tables.
 * ----------
 */
void *
cleanupThread_main( /* @unused@ */ void *dummy)
{
	SlonConn   *conn;
	SlonDString query_baseclean;
	SlonDString query_cleanup_interval_second;
	SlonDString query2;
	SlonDString query_pertbl;

	PGconn	   *dbconn;
	PGresult   *res;
	PGresult   *res2;
	PGresult   *res3;
	struct timeval tv_start;
	struct timeval tv_end;
	int			t;
	int			vac_count = 0;
	int			vac_enable = SLON_VACUUM_FREQUENCY;
	char	   *vacuum_action;
	int			ntuples;
	int              cleanup_interval_second;
	int vac_bias = 0;

	slon_log(SLON_CONFIG, "cleanupThread: thread starts\n");
	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "local_cleanup")) == NULL)
	{
#ifndef WIN32
		(void) kill(getpid(), SIGTERM);
		pthread_exit(NULL);
#else
		exit(0);
#endif
		/* slon_retry(); */
	}

	dbconn = conn->dbconn;
        monitor_state("local_cleanup", 0, conn->conn_pid, "thread main loop", 0, "n/a");
        /*
         *rnancy : Want the vacuum time bias to be 10% of the cleanup interval
         */
        if (vac_bias == 0)
        {
                /* convert cleanup_interval in second*/
                
                dstring_init(&query_cleanup_interval_second);
                slon_mkquery(&query_cleanup_interval_second,
                                 "select date_part('epoch','%s'::interval);",
                                 cleanup_interval
                );
                
                res3 = PQexec(dbconn, dstring_data(&query_cleanup_interval_second));
                if (PQresultStatus(res3) != PGRES_TUPLES_OK) /* query error */
                        {
                                slon_log(SLON_ERROR,
                                                 "cleanupThread: \"%s\" - %s",
                                                 dstring_data(&query_cleanup_interval_second), PQresultErrorMessage(res3));
                                slon_retry();
                        }
                cleanup_interval_second = atoi(PQgetvalue(res3, 0, 0));
                
                slon_log(SLON_DEBUG1, "cleanupThread: Cleanup interval is : %ds\n", cleanup_interval_second); 

                vac_bias = (int)((cleanup_interval_second * 10) /100);

                PQclear(res3);
                dstring_free(&query_cleanup_interval_second);

        }
        slon_log(SLON_CONFIG, "cleanupThread: bias = %d\n", vac_bias);
       
	/*
	 * Build the query string for calling the cleanupEvent() stored procedure
	 */
	dstring_init(&query_baseclean);
	slon_mkquery(&query_baseclean,
				 "begin;"
				 "lock table %s.sl_config_lock;"
				 "select %s.cleanupEvent('%s'::interval);"
				 "commit;",
				 rtcfg_namespace,
				 rtcfg_namespace,
				 cleanup_interval
		);
	dstring_init(&query2);

	/*
	 * Loop until shutdown time arrived
	 *
	 * Note the introduction of vac_bias and an up-to-100s random "fuzz"; this
	 * reduces the likelihood that having multiple slons hitting the same
	 * cluster will run into conflicts due to trying to vacuum common tables *
	 * such as pg_listener concurrently
	 */
	 while (sched_wait_time(conn, 
                                 SCHED_WAIT_SOCK_READ, 
                                 cleanup_interval_second * 1000 + vac_bias + (rand() % cleanup_interval_second)) == SCHED_STATUS_OK)
                                 
	{
		/*
		 * Call the stored procedure cleanupEvent()
		 */
		monitor_state("local_cleanup", 0, conn->conn_pid, "cleanupEvent", 0, "n/a");
		gettimeofday(&tv_start, NULL);
		res = PQexec(dbconn, dstring_data(&query_baseclean));
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_FATAL,
					 "cleanupThread: \"%s\" - %s",
				  dstring_data(&query_baseclean), PQresultErrorMessage(res));
			PQclear(res);
			slon_retry();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
		slon_log(SLON_INFO,
				 "cleanupThread: %8.3f seconds for cleanupEvent()\n",
				 TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Detain the usual suspects (vacuum event and log data)
		 */
		if (vac_frequency != 0)
		{
			vac_enable = vac_frequency;
		}
		if (++vac_count >= vac_enable)
		{
			unsigned long latest_xid;

			vac_count = 0;

			latest_xid = get_earliest_xid(dbconn);
			vacuum_action = "";
			if (earliest_xid == latest_xid)
			{

				slon_log(SLON_INFO,
					"cleanupThread: xid %d still active - analyze instead\n",
						 earliest_xid);
			}
			else
			{
				if (vac_enable == vac_frequency)
				{
					vacuum_action = "vacuum ";
				}
			}
			earliest_xid = latest_xid;

			/*
			 * Build the query string for vacuuming replication runtime data
			 * and event tables
			 */
			gettimeofday(&tv_start, NULL);

			slon_mkquery(&query2, "select nspname, relname from %s.TablesToVacuum();", rtcfg_namespace);
			res = PQexec(dbconn, dstring_data(&query2));

			/*
			 * for each table...  and we should set up the query to return not
			 * only the table name, but also a boolean to support what's in
			 * the SELECT below; that'll nicely simplify this process...
			 */

			if (PQresultStatus(res) != PGRES_TUPLES_OK) /* query error */
			{
				slon_log(SLON_ERROR,
						 "cleanupThread: \"%s\" - %s",
						 dstring_data(&query2), PQresultErrorMessage(res));
			}
			ntuples = PQntuples(res);
			slon_log(SLON_DEBUG1, "cleanupThread: number of tables to clean: %d\n", ntuples);

			monitor_state("local_cleanup", 0, conn->conn_pid, "vacuumTables", 0, "n/a");
			for (t = 0; t < ntuples; t++)
			{
				char	   *tab_nspname = PQgetvalue(res, t, 0);
				char	   *tab_relname = PQgetvalue(res, t, 1);
				ExecStatusType vrc;

				slon_log(SLON_DEBUG1, "cleanupThread: %s analyze \"%s\".%s;\n",
						 vacuum_action, tab_nspname, tab_relname);
				dstring_init(&query_pertbl);
				slon_mkquery(&query_pertbl, "%s analyze \"%s\".%s;",
							 vacuum_action, tab_nspname, tab_relname);
				res2 = PQexec(dbconn, dstring_data(&query_pertbl));
				vrc = PQresultStatus(res2);
				if (vrc == PGRES_FATAL_ERROR)
				{
					slon_log(SLON_ERROR,
							 "cleanupThread: \"%s\" - %s\n",
					dstring_data(&query_pertbl), PQresultErrorMessage(res2));

					/*
					 * slon_retry(); break;
					 */
				}
				else
				{
					if (vrc == PGRES_NONFATAL_ERROR)
					{
						slon_log(SLON_WARN,
								 "cleanupThread: \"%s\" - %s\n",
								 dstring_data(&query_pertbl), PQresultErrorMessage(res2));

					}
				}
				PQclear(res2);
				dstring_reset(&query_pertbl);
			}
			gettimeofday(&tv_end, NULL);
			slon_log(SLON_INFO,
					 "cleanupThread: %8.3f seconds for vacuuming\n",
					 TIMEVAL_DIFF(&tv_start, &tv_end));

			/*
			 * Free Resources
			 */
			dstring_free(&query_pertbl);
			PQclear(res);
			monitor_state("local_cleanup", 0, conn->conn_pid, "thread main loop", 0, "n/a");
		}
	}

	/*
	 * Free Resources
	 */
	dstring_free(&query_baseclean);
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


/* ----------
 * get_earliest_xid()
 *
 * reads the earliest XID that is still active.
 *
 * The idea is that if, between cleanupThread iterations, this XID has
 * not changed, then an old transaction is still in progress,
 * PostgreSQL is holding onto the tuples, and there is no value in
 * doing VACUUMs of the various Slony-I tables.
 * ----------
 */
static unsigned long
get_earliest_xid(PGconn *dbconn)
{
	int64		xid;
	PGresult   *res;
	SlonDString query;

	dstring_init(&query);
	(void) slon_mkquery(&query, "select pg_catalog.txid_snapshot_xmin(pg_catalog.txid_current_snapshot());");
	res = PQexec(dbconn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "cleanupThread: could not txid_snapshot_xmin()!\n");
		PQclear(res);
		slon_retry();
		return (unsigned long) -1;
	}
	xid = strtoll(PQgetvalue(res, 0, 0), NULL, 10);
	slon_log(SLON_DEBUG1, "cleanupThread: minxid: %d\n", xid);
	PQclear(res);
	dstring_free(&query);
	return (unsigned long) xid;
}
