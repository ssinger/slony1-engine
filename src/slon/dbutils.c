/*-------------------------------------------------------------------------
 * slon.c
 *
 *	The control framework for the node daemon.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: dbutils.c,v 1.2 2004-02-20 15:13:28 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
 * slon_connectdb
 * ----------
 */
SlonConn *
slon_connectdb(char *conninfo, char *symname)
{
	PGconn	   *dbconn;
	SlonConn   *conn;

	dbconn = PQconnectdb(conninfo);
	if (dbconn == NULL)
	{
		fprintf(stderr, "slon_connectdb: PQconnectdb(\"%s\") failed\n",
				conninfo);
		return NULL;
	}
	if (PQstatus(dbconn) != CONNECTION_OK)
	{
		fprintf(stderr, "slon_connectdb: PQconnectdb(\"%s\") failed - %s",
				conninfo, PQerrorMessage(dbconn));
		PQfinish(dbconn);
		return NULL;
	}

	conn = (SlonConn *)malloc(sizeof(SlonConn));
	memset(conn, 0, sizeof(SlonConn));
	conn->symname = strdup(symname);

	pthread_mutex_init(&(conn->conn_lock), NULL);
	pthread_cond_init(&(conn->conn_cond), NULL);
	pthread_mutex_lock(&(conn->conn_lock));
	
	conn->dbconn = dbconn;

	return conn;
}


/* ----------
 * slon_disconnectdb
 * ----------
 */
void
slon_disconnectdb(SlonConn *conn)
{
	PQfinish(conn->dbconn);
	pthread_mutex_unlock(&(conn->conn_lock));
	pthread_mutex_destroy(&(conn->conn_lock));
	free(conn->symname);
	free(conn);
}


/* ----------
 * slon_make_dummyconn
 * ----------
 */
SlonConn *
slon_make_dummyconn(char *symname)
{
	SlonConn   *conn;

	conn = (SlonConn *)malloc(sizeof(SlonConn));
	memset(conn, 0, sizeof(SlonConn));
	conn->symname = strdup(symname);

	pthread_mutex_init(&(conn->conn_lock), NULL);
	pthread_cond_init(&(conn->conn_cond), NULL);
	pthread_mutex_lock(&(conn->conn_lock));
	
	return conn;
}


/* ----------
 * slon_free_dummyconn
 * ----------
 */
void
slon_free_dummyconn(SlonConn *conn)
{
	pthread_mutex_unlock(&(conn->conn_lock));
	pthread_mutex_destroy(&(conn->conn_lock));
	free(conn->symname);
	free(conn);
}


/* ----
 * db_getLocalNodeId
 *
 *	Query a connection for the value of sequence sl_local_node_id
 * ----
 */
int
db_getLocalNodeId(PGconn *conn)
{
	char		query[1024];
	PGresult   *res;
	int			retval;

	snprintf(query, 1024, "select last_value::int4 from %s.sl_local_node_id",
			rtcfg_namespace);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "cannot get sl_local_node_id - %s",
				PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr, "query '%s' returned %d rows (expected 1)\n",
				query, PQntuples(res));
		PQclear(res);
		return -1;
	}

	retval = strtol(PQgetvalue(res, 0, 0), NULL, 10);
	PQclear(res);
	
	return retval;
}


int
slon_mkquery(SlonDString *dsp, char *fmt, ...)
{
	va_list		ap;
	char	   *s;
	char		buf[64];

	dstring_reset(dsp);

	va_start(ap, fmt);
	while (*fmt)
	{
		switch(*fmt)
		{
			case '%':
				fmt++;
				switch(*fmt)
				{
					case 's':	s = va_arg(ap, char *);
								dstring_append(dsp, s);
								fmt++;
								break;

					case 'q':	s = va_arg(ap, char *);
								while (*s != '\0')
								{
									switch (*s)
									{
										case '\'':
											dstring_addchar(dsp, '\'');
											break;
										case '\\':
											dstring_addchar(dsp, '\\');
											break;
										default:
											break;
									}
									dstring_addchar(dsp, *s);
									s++;
								}
								fmt++;
								break;

					case 'd':	sprintf(buf, "%d", va_arg(ap, int));
								dstring_append(dsp, buf);
								fmt++;
								break;

					default:	dstring_addchar(dsp, '%');
								dstring_addchar(dsp, *fmt);
								fmt++;
								break;
				}
				break;

			case '\\':	fmt++;
						dstring_addchar(dsp, *fmt);
						fmt++;
						break;

			default:	dstring_addchar(dsp, *fmt);
						fmt++;
						break;
		}
	}
	va_end(ap);

	dstring_terminate(dsp);

	return 0;
}


