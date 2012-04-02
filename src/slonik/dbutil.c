/*-------------------------------------------------------------------------
 * dbutil.c
 *
 *	General database support functions.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#ifndef WIN32

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include "config_msvc.h"
#else
#include "config.h"
#endif
#include "types.h"
#include "libpq-fe.h"
#include "slonik.h"

/*
 * Global data
 */
int			db_notice_silent = false;
SlonikStmt *db_notice_stmt = NULL;

extern int	current_try_level;

/*
 * Local functions
 */
static int	slon_appendquery_int(SlonDString * dsp, char *fmt, va_list ap);

#ifdef HAVE_PQSETNOTICERECEIVER

/* ----------
 * db_notice_recv
 *
 *	PostgreSQL specific notice message processor
 * ----------
 */
void
db_notice_recv(void *arg, const PGresult *res)
{
	/*
	 * Suppress notice messages when we're silenced
	 */
	if (db_notice_silent)
		return;

	/*
	 * Print the message including script location info
	 */
	if (db_notice_stmt == NULL)
	{
		fprintf(stderr, "<unknown>:<unknown>: %s",
				PQresultErrorMessage(res));
	}
	else
	{
		fprintf(stderr, "%s:%d: %s",
				db_notice_stmt->stmt_filename,
				db_notice_stmt->stmt_lno,
				PQresultErrorMessage(res));
	}
}
#else							/* !HAVE_PQSETNOTICERECEIVER */

/* ----------
 * db_notice_recv
 *
 *	PostgreSQL specific notice message processor
 * ----------
 */
void
db_notice_recv(void *arg, const char *msg)
{
	/*
	 * Suppress notice messages when we're silenced
	 */
	if (db_notice_silent)
		return;

	/*
	 * Print the message including script location info
	 */
	if (db_notice_stmt == NULL)
	{
		fprintf(stderr, "<unknown>:<unknown>: %s", msg);
	}
	else
	{
		fprintf(stderr, "%s:%d: %s",
				db_notice_stmt->stmt_filename,
				db_notice_stmt->stmt_lno, msg);
	}
}
#endif   /* !HAVE_PQSETNOTICERECEIVER */

/* ----------
 * db_connect
 * ----------
 */
int
db_connect(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	PGconn	   *dbconn;
	SlonDString query;
	PGresult   *res;

	db_notice_stmt = stmt;

	/* ----
	 * Create the native PostgreSQL database connection;
	 * ----
	 */
	dbconn = PQconnectdb(adminfo->conninfo);
	if (dbconn == NULL)
	{
		printf("%s:%d: FATAL: PQconnectdb() failed\n",
			   stmt->stmt_filename, stmt->stmt_lno);
		return -1;
	}

	if (PQstatus(dbconn) != CONNECTION_OK)
	{
		printf("%s:%d: %s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   PQerrorMessage(dbconn));
		PQfinish(dbconn);
		return -1;
	}

	/* ----
	 * Catch NOTICE messages from the backend.
	 * ----
	 */
#ifdef HAVE_PQSETNOTICERECEIVER
	PQsetNoticeReceiver(dbconn, db_notice_recv, NULL);
#else
	PQsetNoticeProcessor(dbconn, db_notice_recv, NULL);
#endif   /* !HAVE_PQSETNOTICERECEIVER */

	/* ----
	 * Switch to ISO datestyle and set the replication role
	 * to local so that database changes we eventually make don't
	 * invoke any user or replication triggers by accident.
	 * ----
	 */
	dstring_init(&query);
	slon_mkquery(&query, "SET datestyle TO 'ISO'; "
				 "SET session_replication_role TO local; ");


	adminfo->dbconn = dbconn;
	if (db_exec_command(stmt, adminfo, &query) < 0)
	{
		printf("Unable to set session configuration parameters\n");
		dstring_free(&query);
		return -1;
	}
	dstring_free(&query);

	dstring_init(&query);
	slon_mkquery(&query, "select 1 from pg_catalog.pg_settings where name= 'application_name'; ");
	res = db_exec_select(stmt, adminfo, &query);

	if (res == NULL)
		return -1;

	dstring_free(&query);

	if (PQntuples(res) == 0)
	{
		/* Unable to set application_name on this version of PostgreSQL */
		PQclear(res);
	}
	else
	{
		PQclear(res);
		dstring_init(&query);
		slon_mkquery(&query, "SET application_name TO 'slonik'; ");
		adminfo->dbconn = dbconn;
		if (db_exec_command(stmt, adminfo, &query) < 0)
		{
			printf("Unable to set application name ?!?\n");
			return -1;
		}
		dstring_free(&query);
	}
	/* ----
	 * Commit the changes to the session settings.
	 * ----
	 */
	return db_commit_xact(stmt, adminfo);
}


/* ----------
 * db_disconnect
 * ----------
 */
int
db_disconnect(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	int			rc = 0;

	if (adminfo->dbconn == NULL)
		return 0;

	if (adminfo->have_xact)
		rc = db_rollback_xact(stmt, adminfo);

	PQfinish(adminfo->dbconn);
	adminfo->dbconn = NULL;

	return rc;
}


/* ----------
 * db_exec_command
 *
 *	Execute a query and check that we get a positive result code.
 * ----------
 */
int
db_exec_command(SlonikStmt * stmt, SlonikAdmInfo * adminfo, SlonDString * query)
{
	PGresult   *res;
	int			retval;

	db_notice_stmt = stmt;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	res = PQexec(adminfo->dbconn, dstring_data(query));
	if (PQresultStatus(res) != PGRES_COMMAND_OK &&
		PQresultStatus(res) != PGRES_TUPLES_OK &&
		PQresultStatus(res) != PGRES_EMPTY_QUERY)
	{
		fprintf(stderr, "%s:%d: %s %s - %s",
				stmt->stmt_filename, stmt->stmt_lno,
				PQresStatus(PQresultStatus(res)),
				dstring_data(query), PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}

	retval = strtol(PQcmdTuples(res), NULL, 10);
	PQclear(res);

	return retval;
}


/* ----------
 * db_exec_evcommand
 *
 *	Execute a stored procedure returning an event sequence and remember
 *	that in the admin info for later wait events.
 * ----------
 */
int
db_exec_evcommand(SlonikStmt * stmt, SlonikAdmInfo * adminfo, SlonDString * query)
{
	PGresult   *res;

	db_notice_stmt = stmt;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	res = PQexec(adminfo->dbconn, dstring_data(query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "%s:%d: %s %s - %s",
				stmt->stmt_filename, stmt->stmt_lno,
				PQresStatus(PQresultStatus(res)),
				dstring_data(query), PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr, "%s:%d: %s - did not return 1 row",
				stmt->stmt_filename, stmt->stmt_lno,
				dstring_data(query));
		PQclear(res);
		return -1;
	}

	slon_scanint64(PQgetvalue(res, 0, 0), &(adminfo->last_event));
	PQclear(res);

	return 0;
}


/* ----------
 * db_exec_evcommand_p
 *
 *	Execute a stored procedure returning an event sequence and remember
 *	that in the admin info for later wait events. Differs from
 *	db_exec_evcommand by using PQexecParams().
 * ----------
 */
int
db_exec_evcommand_p(SlonikStmt * stmt, SlonikAdmInfo * adminfo,
					SlonDString * query, int nParams, const Oid *paramTypes,
					const char *const * paramValues, const int *paramLengths,
					const int *paramFormats, int resultFormat)
{
	PGresult   *res;

	db_notice_stmt = stmt;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	res = PQexecParams(adminfo->dbconn, dstring_data(query),
					   nParams, paramTypes, paramValues, paramLengths,
					   paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "%s:%d: %s %s - %s",
				stmt->stmt_filename, stmt->stmt_lno,
				PQresStatus(PQresultStatus(res)),
				dstring_data(query), PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr, "%s:%d: %s - did not return 1 row",
				stmt->stmt_filename, stmt->stmt_lno,
				dstring_data(query));
		PQclear(res);
		return -1;
	}

	slon_scanint64(PQgetvalue(res, 0, 0), &(adminfo->last_event));
	PQclear(res);

	return 0;
}


/* ----------
 * db_exec_select
 *
 *	Execute a select query and check that we get set back
 * ----------
 */
PGresult *
db_exec_select(SlonikStmt * stmt, SlonikAdmInfo * adminfo, SlonDString * query)
{
	PGresult   *res;

	db_notice_stmt = stmt;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return NULL;

	res = PQexec(adminfo->dbconn, dstring_data(query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "%s:%d: %s %s - %s",
				stmt->stmt_filename, stmt->stmt_lno,
				PQresStatus(PQresultStatus(res)),
				dstring_data(query), PQresultErrorMessage(res));
		PQclear(res);
		return NULL;
	}

	return res;
}


/* ----------
 * db_get_nodeid
 *
 *	Get the configured no_id from a database
 * ----------
 */
int
db_get_nodeid(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	PGresult   *res;
	SlonDString query;
	int			no_id;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select \"_%s\".getLocalNodeId('_%q');",
				 stmt->script->clustername, stmt->script->clustername);
	res = db_exec_select(stmt, adminfo, &query);
	dstring_free(&query);

	if (res == NULL)
		return -1;

	no_id = strtol(PQgetvalue(res, 0, 0), NULL, 10);
	PQclear(res);

	return no_id;
}


/* ----------
 * db_get_version
 *
 *	Determine the PostgreSQL database version of a connection
 * ----------
 */
int
db_get_version(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	PGresult   *res;
	SlonDString query;
	char		versionstr[7];
	int			major = 0;
	int			minor = 0;
	int			patch = 0;
	int			version = 0;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	dstring_init(&query);
	slon_mkquery(&query, "select version();");
	res = db_exec_select(stmt, adminfo, &query);
	dstring_free(&query);

	if (res == NULL)
		return -1;

	if (sscanf(PQgetvalue(res, 0, 0), "PostgreSQL %d.%d.%d", &major, &minor, &patch) < 2 &&
		sscanf(PQgetvalue(res, 0, 0), "EnterpriseDB %d.%d.%d", &major, &minor, &patch) < 2)
	{
		fprintf(stderr, "%s:%d: failed to parse %s for DB version\n",
				stmt->stmt_filename, stmt->stmt_lno,
				PQgetvalue(res, 0, 0));
		PQclear(res);
		return -1;
	}
	PQclear(res);
	snprintf(versionstr, 7, "%.2d%.2d%.2d", major, minor, patch);
	version = atoi(versionstr);
	return version;
}


/* ----------
 * db_begin_xact
 *
 *	Eventually start a transaction
 * ----------
 */
int
db_begin_xact(SlonikStmt * stmt, SlonikAdmInfo * adminfo, bool suppress_locking)
{
	PGresult   *res;

	if (adminfo->have_xact)
		return 0;

	res = PQexec(adminfo->dbconn, "begin transaction; ");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		printf("%s:%d: begin transaction; - %s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);
	if (current_try_level > 0 && !suppress_locking)
	{
		/**
		 * inside of a try block we obtain sl_event_lock
		 * right away.	This is because if sometime later
		 * in the try block needs sl_event_lock, it will
		 * be running in the same transaction and will then
		 * be too late to obtain the lock.
		 */
		SlonDString lock_query;

		dstring_init(&lock_query);
		slon_mkquery(&lock_query, "lock table \"_%s\".sl_event_lock; "
					 ,stmt->script->clustername);
		res = PQexec(adminfo->dbconn, dstring_data(&lock_query));
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			printf("%s:%d: lock table \"_%s\".sl_event_lock; - %s",
				   stmt->stmt_filename, stmt->stmt_lno,
				   stmt->script->clustername,
				   PQresultErrorMessage(res));
			PQclear(res);
			adminfo->have_xact = true;
			db_rollback_xact(stmt, adminfo);
			return -1;
		}
		PQclear(res);

	}

	adminfo->have_xact = true;

	return 0;
}


/* ----------
 * db_commit_xact
 *
 *	Eventually commit a transaction
 * ----------
 */
int
db_commit_xact(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	PGresult   *res;

	if (!adminfo->have_xact)
		return 0;
	adminfo->have_xact = false;
	res = PQexec(adminfo->dbconn, "commit transaction;");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		printf("%s:%d: commit transaction; - %s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	return 0;
}


/* ----------
 * db_rollback_xact
 *
 *	Eventually rollback a transaction
 * ----------
 */
int
db_rollback_xact(SlonikStmt * stmt, SlonikAdmInfo * adminfo)
{
	PGresult   *res;

	if (!adminfo->have_xact)
		return 0;
	adminfo->have_xact = false;
	res = PQexec(adminfo->dbconn, "rollback transaction;");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		printf("%s:%d: rollback transaction; - %s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	return 0;
}


/* ----------
 * db_check_namespace
 *
 *	Check if a given namespace exists in a database
 * ----------
 */
int
db_check_namespace(SlonikStmt * stmt, SlonikAdmInfo * adminfo, char *clustername)
{
	PGresult   *res;
	SlonDString query;
	int			ntuples;

	if (db_begin_xact(stmt, adminfo, false) < 0)
		return -1;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select 1 from \"pg_catalog\".pg_namespace N "
				 "	where N.nspname = '_%q';",
				 clustername);
	res = db_exec_select(stmt, adminfo, &query);
	dstring_free(&query);
	if (res == NULL)
		return -1;

	ntuples = PQntuples(res);
	PQclear(res);

	return ntuples;
}


/* ----------
 * db_check_requirements
 *
 *	Check if a database fits all the Slony-I needs
 * ----------
 */
int
db_check_requirements(SlonikStmt * stmt, SlonikAdmInfo * adminfo, char *clustername)
{
	PGresult   *res;
	SlonDString query;
	int			ntuples;

	if (db_begin_xact(stmt, adminfo, true) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * Check that PL/pgSQL is installed
	 */
	slon_mkquery(&query,
				 "select 1 from \"pg_catalog\".pg_language "
				 "	where lanname = 'plpgsql';");
	res = db_exec_select(stmt, adminfo, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	PQclear(res);
	if (ntuples == 0)
	{
		printf("%s:%d: Error: language PL/pgSQL is not installed "
			   "in database '%s'\n",
			   stmt->stmt_filename, stmt->stmt_lno,
			   adminfo->conninfo);
		dstring_free(&query);
		return -1;
	}

	/*
	 * Check loading of slony1_funcs module
	 */
	slon_mkquery(&query, "load '$libdir/slony1_funcs.%s'; ", SLONY_I_VERSION_STRING);
	if (db_exec_command(stmt, adminfo, &query) < 0)
	{
		printf("%s:%d: Error: the extension for the Slony-I C functions "
			   "cannot be loaded in database '%s'\n",
			   stmt->stmt_filename, stmt->stmt_lno,
			   adminfo->conninfo);
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);

	return 0;
}


/* ----------
 * slon_mkquery
 *
 *	A simple query formatting and quoting function using dynamic string
 *	buffer allocation.
 *	Similar to sprintf() it uses formatting symbols:
 *		%s		String argument
 *		%q		Quoted literal (\ and ' will be escaped)
 *		%d		Integer argument
 * ----------
 */
int
slon_mkquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	dstring_reset(dsp);

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);

	return 0;
}


/* ----------
 * slon_appendquery
 *
 *	Append query string material to an existing dynamic string.
 * ----------
 */
int
slon_appendquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);

	return 0;
}


/* ----------
 * slon_appendquery_int
 *
 *	Implementation of slon_mkquery() and slon_appendquery().
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
