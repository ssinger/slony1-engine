/* ----------------------------------------------------------------------
 * dbutils.c
 *
 *	Database utility functions for Slony-I
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 * ----------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#endif

#include "slon.h"

bool		keep_alive;
int			keep_alive_idle;
int			keep_alive_count;
int			keep_alive_interval;

static int	slon_appendquery_int(SlonDString * dsp, char *fmt, va_list ap);
static int	db_get_version(PGconn *conn);

#if (PG_VERSION_MAJOR < 8)
/* ----
 * This mutex is used to wrap around PQconnectdb. There's a problem that
 * occurs when your libpq is compiled with libkrb (kerberos) which is not
 * threadsafe.	It is especially odd because I'm not using kerberos.
 *
 * This is fixed in libpq in 8.0, but for now (and for older versions we'll just
 * use this mutex.
 * ----
 */
static pthread_mutex_t slon_connect_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


/* ----------
 * slon_connectdb
 * ----------
 */
SlonConn *
slon_connectdb(char *conninfo, char *symname)
{
	PGconn	   *dbconn;
	SlonConn   *conn;
	PGresult   *res;
	SlonDString query;
	int			connpid = -1;

	/*
	 * Create the native database connection
	 */
#if (PG_VERSION_MAJOR < 8)
	pthread_mutex_lock(&slon_connect_lock);
	dbconn = PQconnectdb(conninfo);
	pthread_mutex_unlock(&slon_connect_lock);
#else
	dbconn = PQconnectdb(conninfo);
#endif
	if (dbconn == NULL)
	{
		slon_log(SLON_ERROR,
				 "slon_connectdb: PQconnectdb(\"%s\") failed\n",
				 conninfo);
		return NULL;
	}
	if (PQstatus(dbconn) != CONNECTION_OK)
	{
		slon_log(SLON_ERROR,
				 "slon_connectdb: PQconnectdb(\"%s\") failed - %s",
				 conninfo, PQerrorMessage(dbconn));
		PQfinish(dbconn);
		return NULL;
	}

	setsockopt(PQsocket(dbconn), SOL_SOCKET, SO_KEEPALIVE, &keep_alive,
			   sizeof(int));
#ifndef WIN32
	if (keep_alive)
	{

		if (keep_alive_idle > 0)
#ifdef TCP_KEEPIDLE
			setsockopt(PQsocket(dbconn), IPPROTO_TCP, TCP_KEEPIDLE,
					   &keep_alive_idle, sizeof(keep_alive_idle));
#else
			slon_log(SLON_WARN, "keep_alive_idle is not supported on this platform");
#endif
		if (keep_alive_interval > 0)
#ifdef TCP_KEEPINTVL
			setsockopt(PQsocket(dbconn), IPPROTO_TCP, TCP_KEEPINTVL,
					   &keep_alive_interval, sizeof(keep_alive_interval));
#else
			slon_log(SLON_WARN, "keep_alive_interval is not supported on this platform");
#endif
		if (keep_alive_count > 0)
#ifdef TCP_KEEPCNT
			setsockopt(PQsocket(dbconn), IPPROTO_TCP, TCP_KEEPCNT,
					   &keep_alive_count, sizeof(keep_alive_count));
#else
			slon_log(SLON_WARN, "keep_alive_count is not supported on this platform");
#endif

	}
#else
	/**
	 * Win32 does not support the setsockopt calls for setting keep alive
	 * parameters.	On Win32 this can be adjusted via the registry.
	 * libpq 9.0 and above provide functions for doing this.
	 * If we ever require libpq9.0 or above we could start to use them.
	 * Alternativly someone could re-implement that functionality inside
	 * of slony.
	 */
	if (keep_alive)
	{
		if (keep_alive_idle > 0)
			slon_log(SLON_WARN, "keep_alive_idle is not supported by Slony on Win32");
		if (keep_alive_interval > 0)
			slon_log(SLON_WARN, "keep_alive_interval is not supported by Slony on Win32");
		if (keep_alive_count > 0)
			slon_log(SLON_WARN, "keep_alive_count is not supported by Slony Win32");

	}
#endif

	dstring_init(&query);

	if (sql_on_connection != NULL)
	{

		slon_mkquery(&query, "%s", sql_on_connection);
		res = PQexec(dbconn, dstring_data(&query));
		if (!((PQresultStatus(res) == PGRES_TUPLES_OK) ||
			  (PQresultStatus(res) == PGRES_COMMAND_OK)))
		{
			slon_log(SLON_ERROR,
					 "query %s failed\n",
					 dstring_data(&query));
		}
		PQclear(res);
	}

	/* set the datestyle to ISO */
	slon_mkquery(&query, "set datestyle to 'ISO'");
	res = PQexec(dbconn, dstring_data(&query));
	if (!(PQresultStatus(res) == PGRES_COMMAND_OK))
	{
		slon_log(SLON_ERROR, "Unable to set the datestyle to ISO\n");
	}
	PQclear(res);

	/* Find PID for connection */
	slon_mkquery(&query, "select pg_catalog.pg_backend_pid();");
	res = PQexec(dbconn, dstring_data(&query));
	if (!(PQresultStatus(res) == PGRES_TUPLES_OK))
	{
		slon_log(SLON_ERROR, "Unable to check connection PID\n");
	}
	else
	{
		connpid = strtol(PQgetvalue(res, 0, 0), NULL, 10);
	}
	PQclear(res);

	/*
	 * Embed it into a SlonConn structure used to exchange it with the
	 * scheduler. On return this new connection object is locked.
	 */
	conn = slon_make_dummyconn(symname);
	conn->dbconn = dbconn;
	conn->pg_version = db_get_version(dbconn);
	conn->conn_pid = connpid;
	if (conn->pg_version < 80300)
	{
		slon_log(SLON_ERROR,
				 "slon_connectdb: PQconnectdb(\"%s\") PostgreSQL version not supported\n",
				 conninfo);
		PQfinish(dbconn);
		return NULL;
	}

	slon_log(SLON_CONFIG,
			 "version for \"%s\" is %d\n", conninfo, conn->pg_version);

	if (conn->pg_version >= 80300)
	{
		slon_mkquery(&query, "set escape_string_warning to 'off'");
		res = PQexec(dbconn, dstring_data(&query));
		if (!(PQresultStatus(res) == PGRES_COMMAND_OK))
		{
			slon_log(SLON_ERROR, "Unable to set escape_string_warning to off\n");
		}
		PQclear(res);

		slon_mkquery(&query, "set standard_conforming_strings to 'off'");
		res = PQexec(dbconn, dstring_data(&query));
		if (!(PQresultStatus(res) == PGRES_COMMAND_OK))
		{
			slon_log(SLON_ERROR, "Unable to set the standard_conforming_strings to off\n");
		}
		PQclear(res);
	}

	slon_mkquery(&query, "select %s.store_application_name('slon.%s');",
				 rtcfg_namespace, symname);
	res = PQexec(dbconn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR, "Unable to submit application_name store request\n");
	}
	PQclear(res);


	if (slon_log_level >= SLON_DEBUG1)
	{
		slon_mkquery(&query, "select pg_backend_pid()");
		res = PQexec(dbconn, dstring_data(&query));
		if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			slon_log(SLON_ERROR, "%s: Unable to get backend pid - %s\n",
					 symname, PQresultErrorMessage(res));
		}
		else
		{
			slon_log(SLON_DEBUG1, "%s \"%s\": backend pid = %s\n",
					 symname, conninfo, PQgetvalue(res, 0, 0));
		}
		PQclear(res);
	}

	dstring_free(&query);
	return conn;
}


/* ----------
 * slon_disconnectdb
 * ----------
 */
void
slon_disconnectdb(SlonConn * conn)
{
	/*
	 * Disconnect the native database connection
	 */
	PQfinish(conn->dbconn);
#ifdef SLON_MEMDEBUG
	conn->dbconn = NULL;
#endif

	/*
	 * Unlock and destroy the condition and mutex variables and free memory.
	 */
	slon_free_dummyconn(conn);
}


/* ----------
 * slon_make_dummyconn
 * ----------
 */
SlonConn *
slon_make_dummyconn(char *symname)
{
	SlonConn   *conn;

	/*
	 * Allocate and initialize the SlonConn structure
	 */
	conn = (SlonConn *) malloc(sizeof(SlonConn));
	if (conn == NULL)
	{
		perror("slon_make_dummyconn: malloc()");
		slon_retry();
	}
	memset(conn, 0, sizeof(SlonConn));
	conn->symname = strdup(symname);

	/*
	 * Initialize and lock the condition and mutex variables
	 */
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
slon_free_dummyconn(SlonConn * conn)
{
	/*
	 * Destroy and unlock the condition and mutex variables
	 */
	pthread_cond_destroy(&(conn->conn_cond));
	pthread_mutex_unlock(&(conn->conn_lock));
	pthread_mutex_destroy(&(conn->conn_lock));

	/*
	 * Free allocated memory
	 */
	free(conn->symname);
#ifdef SLON_MEMDEBUG
	conn->symname = NULL;
#endif
	free(conn);
}


/* ----------
 * db_getLocalNodeId
 *
 * Query a connection for the value of sequence sl_local_node_id
 * ----------
 */
int
db_getLocalNodeId(PGconn *conn)
{
	char		query[1024];
	PGresult   *res;
	int			retval;

	/*
	 * Select the last_value from the sl_local_node_id sequence
	 */
	snprintf(query, 1024, "select last_value::int4 from %s.sl_local_node_id",
			 rtcfg_namespace);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR,
				 "cannot get sl_local_node_id - %s",
				 PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		slon_log(SLON_ERROR,
				 "query '%s' returned %d rows (expected 1)\n",
				 query, PQntuples(res));
		PQclear(res);
		return -1;
	}

	/*
	 * Return the result as an integer value
	 */
	retval = strtol(PQgetvalue(res, 0, 0), NULL, 10);
	PQclear(res);

	return retval;
}


/* ----------
 * db_checkSchemaVersion
 *
 *	Check the Slony schema on a connection for the correct version number
 * ----------
 */
int
db_checkSchemaVersion(PGconn *conn)
{
	char		query[1024];
	PGresult   *res;
	int			retval = 0;

	/*
	 * Select the version number from the schema
	 */
	snprintf(query, 1024, "select %s.slonyVersion()",
			 rtcfg_namespace);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR,
				 "cannot get Slony-I schema version - %s",
				 PQresultErrorMessage(res));
		slon_log(SLON_ERROR,
				 "please upgrade Slony-I schema to version %s\n",
				 SLONY_I_VERSION_STRING);
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		slon_log(SLON_ERROR,
				 "query '%s' returned %d rows (expected 1)\n",
				 query, PQntuples(res));
		PQclear(res);
		return -1;
	}

	/*
	 * Check the version string of the schema
	 */
	if (strcmp(PQgetvalue(res, 0, 0), SLONY_I_VERSION_STRING) != 0)
	{
		slon_log(SLON_ERROR,
				 "Slony-I schema version is %s\n",
				 PQgetvalue(res, 0, 0));
		slon_log(SLON_ERROR,
				 "please upgrade Slony-I schema to version %s\n",
				 SLONY_I_VERSION_STRING);
		retval = -1;
	}
	PQclear(res);

	/*
	 * Select the version number from the module
	 */
	snprintf(query, 1024, "select %s.getModuleVersion()",
			 rtcfg_namespace);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR,
				 "cannot get Slony-I module version - %s",
				 PQresultErrorMessage(res));
		slon_log(SLON_ERROR,
				 "please upgrade Slony-I shared module to version %s\n",
				 SLONY_I_VERSION_STRING);
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		slon_log(SLON_ERROR,
				 "query '%s' returned %d rows (expected 1)\n",
				 query, PQntuples(res));
		PQclear(res);
		return -1;
	}

	/*
	 * Check the version string of the module
	 */
	if (strcmp(PQgetvalue(res, 0, 0), SLONY_I_VERSION_STRING) != 0)
	{
		slon_log(SLON_ERROR,
				 "Slony-I module version is %s\n",
				 PQgetvalue(res, 0, 0));
		slon_log(SLON_ERROR,
				 "please upgrade Slony-I shared module to version %s\n",
				 SLONY_I_VERSION_STRING);
		retval = -1;
	}
	PQclear(res);

	return retval;
}


/* ----------
 * slon_mkquery
 *
 * A simple query formatting and quoting function using dynamic string buffer
 * allocation. Similar to sprintf() it uses formatting symbols:
 *	   %s	String argument
 *	   %q	Quoted literal (\ and ' will be escaped)
 *	   %d	Integer argument
 * ----------
 */
void
slon_mkquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	dstring_reset(dsp);

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);
}


/* ----------
 * slon_appendquery
 *
 * Append query string material to an existing dynamic string.
 * ----------
 */
void
slon_appendquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);
}


/* ----------
 * slon_appendquery_int
 *
 * Implementation of slon_mkquery() and slon_appendquery().
 * ----------
 */
static int
slon_appendquery_int(SlonDString * dsp, char *fmt, va_list ap)
{
	char	   *s;
	char		buf[64];

	while (*fmt)
	{
		switch (*fmt)
		{
			case '%':
				fmt++;
				switch (*fmt)
				{
					case 's':
						s = va_arg(ap, char *);
						dstring_append(dsp, s);
						fmt++;
						break;

					case 'q':
						s = va_arg(ap, char *);
						while (s && *s != '\0')
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

					case 'd':
						sprintf(buf, "%d", va_arg(ap, int));
						dstring_append(dsp, buf);
						fmt++;
						break;

					case 'L':
						sprintf(buf, INT64_FORMAT, va_arg(ap, int64));
						dstring_append(dsp, buf);
						fmt++;
						break;

					default:
						dstring_addchar(dsp, '%');
						dstring_addchar(dsp, *fmt);
						fmt++;
						break;
				}
				break;

			case '\\':
				fmt++;
				dstring_addchar(dsp, *fmt);
				fmt++;
				break;

			default:
				dstring_addchar(dsp, *fmt);
				fmt++;
				break;
		}
	}

	dstring_terminate(dsp);

	return 0;
}

static int
db_get_version(PGconn *conn)
{
	PGresult   *res;
	SlonDString query;
	char		versionstr[7];
	int			version = 0;
	int			major = 0;
	int			minor = 0;
	int			patch = 0;

	dstring_init(&query);
	slon_mkquery(&query, "SELECT version();");
	res = PQexec(conn, dstring_data(&query));

	if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		return -1;
	}
	if (sscanf(PQgetvalue(res, 0, 0), "PostgreSQL %d.%d.%d", &major, &minor, &patch) < 2 &&
		sscanf(PQgetvalue(res, 0, 0), "EnterpriseDB %d.%d.%d", &major, &minor, &patch) < 2)
	{
		PQclear(res);
		return -1;
	}
	PQclear(res);
	snprintf(versionstr, 7, "%.2d%.2d%.2d", major, minor, patch);
	version = atoi(versionstr);
	dstring_free(&query);
	return version;
}

/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
