/*-------------------------------------------------------------------------
 * remote_node.c
 *
 *	Handling of connection to a remote node
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: remote_node.c,v 1.1 2003-12-13 17:02:03 wieck Exp $
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
 * slon_remoteEventThread
 *
 *	Listens for events on a remote database connection
 * ----------
 */
void *
slon_remoteEventThread(void *cdata)
{
	SlonNode   *node = (SlonNode *)cdata;
	SlonConn   *conn = NULL;
	SlonConn   *conn_wait = NULL;
	char		buf[256];
	char		query[1024];
	PGresult   *res;
	PGnotify   *notify;
	int			rc;
	int			have_notify;

	for (;;)
	{
		pthread_mutex_lock(&(node->node_lock));
		while (node->event_conn == NULL)
		{
			pthread_mutex_unlock(&(node->node_lock));

			/*
			 * Connect to the remote database
			 */
			snprintf(buf, sizeof(buf), "no_id=%d (%s) listen",
					node->no_id, node->no_comment);
			if ((conn = slon_connectdb(node->pa_conninfo, buf)) == NULL)
			{
				/*
				 * If the connection failed, retry after the
				 * configured delay (pa_connretry).
				 */
				if (conn_wait == NULL)
				{
					snprintf(buf, sizeof(buf), "no_id=%d (%s) timewait",
							node->no_id, node->no_comment);
					conn_wait = slon_make_dummyconn(buf);
				}
				if (sched_wait_time(conn_wait, SCHED_WAIT_TIMEOUT, 
						node->pa_connretry * 1000) < 0)
				{
					/*
					 * scheduler want's us to exit
					 */
					slon_free_dummyconn(conn_wait);
					pthread_exit(NULL);
				}

				continue;
			}

			/*
			 * Connection succeeded ... check if this is the node
			 * that we are trying to connect to.
			 */
			if ((rc = slon_getLocalNodeId(conn->dbconn)) != node->no_id)
			{
				fprintf(stderr, "slon_remoteEventThread: wrong or illegal node %d, should be %d\n",
						rc, node->no_id);
				slon_disconnectdb(conn);
				
				if (conn_wait == NULL)
				{
					snprintf(buf, sizeof(buf), "no_id=%d (%s) timewait",
							node->no_id, node->no_comment);
					conn_wait = slon_make_dummyconn(buf);
				}
				if (sched_wait_time(conn_wait, SCHED_WAIT_TIMEOUT, 
						node->pa_connretry * 1000) < 0)
				{
					/*
					 * scheduler want's us to exit
					 */
					slon_free_dummyconn(conn_wait);
					pthread_exit(NULL);
				}

				continue;
			}

			/*
			 * We have a valid connection to the right node now.
			 */
			pthread_mutex_lock(&(node->node_lock));
			node->event_conn = conn;
			pthread_mutex_unlock(&(node->node_lock));
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
			pthread_mutex_lock(&(node->node_lock));
			slon_disconnectdb(conn);
			node->event_conn = NULL;
			pthread_mutex_unlock(&(node->node_lock));

			/*
			 * Reconnect to the database
			 */
			continue;
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
				fprintf(stderr, "slon_remoteEventThread: %s",
						PQerrorMessage(conn->dbconn));
				break;
			}
			while ((notify = PQnotifies(conn->dbconn)) != NULL)
				free(notify);

			/*
			 * Process all configuration change events we have not
			 * seen yet.
			 */
			have_notify = 0;
printf("remoteEventThread: need to scan for new events\n");

			/*
			 * Check for new notifications.
			 */
			rc = PQconsumeInput(conn->dbconn);
			if (rc != 1)
			{
				fprintf(stderr, "slon_remoteEventThread: %s",
						PQerrorMessage(conn->dbconn));
				break;
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
			{
				pthread_mutex_lock(&(node->node_lock));
				slon_disconnectdb(conn);
				if (conn_wait != NULL)
					slon_free_dummyconn(conn_wait);
				pthread_mutex_unlock(&(node->node_lock));

				pthread_exit(NULL);
			}
		
		}

		/*
		 * If we reach here something during DB access went wrong.
		 * Close the database connection and reconnect.
		 */
		pthread_mutex_lock(&(node->node_lock));
		slon_disconnectdb(conn);
		node->event_conn = NULL;
		pthread_mutex_unlock(&(node->node_lock));
	}
}


