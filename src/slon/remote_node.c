/*-------------------------------------------------------------------------
 * remote_node.c
 *
 *	Handling of connection to a remote node
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: remote_node.c,v 1.3 2004-01-09 21:33:14 wieck Exp $
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


static int		slon_forward_confirm(PGconn *src_dbconn, PGconn *dest_dbconn);
static int		slon_local_confirm(SlonNode *node, char *ev_origin,
									char *ev_seqno);
static int		slon_local_storeevent(SlonNode *node, char *ev_origin,
									char *ev_seqno, char *ev_timestamp,
									char *ev_minxid, char *maxxid,
									char *ev_xip, char *ev_type,
									char *ev_data1, char *ev_data2,
									char *ev_data3, char *ev_data4,
									char *ev_data5, char *ev_data6,
									char *ev_data7, char *ev_data8);



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
	char		confirm_notify_name[64];
	char		query[1024];
	static slon_querybuf qbuf = {0, NULL};
	PGresult   *res;
	PGresult   *res2;
	PGnotify   *notify;
	int			rc;
	int			have_notify;
	PGconn	   *local_dbconn;
	PGconn	   *data_dbconn;
	int			iserror;
	int			numrows;
	int			rownum;

	sprintf(confirm_notify_name, "_%s_Confirm", local_cluster_name);

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
			 * But we want more connections. One to read data from
			 * that node and one to the local database to push
			 * data and event information down.
			 */
			local_dbconn = PQconnectdb(local_conninfo);
			if (PQstatus(local_dbconn) != CONNECTION_OK)
			{
				fprintf(stderr, "slon_remoteEventThread: node %d cannot connect to local DB - %s",
						node->no_id, PQerrorMessage(local_dbconn));
				PQfinish(local_dbconn);
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

			data_dbconn = PQconnectdb(node->pa_conninfo);
			if (PQstatus(local_dbconn) != CONNECTION_OK)
			{
				fprintf(stderr, "slon_remoteEventThread: node %d cannot establish data connection - %s",
						node->no_id, PQerrorMessage(local_dbconn));
				PQfinish(data_dbconn);
				PQfinish(local_dbconn);
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
			 * Done ... remember all of the connections. That will
			 * exit this loop.
			 */
			pthread_mutex_lock(&(node->node_lock));
			node->event_conn   = conn;
			node->local_dbconn = local_dbconn;
			node->data_dbconn  = data_dbconn;
			pthread_mutex_unlock(&(node->node_lock));
		}

		iserror = false;
		for (;;)
		{
			/*
			 * Exchange all confirm information with the remote node
			 */
			if (slon_forward_confirm(node->local_dbconn, node->data_dbconn) < 0)
			{
				iserror = true;
				break;
			}

			if (slon_forward_confirm(node->data_dbconn, node->local_dbconn) < 0)
			{
				iserror = true;
				break;
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
				iserror = true;
				break;
			}
			PQclear(res);

			/*
			 * Execute the statement 'listen "_@CLUSTER_NAME@_Confirm"'
			 */
			snprintf(query, sizeof(query), "listen \"%s\"", confirm_notify_name);
			res = PQexec(conn->dbconn, query);

			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "cannot %s - %s", query,
						PQresultErrorMessage(res));
				PQclear(res);
				iserror = true;
				break;
			}
			PQclear(res);

			break;
		}

		/*
		 * If any error occured during the startup connections
		 * or initial exchange of sl_confirm information, close
		 * all connections, wait for the connretry timeout and
		 * do it all over again.
		 */
		if (iserror)
		{
			pthread_mutex_lock(&(node->node_lock));
			slon_disconnectdb(conn);
			node->event_conn = NULL;
			PQfinish(node->local_dbconn);
			PQfinish(node->data_dbconn);
			pthread_mutex_unlock(&(node->node_lock));

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

			/*
			 * Reconnect to the database
			 */
			continue;
		}

		/*
		 * Loop forever
		 */
		iserror = false;
		while (!iserror)
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
			{
				/*
				 * On receiving a Confirm notify, resync the information
				 */
				if (strcmp(notify->relname, confirm_notify_name) == 0)
				{
					if (slon_forward_confirm(node->data_dbconn, node->local_dbconn) < 0)
					{
						free(notify);
						iserror = true;
						break;
					}
				}
					
				free(notify);
			}

			/*
			 * Select all remote events that we are listening for
			 * on this connection.
			 */
			have_notify = 0;
			snprintf(query, sizeof(query),
					"select ev_origin, ev_seqno, "
					"		ev_timestamp, ev_minxid, ev_maxxid, "
					"		ev_xip, ev_type, "
					"		ev_data1, ev_data2, "
					"		ev_data3, ev_data4, "
					"		ev_data5, ev_data6, "
					"		ev_data7, ev_data8 "
					"	from %s.sl_event EV, %s.sl_listen LI "
					"	where EV.ev_origin = LI.li_origin "
					"	and LI.li_provider = '%d' "
					"	and LI.li_receiver = '%d' "
					"	and EV.ev_seqno > (select max(con_seqno) "
					"			from %s.sl_confirm CON "
					"			where CON.con_origin = EV.ev_origin "
					"			and CON.con_received = '%d') "
					"	order by ev_timestamp, ev_seqno",
					local_namespace, local_namespace,
					node->no_id, local_nodeid,
					local_namespace, local_nodeid);

			res = PQexec(conn->dbconn, query);
			if (PQresultStatus(res) != PGRES_TUPLES_OK)
			{
				fprintf(stderr, "slon_remoteEventThread: cannot %s - %s",
						query, PQresultErrorMessage(res));
				PQclear(res);
				break;
			}

			numrows = PQntuples(res);
			for (rownum = 0; rownum < numrows && !iserror; rownum++)
			{
				char	   *ev_origin    = PQgetvalue(res, rownum, 0);
				char	   *ev_seqno     = PQgetvalue(res, rownum, 1);
				char	   *ev_timestamp = PQgetvalue(res, rownum, 2);
				char	   *ev_minxid    = PQgetvalue(res, rownum, 3);
				char	   *ev_maxxid    = PQgetvalue(res, rownum, 4);
				char	   *ev_xip       = PQgetvalue(res, rownum, 5);
				char	   *ev_type      = PQgetvalue(res, rownum, 6);
				char	   *ev_data1     = PQgetvalue(res, rownum, 7);
				char	   *ev_data2     = PQgetvalue(res, rownum, 8);
				char	   *ev_data3     = PQgetvalue(res, rownum, 9);
				char	   *ev_data4     = PQgetvalue(res, rownum, 10);
				/* Not needed yet
				char	   *ev_data5     = PQgetvalue(res, rownum, 11);
				char	   *ev_data6     = PQgetvalue(res, rownum, 12);
				char	   *ev_data7     = PQgetvalue(res, rownum, 13);
				char	   *ev_data8     = PQgetvalue(res, rownum, 14);
				*/

				/****************************************
				 * Event SYNC
				 ****************************************/
				if (strcmp(ev_type, "SYNC") == 0)
				{
printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s SYNC min=%s max=%s xip=%s\n",
ev_origin, ev_seqno, ev_minxid, ev_maxxid, ev_xip);

					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								NULL, NULL, NULL, NULL,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				/****************************************
				 * Event STORE_NODE
				 ****************************************/
				else if (strcmp(ev_type, "STORE_NODE") == 0)
				{
					int		no_id		= strtol(ev_data1, NULL, 10);
					char   *no_comment	= ev_data2;

printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s %s no_id=%d no_comment='%s'\n",
ev_origin, ev_seqno, ev_type, no_id, no_comment);

					/*
					 * Call the function storeNode_int() on the local database
					 */
					slon_mkquery(&qbuf, 
							"select %s.storeNode_int(%d, '%q')",
							local_namespace, no_id, no_comment);
					res2 = PQexec(node->local_dbconn, qbuf.buf);
					if (PQresultStatus(res2) != PGRES_TUPLES_OK)
					{
						fprintf(stderr, "cannot %s - %s", qbuf.buf,
								PQresultErrorMessage(res2));
						PQclear(res2);
						iserror = true;
						break;
					}
					PQclear(res2);

					/*
					 * Add the node to our in memory configuration
					 */
					slon_storeNode(no_id, no_comment);

					/*
					 * Forward the STORE_NODE event
					 */
					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								ev_data1, ev_data2, NULL, NULL,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				/****************************************
				 * Event ENABLE_NODE
				 ****************************************/
				else if (strcmp(ev_type, "ENABLE_NODE") == 0)
				{
					int		no_id		= strtol(ev_data1, NULL, 10);

printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s %s no_id=%d\n",
ev_origin, ev_seqno, ev_type, no_id);

					/*
					 * Call the function enableNode_int() on the local database
					 */
					slon_mkquery(&qbuf, 
							"select %s.enableNode_int(%d)",
							local_namespace, no_id);
					res2 = PQexec(node->local_dbconn, qbuf.buf);
					if (PQresultStatus(res2) != PGRES_TUPLES_OK)
					{
						fprintf(stderr, "cannot %s - %s", qbuf.buf,
								PQresultErrorMessage(res2));
						PQclear(res2);
						iserror = true;
						break;
					}
					PQclear(res2);

					/*
					 * Enable our in memory configuration of the node
					 */
					if (no_id != local_nodeid)
						slon_enableNode(no_id);

					/*
					 * Forward the ENABLE_NODE event
					 */
					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								ev_data1, NULL, NULL, NULL,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				/****************************************
				 * Event STORE_PATH
				 ****************************************/
				else if (strcmp(ev_type, "STORE_PATH") == 0)
				{
					int		pa_server	= strtol(ev_data1, NULL, 10);
					int		pa_client	= strtol(ev_data2, NULL, 10);
					char   *pa_conninfo	= ev_data3;
					int		pa_connretry = strtol(ev_data4, NULL, 10);

printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s %s pa_server=%d pa_client=%d pa_connifo='%s' pa_connretry=%d\n",
ev_origin, ev_seqno, ev_type, pa_server, pa_client, pa_conninfo, pa_connretry);

					/*
					 * Call the function storePath_int() on the local database
					 */
					slon_mkquery(&qbuf, 
							"select %s.storePath_int(%d, %d, '%q', %d)",
							local_namespace, pa_server, pa_client,
							pa_conninfo, pa_connretry);
					res2 = PQexec(node->local_dbconn, qbuf.buf);
					if (PQresultStatus(res2) != PGRES_TUPLES_OK)
					{
						fprintf(stderr, "cannot %s - %s", qbuf.buf,
								PQresultErrorMessage(res2));
						PQclear(res2);
						iserror = true;
						break;
					}
					PQclear(res2);

					/*
					 * If we are the client of the path, add it
					 * to our in memory configuration
					 */
					if (pa_client == local_nodeid)
						slon_storePath(pa_server, pa_conninfo, pa_connretry);

					/*
					 * Forward the STORE_PATH event
					 */
					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								ev_data1, ev_data2, ev_data3, ev_data4,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				/****************************************
				 * Event STORE_LISTEN
				 ****************************************/
				else if (strcmp(ev_type, "STORE_LISTEN") == 0)
				{
					int		li_origin	= strtol(ev_data1, NULL, 10);
					int		li_provider	= strtol(ev_data2, NULL, 10);
					int		li_receiver	= strtol(ev_data3, NULL, 10);

printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s %s li_origin=%d li_provider=%d li_receiver=%d\n",
ev_origin, ev_seqno, ev_type, li_origin, li_provider, li_receiver);

					/*
					 * Call the function storeListen_int() on the local database
					 */
					slon_mkquery(&qbuf, 
							"select %s.storeListen_int(%d, %d, %d)",
							local_namespace, li_origin,
							li_provider, li_receiver);
					res2 = PQexec(node->local_dbconn, qbuf.buf);
					if (PQresultStatus(res2) != PGRES_TUPLES_OK)
					{
						fprintf(stderr, "cannot %s - %s", qbuf.buf,
								PQresultErrorMessage(res2));
						PQclear(res2);
						iserror = true;
						break;
					}
					PQclear(res2);

					/*
					 * If we are the receiver, add the information to
					 * our in memory configuration
					 */
					if (li_receiver == local_nodeid)
						slon_storeListen(li_origin, li_provider);

					/*
					 * Forward the STORE_LISTEN event
					 */
					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								ev_data1, ev_data2, ev_data3, NULL,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				/****************************************
				 * Event STORE_SET
				 ****************************************/
				else if (strcmp(ev_type, "STORE_SET") == 0)
				{
					int		set_id		= strtol(ev_data1, NULL, 10);
					int		set_origin	= strtol(ev_data2, NULL, 10);
					char   *set_comment	= ev_data3;

printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s %s set_id=%d, set_origin=%d set_comment='%s'\n",
ev_origin, ev_seqno, ev_type, set_id, set_origin, set_comment);

					/*
					 * Call the function storeSet_int() on the local database
					 */
					slon_mkquery(&qbuf, 
							"select %s.storeSet_int(%d, %d, '%q')",
							local_namespace, set_id,
							set_origin, set_comment);
					res2 = PQexec(node->local_dbconn, qbuf.buf);
					if (PQresultStatus(res2) != PGRES_TUPLES_OK)
					{
						fprintf(stderr, "cannot %s - %s", qbuf.buf,
								PQresultErrorMessage(res2));
						PQclear(res2);
						iserror = true;
						break;
					}
					PQclear(res2);

					/*
					 * Add the set to our in memory configuration
					 */
					slon_storeSet(set_id, set_origin, set_comment);

					/*
					 * Forward the STORE_SET event
					 */
					if (slon_local_storeevent(node,
								ev_origin, ev_seqno, ev_timestamp,
								ev_minxid, ev_maxxid, ev_xip, ev_type,
								ev_data1, ev_data2, ev_data3, NULL,
								NULL, NULL, NULL, NULL) < 0)
						iserror = true;
				}
				else
				{
printf("slon_remoteEventThread: ev_origin=%s ev_seqno=%s unknown event type %s\n",
ev_origin, ev_seqno, ev_type);
				}

				if (!iserror)
					if (slon_local_confirm(node, ev_origin, ev_seqno) < 0)
						iserror = true;
			}
			PQclear(res);
			if (iserror)
				break;

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
				/*
				 * On receiving a Confirm notify, resync the information
				 */
				if (strcmp(notify->relname, confirm_notify_name) == 0)
				{
					if (slon_forward_confirm(node->data_dbconn, node->local_dbconn) < 0)
					{
						free(notify);
						iserror = true;
						break;
					}
					continue;
				}
					
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
		if (node->data_dbconn != NULL)
		{
			PQfinish(node->data_dbconn);
			node->data_dbconn = NULL;
		}
		if (node->local_dbconn != NULL)
		{
			PQfinish(node->local_dbconn);
			node->local_dbconn = NULL;
		}
		pthread_mutex_unlock(&(node->node_lock));

		/*
		 * nap a little
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
	}
}


static int
slon_forward_confirm(PGconn *src_dbconn, PGconn *dest_dbconn)
{
	char		query[1024];
	PGresult   *res;
	PGresult   *res2;
	int			numrows;
	int			rownum;
	int			num_updates = 0;

	/*
	 * Query the src database for the top sl_confirm rows
	 */
	snprintf(query, sizeof(query),
			"select con_origin, con_received, "
			"		max(con_seqno) as con_seqno, "
			"		max(con_timestamp) as con_timestamp "
			"	from %s.sl_confirm "
			"	group by con_origin, con_received",
			local_namespace);
	res = PQexec(src_dbconn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "cannot %s - %s", query,
				PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	
	/*
	 * Forward the sl_confirm status if it is newer than in the
	 * destination database
	 */
	numrows = PQntuples(res);
	for (rownum = 0; rownum < numrows; rownum++)
	{
		char	   *con_origin    = PQgetvalue(res, rownum, 0);
		char	   *con_received  = PQgetvalue(res, rownum, 1);
		char	   *con_seqno     = PQgetvalue(res, rownum, 2);
		char	   *con_timestamp = PQgetvalue(res, rownum, 3);

		snprintf(query, sizeof(query), 
				"insert into %s.sl_confirm "
				"	(con_origin, con_received, con_seqno, con_timestamp) "
				"	select '%s'::int4, '%s'::int4, "
				"			'%s'::int8, '%s'::timestamp "
				"		from %s.sl_node N1, %s.sl_node N2 "
				"	where N1.no_id = '%s' and N2.no_id = '%s' "
				"	and not exists "
				"		(select 1 from %s.sl_confirm L"
				"			where L.con_origin = '%s' "
				"			and L.con_received = '%s' "
				"			and L.con_seqno >= '%s')",
				local_namespace,
				con_origin, con_received, con_seqno, con_timestamp,
				local_namespace, local_namespace,
				con_origin, con_received,
				local_namespace,
				con_origin, con_received, con_seqno);
		res2 = PQexec(dest_dbconn, query);
		if (PQresultStatus(res2) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "cannot %s - %s", query,
					PQresultErrorMessage(res2));
			PQclear(res2);
			PQclear(res);
			return -1;
		}
		num_updates += strtol(PQcmdTuples(res2), NULL, 10);
		PQclear(res2);
	}
	PQclear(res);

	/*
	 * If there was anything to forward, NOTIFY on _Confirm
	 */
	if (num_updates > 0)
	{
		snprintf(query, sizeof(query), "notify \"_%s_Confirm\"",
				local_cluster_name);
		res = PQexec(dest_dbconn, query);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "cannot %s - %s", query,
					PQresultErrorMessage(res));
			PQclear(res);
			return -1;
		}
		PQclear(res);
	}

	return num_updates;
}


static int		
slon_local_confirm(SlonNode *node, char *ev_origin, char *ev_seqno)
{
	PGresult   *res;
	char		query[1024];

	/*
	 * Insert the sl_confirm row
	 */
	snprintf(query, sizeof(query),
			"insert into %s.sl_confirm "
			"		(con_origin, con_received, con_seqno, con_timestamp) "
			"		values ('%s', '%d', '%s', current_timestamp)",
			local_namespace, ev_origin, local_nodeid, ev_seqno);
	res = PQexec(node->event_conn->dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "slon_remoteEventThread: cannot %s - %s",
				query, PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	/*
	 * Generate a Confirm event so that all remote nodes pick it up
	 */
	snprintf(query, sizeof(query),
			"notify \"_%s_Confirm\"", local_cluster_name);
	res = PQexec(node->event_conn->dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "slon_remoteEventThread: cannot %s - %s",
				query, PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	return 0;
}


static int
slon_local_storeevent(SlonNode *node, char *ev_origin,
			char *ev_seqno, char *ev_timestamp,
			char *ev_minxid, char *ev_maxxid,
			char *ev_xip, char *ev_type,
			char *ev_data1, char *ev_data2,
			char *ev_data3, char *ev_data4,
			char *ev_data5, char *ev_data6,
			char *ev_data7, char *ev_data8)
{
	static size_t	qlen = 0;
	static char	   *query = NULL;

	PGresult	   *res;
	size_t			len, lena;
	char		   *cp;

	/*
	 * Calculate a 2^ that is large enough to hold the query for sure
	 * and adjust the size of the query buffer to that.
	 */
	len = 512;
	if (ev_data1 != NULL) len += strlen(ev_data1);
	if (ev_data2 != NULL) len += strlen(ev_data2);
	if (ev_data3 != NULL) len += strlen(ev_data3);
	if (ev_data4 != NULL) len += strlen(ev_data4);
	if (ev_data5 != NULL) len += strlen(ev_data5);
	if (ev_data6 != NULL) len += strlen(ev_data6);
	if (ev_data7 != NULL) len += strlen(ev_data7);
	if (ev_data8 != NULL) len += strlen(ev_data8);
	len *= 2;
	for (lena = 8192; lena < len; lena *= 2);

	if (lena > qlen)
	{
		if (query != NULL)
			free(query);
		query = malloc(qlen = lena);
		if (query == NULL)
		{
			fprintf(stderr, "slon_local_storeevent: out of memory\n");
			slon_abort();
			pthread_exit(NULL);
		}
	}

	/*
	 * Construct the INSERT query
	 */
	sprintf(query,
			"insert into %s.sl_event "
			"		(ev_origin, ev_seqno, ev_timestamp, "
			"		 ev_minxid, ev_maxxid, ev_xip, ev_type",
			local_namespace);
	cp = query + strlen(query);
	if (ev_data1 != NULL) {strcpy(cp, ", ev_data1"); cp += 10;}
	if (ev_data2 != NULL) {strcpy(cp, ", ev_data2"); cp += 10;}
	if (ev_data3 != NULL) {strcpy(cp, ", ev_data3"); cp += 10;}
	if (ev_data4 != NULL) {strcpy(cp, ", ev_data4"); cp += 10;}
	if (ev_data5 != NULL) {strcpy(cp, ", ev_data5"); cp += 10;}
	if (ev_data7 != NULL) {strcpy(cp, ", ev_data6"); cp += 10;}
	if (ev_data7 != NULL) {strcpy(cp, ", ev_data7"); cp += 10;}
	if (ev_data8 != NULL) {strcpy(cp, ", ev_data8"); cp += 10;}
	sprintf(cp, ") values ('%s', '%s', '%s', "
			"		'%s', '%s', ",
			ev_origin, ev_seqno, ev_timestamp,
			ev_minxid, ev_maxxid);
	cp += strlen(cp);
	slon_quote(cp, ev_xip, &cp);
	sprintf(cp, ", '%s'", ev_type);
	cp += strlen(cp);
	
	if (ev_data1 != NULL) {*cp++ = ','; slon_quote(cp, ev_data1, &cp);}
	if (ev_data2 != NULL) {*cp++ = ','; slon_quote(cp, ev_data2, &cp);}
	if (ev_data3 != NULL) {*cp++ = ','; slon_quote(cp, ev_data3, &cp);}
	if (ev_data4 != NULL) {*cp++ = ','; slon_quote(cp, ev_data4, &cp);}
	if (ev_data5 != NULL) {*cp++ = ','; slon_quote(cp, ev_data5, &cp);}
	if (ev_data6 != NULL) {*cp++ = ','; slon_quote(cp, ev_data6, &cp);}
	if (ev_data7 != NULL) {*cp++ = ','; slon_quote(cp, ev_data7, &cp);}
	if (ev_data8 != NULL) {*cp++ = ','; slon_quote(cp, ev_data8, &cp);}

	strcpy(cp, ")");

	/*
	 * Execute this moster query
	 */
	res = PQexec(node->local_dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "slon_remoteEventThread: cannot %s - %s", query,
				PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	/*
	 * Also create an Event notify on the local database
	 * so that remote nodes will pick it up.
	 */
	sprintf(query, "notify \"_%s_Event\"", local_cluster_name);
	res = PQexec(node->local_dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "slon_remoteEventThread: cannot %s - %s", query,
				PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	return 0;
}


