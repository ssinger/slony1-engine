/*-------------------------------------------------------------------------
 * sync_thread.c
 *
 *	Implementation of the thread generating SYNC evens.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: sync_thread.c,v 1.8 2004-04-02 03:49:21 wieck Exp $
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
 * slon_localSyncThread
 *
 *	Generate SYNC event if local database activity created new log info.
 * ----------
 */
void *
syncThread_main(void *dummy)
{
	SlonConn   *conn;
	char		last_actseq_buf[64];
	SlonDString	query1;
	SlonDString	query2;
	PGconn	   *dbconn;
	PGresult   *res;
	int			restart_count = 1440;

	slon_log(SLON_DEBUG1,
			"syncThread: thread starts\n");

	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "local_sync")) == NULL)
		slon_abort();
	dbconn = conn->dbconn;

	/*
	 * We don't initialize the last known action sequence to the
	 * actual value. This causes that we create a SYNC event
	 * allways on startup, just in case.
	 */
	last_actseq_buf[0] = '\0';

	/*
	 * Build the query that starts a transaction and retrieves
	 * the last value from the action sequence.
	 */
	dstring_init(&query1);
	slon_mkquery(&query1, 
			"start transaction;"
			"set transaction isolation level serializable;"
			"select last_value from %s.sl_action_seq;",
			rtcfg_namespace);

	/*
	 * Build the query that calls createEvent() for the SYNC
	 */
	dstring_init(&query2);
	slon_mkquery(&query2,
			"select %s.createEvent('_%s', 'SYNC', NULL);",
			rtcfg_namespace, rtcfg_cluster_name);

	while (sched_wait_time(conn, SCHED_WAIT_SOCK_READ, 10000) == SCHED_STATUS_OK)
	{
		/*
		 * Start a serializable transaction and get the last value
		 * from the action sequence number.
		 */
		res = PQexec(dbconn, dstring_data(&query1));
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_FATAL,
					"syncThread: \"%s\" - %s",
					dstring_data(&query1), PQresultErrorMessage(res));
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
			res = PQexec(dbconn, dstring_data(&query2));
			if (PQresultStatus(res) != PGRES_TUPLES_OK)
			{
				slon_log(SLON_FATAL,
						"syncThread: \"%s\" - %s",
						dstring_data(&query2), PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
				break;
			}
			slon_log(SLON_DEBUG2,
					"syncThread: new sl_action_seq %s - SYNC %s\n", 
					last_actseq_buf, PQgetvalue(res, 0, 0));
			PQclear(res);
			
			/*
			 * Commit the transaction
			 */
			res = PQexec(dbconn, "commit transaction;");
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				slon_log(SLON_FATAL,
						"syncThread: \"commit transaction;\" - %s",
						PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
			}
			PQclear(res);
		}
		else
		{
			/*
			 * No database activity detected - rollback.
			 */
			PQclear(res);
			res = PQexec(dbconn, "rollback transaction;");
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				slon_log(SLON_FATAL,
						"syncThread: \"rollback transaction;\" - %s",
						PQresultErrorMessage(res));
				PQclear(res);
				slon_abort();
			}
			PQclear(res);
		}

		/*
		 * Once every four hours we restart the node daemon to avoid
		 * any possible resource leakage to pile up.
		 */
		if (--restart_count == 0)
			slon_restart();
	}

	dstring_free(&query1);
	dstring_free(&query2);
	slon_disconnectdb(conn);

	slon_log(SLON_DEBUG1, "syncThread: thread done\n");
	pthread_exit(NULL);
}


