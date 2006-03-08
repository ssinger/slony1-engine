/*-------------------------------------------------------------------------
 * cleanup_thread.c
 *
 *	Periodic cleanup of confirm-, event- and log-data.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: cleanup_thread.c,v 1.32 2006-03-08 18:29:10 darcyb Exp $
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
 * Global data 
 * ----------
 */
int			vac_frequency = SLON_VACUUM_FREQUENCY;
static int	vac_bias = 0;
static unsigned long earliest_xid = 0;
static unsigned long get_earliest_xid(PGconn *dbconn);

/* The list of tables that need to be vacuumed by Slony-I */
static char *table_list[] = {"%s.sl_event",
	"%s.sl_confirm",
	"%s.sl_setsync",
	"%s.sl_log_1",
	"%s.sl_log_2",
	"%s.sl_seqlog",
	"pg_catalog.pg_listener",
	"pg_catalog.pg_statistic",
	NULL
};


static char tstring[255];		/* string used to store table names for the
								 * VACUUM statements */

/* ----------
 * cleanupThread_main
 *
 * Periodically calls the stored procedure to remove old events and log data and
 * vacuums those tables. 
 * ----------
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
	int			n,
				t;
	int			vac_count = 0;
	int			vac_enable = SLON_VACUUM_FREQUENCY;
	char	   *vacuum_action;

	slon_log(SLON_DEBUG1, "cleanupThread: thread starts\n");

	/*
	 * Want the vacuum time bias to be between 0 and 100 seconds, hence
	 * between 0 and 100000
	 */
	if (vac_bias == 0)
	{
		vac_bias = rand() % (SLON_CLEANUP_SLEEP * 166);
	}
	slon_log(SLON_DEBUG4, "cleanupThread: bias = %d\n", vac_bias);

	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "local_cleanup")) == NULL)
	{
#ifndef WIN32
		kill(getpid(), SIGTERM);
		pthread_exit(NULL);
#else
		exit(0);
#endif
		/* slon_retry(); */
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
	 *
	 * Note the introduction of vac_bias and an up-to-100s random "fuzz"; this
	 * reduces the likelihood that having multiple slons hitting the same
	 * cluster will run into conflicts due to trying to vacuum pg_listener
	 * concurrently
	 */
	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, SLON_CLEANUP_SLEEP * 1000 + vac_bias + (rand() % (SLON_CLEANUP_SLEEP * 166))) == SCHED_STATUS_OK)
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
			slon_retry();
			break;
		}
		PQclear(res);
		gettimeofday(&tv_end, NULL);
		slon_log(SLON_DEBUG1,
				 "cleanupThread: %8.3f seconds for cleanupEvent()\n",
				 TIMEVAL_DIFF(&tv_start, &tv_end));

		/*
		 * Clean up the logs and eventually finish switching logs
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
			slon_retry();
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
						 "and seql_ev_seqno < '%s'; "
						 "select %s.logswitch_finish(); ",
						 rtcfg_namespace, PQgetvalue(res, t, 0),
						 PQgetvalue(res, t, 2),
						 rtcfg_namespace, PQgetvalue(res, t, 0),
						 PQgetvalue(res, t, 2),
						 rtcfg_namespace, PQgetvalue(res, t, 0),
						 PQgetvalue(res, t, 1),
						 rtcfg_namespace);
			res2 = PQexec(dbconn, dstring_data(&query2));
			if (PQresultStatus(res2) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_FATAL,
						 "cleanupThread: \"%s\" - %s",
						 dstring_data(&query2), PQresultErrorMessage(res2));
				PQclear(res);
				PQclear(res2);
				slon_retry();
				break;
			}
			PQclear(res2);

			/*
			 * Eventually kick off a logswitch. This might fail,
			 * but this is not really slon's problem, so we just
			 * shrug and move on if it does.
			 */
			slon_mkquery(&query2,
						 "select %s.logswitch_weekly(); ",
						 rtcfg_namespace);
			res2 = PQexec(dbconn, dstring_data(&query2));
			if (PQresultStatus(res2) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_WARN,
						 "cleanupThread: \"%s\" - %s",
						 dstring_data(&query2), PQresultErrorMessage(res2));
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
		if (vac_frequency !=0)
		{
			vac_enable = vac_frequency;
		}
		if (++vac_count >= vac_enable)
		{
			unsigned long latest_xid;
			int a_vac = 0;

			vac_count = 0;

			dstring_init(&query3);

			/* 
			 * if we are running a version >= 8.1, check to see 
			 * if autovacuum is enabled, if so we should only run
			 * analyze on the tables in table_list[], otherwise it
			 * is ok to vacuum analyze
			 */

			if (conn->pg_version >=80100)
			{
				dstring_reset(&query3);
				slon_mkquery(&query3, "show autovacuum");
				res = PQexec(dbconn, dstring_data(&query3));
				if (PQresultStatus(res)  != PGRES_TUPLES_OK)
				{
					slon_log(SLON_ERROR,
                                                         "cleanupThread: \"%s\" - %s",
                                                   dstring_data(&query3), PQresultErrorMessage(res));

                                        /*
                                         * slon_retry(); break;
                                         */
                                }
				else if (strncmp(PQgetvalue(res, 0, 0), "on", 2) == 0)
				{
					/*
					 * autovacuum is on
					 */
					a_vac=1;
				}
                                PQclear(res);
				dstring_reset(&query3);
			}
			latest_xid = get_earliest_xid(dbconn);
			if (earliest_xid == latest_xid)
			{
				
				vacuum_action = "";
				slon_log(SLON_DEBUG4,
					"cleanupThread: xid %d still active - analyze instead\n",
					earliest_xid);
			}
			else
			{
				if ((vac_enable == vac_frequency) && (a_vac==0))
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
			for (t = 0; table_list[t] != NULL ; t++)
			{

				sprintf(tstring, table_list[t], rtcfg_namespace);
				if (a_vac==1)
				{
					slon_mkquery(&query3,"select (case when pga.enabled ISNULL THEN true ELSE pga.enabled END) "
						"from \"pg_catalog\".pg_namespace PGN, \"pg_catalog\".pg_class PGC LEFT OUTER JOIN "
						"\"pg_catalog\".pg_autovacuum pga ON (PGC.oid = pga.vacrelid) where PGC.relnamespace = PGN.oid "
						"and %s.slon_quote_input('%s')=%s.slon_quote_brute(PGN.nspname) || '.' || %s.slon_quote_brute(PGC.relname);",
					 	rtcfg_namespace,tstring, rtcfg_namespace, rtcfg_namespace);

					res = PQexec(dbconn, dstring_data(&query3));
					if (PQresultStatus(res) != PGRES_TUPLES_OK)  /* query error */
					{
						slon_log(SLON_ERROR,
							 "cleanupThread: \"%s\" - %s",
							   dstring_data(&query3), PQresultErrorMessage(res));
						/*
						 * slon_retry(); break;
						 */
					}
					else	/* no errors */
					{
						if (PQntuples(res) == 1)	/* we want 1 and only 1 row, otherwise skip */
						{
							if (strncmp(PQgetvalue(res, 0, 0), "f", 1) == 0) 
							{
								/* 
								 * pg_avac is NOT enabled for this table
								 * so this means we need to handel it internaly
								 */
								if (vac_enable == vac_frequency)
								{
									vacuum_action = "vacuum ";
								}
							}
							else
							{
								vacuum_action = "";
							}
						}
					}
					PQclear(res);
				}
				dstring_reset(&query3);

				slon_mkquery(&query3,"%s analyze %s;",vacuum_action, tstring);
				res = PQexec(dbconn, dstring_data(&query3));
				if (PQresultStatus(res) != PGRES_COMMAND_OK)  /* query error */
                                {
                 	                slon_log(SLON_ERROR,
	                                        "cleanupThread: \"%s\" - %s",
                                                dstring_data(&query3), PQresultErrorMessage(res));
                                                /*
                                                 * slon_retry(); break;
                                                 */                  
                                }
				PQclear(res);
				dstring_reset(&query3);
			}
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
	long long	xid;
	PGresult   *res;
	SlonDString query1;

	dstring_init(&query1);
	slon_mkquery(&query1, "select %s.getMinXid();", rtcfg_namespace);
	res = PQexec(dbconn, dstring_data(&query1));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "cleanupThread: could not getMinXid()!\n");
		PQclear(res);
		slon_retry();
		return -1;
	}
	xid = strtoll(PQgetvalue(res, 0, 0), NULL, 10);
	slon_log(SLON_DEBUG3, "cleanupThread: minxid: %d\n", xid);
	return (unsigned long)xid;
}


