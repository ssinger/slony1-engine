/*-------------------------------------------------------------------------
 * slony_logshipper.c
 *
 *	A configuration and admin script utility for Slony-I.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#else
#define sleep(x) Sleep(x*1000)
#define vsnprintf _vsnprintf
#endif

#include "libpq-fe.h"

#include "../slonik/types.h"
#include "slony_logshipper.h"
#include "config.h"


typedef struct archscan_entry_s
{
	char	   *fname;
	struct archscan_entry_s *left;
	struct archscan_entry_s *right;
}	archscan_entry;

/*
 * Global data
 */
int			rescan_interval = 0;
int			parse_errors = 0;
int			opt_quiet = 0;
char	   *destinationfname = NULL;
FILE	   *destinationfp = NULL;
char	   *logfile_path = NULL;
FILE	   *logfile_fp = NULL;
int			max_archives = 1000;
PGconn	   *dbconn = NULL;
bool		logfile_switch_requested = false;
bool		wait_for_resume = false;
bool		shutdown_smart_requested = false;
bool		shutdown_immed_requested = false;

/*
 * Configuration data
 */
char	   *archive_dir = NULL;
char	   *destination_dir = NULL;
char	   *destination_conninfo = NULL;
char	   *cluster_name = NULL;
char	   *namespace = NULL;

RenameObject *rename_list = NULL;
ProcessingCommand *pre_processing_commands = NULL;
ProcessingCommand *post_processing_commands = NULL;
ProcessingCommand *error_commands = NULL;

/*
 * Local commandline options
 */
static int	opt_send_logswitch = 0;
static int	opt_send_resume = 0;
static int	opt_shutdown_smart = 0;
static int	opt_shutdown_immed = 0;
static int	opt_foreground = 0;
static int	opt_cleanup = 0;
static int	opt_nowait = 0;

/*
 * Local data
 */
static archscan_entry *archscan_sort = NULL;
static char current_at_counter[64];
static bool process_in_transaction = false;
static char *current_archive_path = NULL;
static bool suppress_copy = false;
static SlonDString errlog_messages;
static int	archive_count = 0;


/*
 * Local functions
 */
static void usage(void);
static int	process_archive(char *fname);
static int	process_exec_sql(char *sql);
static int	archscan(int optind, int argc, char **argv);
static int archscan_sort_in(archscan_entry ** entpm, char *fname,
				 int optind, int argc, char **argv);
static int	archscan_sort_out(archscan_entry * ent);
static int	get_current_at_counter(void);
static int	idents_are_distinct(char *id1, char *id2);
static int	process_command(char *command, char *inarchive, char *outarchive);
static void notice_processor(void *arg, const char *msg);

/* ----------
 * main
 * ----------
 */
int
main(int argc, const char *argv[])
{
	FILE	   *fp;
	extern int	optind;
	int			opt;
	int			init_rc;
	int			rc;
	char		archive_path[MSGMAX];
	pid_t		pid;

	dstring_init(&errlog_messages);
	logfile_fp = stdout;

	/*
	 * Parse commandline options
	 *
	 */
	while ((opt = getopt(argc, (char **) argv, "hvqcflrtTws:")) != EOF)
	{
		switch (opt)
		{
			case 'h':
				parse_errors++;
				break;

			case 'v':
				printf("slony_logshipper version %s\n", SLONY_I_VERSION_STRING);
				exit(0);
				break;

			case 'q':
				opt_quiet = 1;
				break;

			case 'c':
				opt_cleanup = 1;
				break;

			case 'f':
				opt_foreground = 1;
				break;

			case 'l':
				opt_send_logswitch = 1;
				break;

			case 'r':
				opt_send_resume = 1;
				break;

			case 't':
				opt_shutdown_smart = 1;
				break;

			case 'T':
				opt_shutdown_immed = 1;
				break;

			case 'w':
				opt_nowait = 1;
				break;

			case 's':
				rescan_interval = atoi(optarg);
				break;

			default:
				fprintf(stderr, "unknown option '%c'\n", opt);
				parse_errors++;
				break;
		}
	}
	if (parse_errors || optind >= argc)
		usage();

	/*
	 * Parse the config file
	 *
	 */
	fp = fopen(argv[optind], "r");
	if (fp == NULL)
	{
		fprintf(stderr, "could not open file '%s'\n", argv[optind]);
		return 2;
	}
	scan_new_input_file(fp);
	current_file = (char *) argv[optind++];
	scan_push_string("start_config;");
	parse_errors += yyparse();
	if (parse_errors != 0)
		return 1;

	/*
	 * Check that we have an archive dir
	 */
	if (archive_dir == NULL)
	{
		fprintf(stderr, "ERROR: no archive dir specified in config file\n");
		return 1;
	}

	if (opt_cleanup)
		return ipc_cleanup(archive_dir);
	if (opt_send_logswitch)
		return ipc_send_logswitch(archive_dir);
	if (opt_send_resume)
		return ipc_send_resume(archive_dir);
	if (opt_shutdown_immed)
		return ipc_send_term(archive_dir, true);
	if (opt_shutdown_smart)
		return ipc_send_term(archive_dir, false);

	if (destination_conninfo != NULL && cluster_name == NULL)
	{
		fprintf(stderr, "no cluster name specified in config file "
				"(required for database mode)\n");
		return -1;
	}

	/*
	 * Configuration and usage are OK. From now on all messages go into the
	 * logfile, if one is configured.
	 */
	if (logfile_path != NULL)
	{
		logfile_fp = fopen(logfile_path, "a");
		if (logfile_fp == NULL)
		{
			fprintf(stderr, "%s: %s\n", logfile_path, strerror(errno));
			return -1;
		}
	}

	/*
	 * Initialize IPC
	 */
	if ((init_rc = ipc_init(archive_dir)) < 0)
		return -1;

	/*
	 * If we are the message queue creator and if we are running in database
	 * connetion mode, add archives not yet processed to the queue.
	 */
	if (init_rc == 1)
	{
		if (archscan(optind, argc, (char **) argv) < 0)
			return -0;
	}

	/*
	 * Put all filenames given to us onto the queue
	 */
	while (optind < argc)
	{
		if (ipc_send_path((char *) (argv[optind])) < 0)
		{
			ipc_finish(true);
			return -1;
		}
		if (!opt_quiet)
			errlog(LOG_INFO, "archive %s added to queue\n", argv[optind]);

		optind++;
	}

	if (ipc_unlock() < 0)
		return -1;

	/*
	 * If we're not the message queue creator, we're done here.
	 */
	if (init_rc == 0)
		return 0;

	/*
	 * Put ourselves into the background unless -f was given
	 */
	if (!opt_foreground)
	{
		switch (pid = fork())
		{
			case -1:
				fprintf(stderr, "fork() failed: %s\n",
						strerror(errno));
				ipc_finish(true);
				return -1;

			case 0:				/* This is the child */
				signal(SIGHUP, SIG_IGN);
				close(0);
				close(2);
				dup2(1, 2);
				setsid();
				break;

			default:
				if (!opt_quiet)
					printf("logshipper daemon created - pid = %d\n",
						   pid);
				return 0;
		}
	}

	/*
	 * Arrange for program exit when running out of files if -w was given.
	 */
	if (opt_nowait)
		ipc_set_shutdown_smart();

	/*
	 * This is the daemon. Process files as they come in.
	 */

	while (true)
	{
		/*
		 * If we have a bad DB connection, we wait with further processing
		 * until we can recover it.
		 */
		if (dbconn != NULL)
		{
			if (PQstatus(dbconn) != CONNECTION_OK)
				while (PQstatus(dbconn) != CONNECTION_OK)
				{
					errlog(LOG_WARN, "bad database connection, try to recover\n");
					PQreset(dbconn);
					process_in_transaction = false;
					if (PQstatus(dbconn) == CONNECTION_OK)
						break;

					ipc_poll(true);

					if (shutdown_immed_requested)
						break;
				}
		}

		if (wait_for_resume)
			errlog(LOG_WARN, "waiting for resume\n");
		while (wait_for_resume)
		{
			ipc_poll(true);

			if (shutdown_immed_requested)
				break;
		}

		rc = ipc_recv_path(archive_path);
		if (rc == 0)
			break;

		if (rc == -2)
		{
			archscan_sort = NULL;
			errlog(LOG_INFO, "Queue is empty.  Going to rescan in %d seconds\n", rescan_interval);
			sleep(rescan_interval);
			if (archscan(optind, argc, (char **) argv) < 0)
			{
				return -1;
			}
			errlog(LOG_INFO, "Archive dir scanned\n");
			continue;
		}

		if (rc < 0)
		{
			errlog(LOG_ERROR, "ipc_recv_path returned an error:%d\n", rc);
			return -1;
		}

		current_archive_path = archive_path;
		if (process_archive(archive_path) != 0)
		{
			ProcessingCommand *errcmd;
			SlonDString cmd;

			dstring_terminate(&errlog_messages);
			dstring_init(&cmd);
			for (errcmd = error_commands; errcmd != NULL; errcmd = errcmd->next)
			{
				slon_mkquery(&cmd,
						 "inarchive='%q'; outarchive=''; errortext='%q'; %s",
							 archive_path, dstring_data(&errlog_messages),
							 errcmd->command);

				system(dstring_data(&cmd));
			}
			dstring_free(&cmd);


			/*
			 * If it left a transaction in progress, roll it back.
			 */
			if (process_in_transaction)
			{
				process_end_transaction("rollback;");
			}

			/*
			 * Stop everything if we are in nowait mode anyway
			 */
			if (opt_nowait)
			{
				if (dbconn != NULL)
					PQfinish(dbconn);
				ipc_finish(true);
				return -1;
			}

			/*
			 * Reinsert the logfile into the processing queue.
			 */
			if (ipc_send_path(archive_path) < 0)
				return -1;

			if (dbconn != NULL)
			{
				if (PQstatus(dbconn) == CONNECTION_OK)
				{
					/*
					 * this must have failed due to a parse error or something
					 * went wrong with one of the queries. Wait for resume
					 * command.
					 */
					wait_for_resume = true;
				}
			}
			else
			{
				/*
				 * If we don't even have a DB connection, this can only be a
				 * parse error or something similar. Wait for resume command.
				 */
				wait_for_resume = true;
			}
		}
		else
		{
			/*
			 * Every time we process one archive successfully, we reset the
			 * errlog message collection.
			 */
			dstring_reset(&errlog_messages);
		}
		current_archive_path = NULL;
	}

	return 0;
}


static int
process_archive(char *fname)
{
	SlonDString destfname;
	char	   *cp;
	FILE	   *fp;
	ProcessingCommand *cmd;

	errlog(LOG_INFO, "Processing archive file %s\n", fname);

	archive_count++;
	if (archive_count >= max_archives)
		ipc_set_shutdown_smart();

	fp = fopen(fname, "r");
	if (fp == NULL)
	{
		errlog(LOG_ERROR, "cannot open %s - %s\n", fname, strerror(errno));
		return -1;
	}

	if (destination_dir != NULL)
	{
		cp = strrchr(fname, '/');
		if (cp == NULL)
			cp = fname;
		else
			cp++;
		dstring_init(&destfname);
		dstring_append(&destfname, destination_dir);
		dstring_addchar(&destfname, '/');
		dstring_append(&destfname, cp);
		dstring_terminate(&destfname);

		destinationfname = dstring_data(&destfname);
	}
	else
	{
		/*
		 * This is to avoid a "possibly used uninitialized" warning.
		 */
		dstring_data(&destfname) = NULL;
	}

	for (cmd = pre_processing_commands; cmd != NULL; cmd = cmd->next)
	{
		if (process_command(cmd->command, fname, destinationfname) < 0)
		{
			fclose(fp);
			if (destinationfname != NULL)
			{
				destinationfname = NULL;
				dstring_free(&destfname);
			}
			return -1;
		}
	}

	if (destinationfname != NULL)
	{
		destinationfp = fopen(destinationfname, "w");
		if (destinationfp == NULL)
		{
			errlog(LOG_ERROR, "cannot open %s - %s\n",
				   dstring_data(&destfname), strerror(errno));
			fclose(fp);
			destinationfname = NULL;
			dstring_free(&destfname);
			return 1;
		}
	}

	scan_new_input_file(fp);
	current_file = fname;
	scan_push_string("start_archive;");
	parse_errors = 0;
	parse_errors += yyparse();
	fclose(fp);

	if (destinationfname != NULL)
	{
		fclose(destinationfp);
		destinationfp = NULL;

		if (parse_errors != 0)
			unlink(destinationfname);
	}

	if (parse_errors == 0)
	{
		for (cmd = post_processing_commands; cmd != NULL; cmd = cmd->next)
		{
			if (process_command(cmd->command, fname, destinationfname) < 0)
			{
				parse_errors++;
				break;
			}
		}
	}

	if (destinationfname != NULL)
	{
		destinationfname = NULL;
		dstring_free(&destfname);
	}

	return parse_errors;
}


static int
process_exec_sql(char *sql)
{
	/*
	 * If we have a database connection, throw the query over to there.
	 */
	if (dbconn != NULL)
	{
		PGresult   *res;

		res = PQexec(dbconn, sql);
		if (PQresultStatus(res) != PGRES_COMMAND_OK &&
			PQresultStatus(res) != PGRES_TUPLES_OK &&
			PQresultStatus(res) != PGRES_EMPTY_QUERY)
		{
			errlog(LOG_ERROR, "%s: %sQuery was: %s\n",
				   PQresStatus(PQresultStatus(res)),
				   PQresultErrorMessage(res), sql);
			PQclear(res);

			return -1;
		}

		PQclear(res);
	}

	if (destinationfp != NULL)
	{
		if (fputs(sql, destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			return -1;
		}
		if (fputc('\n', destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			return -1;
		}
	}

	return 0;
}


int
process_check_at_counter(char *at_counter)
{
	char		buf1[64];
	char		buf2[64];
	size_t		i;

	if (destination_conninfo == NULL)
		return 0;

	if (strlen(at_counter) > 20)
	{
		errlog(LOG_ERROR, "at_counter %s too long in process_check_at_counter\n",
			   at_counter);
		return -1;
	}

	if (get_current_at_counter() < 0)
		return -1;

	for (i = 0; i < (20 - strlen(current_at_counter)); i++)
		buf1[i] = '0';
	strcpy(&buf1[i], current_at_counter);
	for (i = 0; i < (20 - strlen(at_counter)); i++)
		buf2[i] = '0';
	strcpy(&buf2[i], at_counter);

	if (strcmp(buf2, buf1) <= 0)
	{
		errlog(LOG_WARN, "skip archive with counter %s - already applied\n",
			   at_counter);
		if (process_in_transaction)
			process_end_transaction("rollback;");
		return 1;
	}

	return 0;
}


int
process_simple_sql(char *sql)
{
	return process_exec_sql(sql);
}


int
process_insert(InsertStmt *stmt)
{
	SlonDString ds;
	char	   *glue;
	AttElem    *elem;
	int			rc;
	char	   *namespace;
	char	   *tablename;

	if (lookup_rename(stmt->namespace, stmt->tablename,
					  &namespace, &tablename) == 0)
		return 0;

	dstring_init(&ds);
	slon_mkquery(&ds, "insert into %s.%s ", namespace, tablename);
	glue = "(";
	for (elem = stmt->attributes->list_head; elem != NULL; elem = elem->next)
	{
		slon_appendquery(&ds, "%s%s", glue, elem->attname);
		glue = ", ";
	}
	slon_appendquery(&ds, ") values ");
	glue = "(";
	for (elem = stmt->attributes->list_head; elem != NULL; elem = elem->next)
	{
		if (elem->attvalue == NULL)
			slon_appendquery(&ds, "%sNULL", glue);
		else
			slon_appendquery(&ds, "%s'%s'", glue, elem->attvalue);
		glue = ", ";
	}
	dstring_addchar(&ds, ')');
	dstring_addchar(&ds, ';');
	dstring_terminate(&ds);

	rc = process_exec_sql(dstring_data(&ds));
	dstring_free(&ds);

	return rc;
}


int
process_update(UpdateStmt *stmt)
{
	SlonDString ds;
	char	   *glue;
	AttElem    *elem;
	int			rc;
	char	   *namespace;
	char	   *tablename;

	if (lookup_rename(stmt->namespace, stmt->tablename,
					  &namespace, &tablename) == 0)
		return 0;

	dstring_init(&ds);
	slon_mkquery(&ds, "update only %s.%s ", namespace, tablename);
	glue = "set";
	for (elem = stmt->changes->list_head; elem != NULL; elem = elem->next)
	{
		if (elem->attvalue == NULL)
			slon_appendquery(&ds, "%s %s=NULL", glue,
							 elem->attname);
		else
			slon_appendquery(&ds, "%s %s='%s'", glue,
							 elem->attname, elem->attvalue);
		glue = ",";
	}
	glue = " where";
	for (elem = stmt->qualification->list_head; elem != NULL; elem = elem->next)
	{
		if (elem->attvalue == NULL)
			slon_appendquery(&ds, "%s %s IS NULL", glue,
							 elem->attname);
		else
			slon_appendquery(&ds, "%s %s='%s'", glue,
							 elem->attname, elem->attvalue);
		glue = " and";
	}
	dstring_addchar(&ds, ';');
	dstring_terminate(&ds);

	rc = process_exec_sql(dstring_data(&ds));
	dstring_free(&ds);

	return rc;
}


int
process_delete(DeleteStmt *stmt)
{
	SlonDString ds;
	char	   *glue;
	AttElem    *elem;
	int			rc;
	char	   *namespace;
	char	   *tablename;

	if (lookup_rename(stmt->namespace, stmt->tablename,
					  &namespace, &tablename) == 0)
		return 0;

	dstring_init(&ds);
	slon_mkquery(&ds, "delete from %s%s.%s",
				 (stmt->only) ? "only " : "", namespace, tablename);
	if (stmt->qualification != NULL)
	{
		glue = " where";
		for (elem = stmt->qualification->list_head; elem != NULL; elem = elem->next)
		{
			if (elem->attvalue == NULL)
				slon_appendquery(&ds, "%s %s IS NULL", glue,
								 elem->attname);
			else
				slon_appendquery(&ds, "%s %s='%s'", glue,
								 elem->attname, elem->attvalue);
			glue = " and";
		}
	}
	dstring_addchar(&ds, ';');
	dstring_terminate(&ds);

	rc = process_exec_sql(dstring_data(&ds));
	dstring_free(&ds);

	return rc;
}


int
process_truncate(TruncateStmt *stmt)
{
	SlonDString ds;
	int			rc;
	char	   *namespace;
	char	   *tablename;

	if (lookup_rename(stmt->namespace, stmt->tablename,
					  &namespace, &tablename) == 0)
		return 0;

	dstring_init(&ds);
	slon_mkquery(&ds, "truncate only %s.%s cascade;",
				 namespace, tablename);
	dstring_terminate(&ds);

	rc = process_exec_sql(dstring_data(&ds));
	dstring_free(&ds);

	return rc;
}

int
process_copy(CopyStmt *stmt)
{
	SlonDString ds;
	char	   *glue;
	AttElem    *elem;
	PGresult   *res;
	char	   *namespace;
	char	   *tablename;

	if (lookup_rename(stmt->namespace, stmt->tablename,
					  &namespace, &tablename) == 0)
	{
		suppress_copy = true;
		scan_copy_start();
		return 0;
	}
	suppress_copy = false;

	dstring_init(&ds);
	slon_mkquery(&ds, "copy %s.%s ", namespace, tablename);
	glue = "(";
	for (elem = stmt->attributes->list_head; elem != NULL; elem = elem->next)
	{
		slon_appendquery(&ds, "%s%s", glue, elem->attname);
		glue = ", ";
	}
	slon_appendquery(&ds, ") from %s;", stmt->from);

	if (dbconn != NULL)
	{
		res = PQexec(dbconn, dstring_data(&ds));
		if (PQresultStatus(res) != PGRES_COPY_IN)
		{
			errlog(LOG_ERROR, "%s: %sQuery was: %s\n",
				   PQresStatus(PQresultStatus(res)),
				   PQresultErrorMessage(res), dstring_data(&ds));
			PQclear(res);
			dstring_free(&ds);
			return -1;
		}
	}

	if (destinationfp != NULL)
	{
		if (fputs(dstring_data(&ds), destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			dstring_free(&ds);
			return -1;
		}
		if (fputc('\n', destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			return -1;
		}
	}

	dstring_free(&ds);

	scan_copy_start();

	return 0;
}


int
process_copydata(char *line)
{
	PGresult   *res;

	if (suppress_copy)
		return 0;

	if (dbconn != NULL)
	{
#ifdef HAVE_PQPUTCOPYDATA
		if (PQputCopyData(dbconn, line, strlen(line)) != 1)
		{
			errlog(LOG_ERROR, "%s", PQerrorMessage(dbconn));
			PQputCopyEnd(dbconn, "Offline copy_set failed");
			res = PQgetResult(dbconn);
			if (res != NULL)
				PQclear(res);
			return -1;
		}
#else
		if (PQputline(dbconn, line) != 0)
		{
			errlog(LOG_ERROR, "%s", PQerrorMessage(dbconn));
			PQendcopy(dbconn);
			return -1;
		}
#endif
	}

	if (destinationfp != NULL)
	{
		if (fputs(line, destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			return -1;
		}
	}

	return 0;
}


int
process_copyend(void)
{
	if (suppress_copy)
		return 0;

	if (dbconn != NULL)
	{
#ifdef HAVE_PQPUTCOPYDATA
		PGresult   *res;

		if (PQputCopyEnd(dbconn, NULL) != 1)
		{
			errlog(LOG_ERROR, "%s", PQerrorMessage(dbconn));
			return -1;
		}
		res = PQgetResult(dbconn);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			errlog(LOG_ERROR, "%s: %s",
				   PQresStatus(PQresultStatus(res)),
				   PQresultErrorMessage(res));
			PQclear(res);
			return -1;
		}
		PQclear(res);
#else
		if (PQputline(dbconn, "\\.\n") != 0)
		{
			errlog(LOG_ERROR, "%s", PQerrorMessage(dbconn));
			PQendcopy(dbconn);
			return -1;
		}
		if (PQendcopy(dbconn) != 0)
		{
			errlog(LOG_ERROR, "%s", PQerrorMessage(dbconn));
			return -1;
		}
#endif
	}

	if (destinationfp != NULL)
	{
		if (fputs("\\.\n", destinationfp) == EOF)
		{
			errlog(LOG_ERROR, "%s: %s\n", destinationfname, strerror(errno));
			return -1;
		}
	}

	return 0;
}


int
process_start_transaction(char *sql)
{
	if (process_in_transaction)
	{
		errlog(LOG_ERROR, "already inside a transaction\n");
		return -1;
	}
	process_in_transaction = true;

	return process_exec_sql(sql);
}


int
process_end_transaction(char *sql)
{
	if (!process_in_transaction)
	{
		errlog(LOG_ERROR, "not inside a transaction\n");
		return -1;
	}
	process_in_transaction = false;

	return process_exec_sql(sql);
}


static int
process_command(char *command, char *inarchive, char *outarchive)
{
	int			status;
	int			errorignore = 0;
	SlonDString cmd;

	if (*command == '@')
	{
		errorignore = 1;
		command++;
	}

	dstring_init(&cmd);
	slon_mkquery(&cmd, "inarchive='%q'; outarchive='%q'; %s",
				 inarchive, (outarchive == NULL) ? "" : outarchive, command);
	status = system(dstring_data(&cmd));
	if (status != 0)
	{
		if (errorignore)
			errlog(LOG_WARN, "exit code %d for command: %s\n", status, command);
		else
			errlog(LOG_ERROR, "exit code %d for command: %s\n", status, command);
	}
	dstring_free(&cmd);

	if (errorignore)
		return 0;

	return (status == 0) ? 0 : -1;
}


static void
notice_processor(void *arg, const char *msg)
{
	errlog(LOG_INFO, "%s", msg);
}


void
errlog(log_level level, char *fmt,...)
{
	static char errbuf[256 * 1024];
	char	   *pos;
	va_list		ap;
	time_t		tnow;
	struct tm	now;
	char	   *clevel[] = {
		"DEBUG", "INFO", "WARN", "ERROR"
	};

	if (logfile_switch_requested && logfile_path != NULL)
	{
		if (logfile_fp != stdout)
			fclose(logfile_fp);
		logfile_fp = fopen(logfile_path, "a");
		if (logfile_fp == NULL)
			logfile_fp = stdout;

		logfile_switch_requested = false;
	}

	time(&tnow);
	localtime_r(&tnow, &now);

	snprintf(errbuf, sizeof(errbuf), "%-5.5s %04d-%02d-%02d %02d:%02d:%02d > ",
			 clevel[level], now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
			 now.tm_hour, now.tm_min, now.tm_sec);
	pos = errbuf + strlen(errbuf);
	va_start(ap, fmt);
	vsnprintf(pos, sizeof(errbuf) - (pos - errbuf), fmt, ap);
	va_end(ap);

	if (!opt_quiet || level > LOG_INFO)
	{
		fputs(errbuf, logfile_fp);
		fflush(logfile_fp);
	}
	dstring_append(&errlog_messages, errbuf);
}


static int
idents_are_distinct(char *id1, char *id2)
{
	if (id1 == NULL && id2 == NULL)
		return 0;
	if (id1 == NULL || id2 == NULL)
		return 1;

	if (*id1 == '"')
	{
		if (strcmp(id1, id2) == 0)
			return 0;
		return 1;
	}
	if (strcasecmp(id1, id2) == 0)
		return 0;
	return 1;
}


void
config_add_rename(RenameObject * entry)
{
	RenameObject **ep;

	for (ep = &rename_list; *ep != NULL; ep = &((*ep)->next))
	{
		if (idents_are_distinct((*ep)->old_namespace, entry->old_namespace))
			continue;
		if (idents_are_distinct((*ep)->new_namespace, entry->new_namespace))
			continue;
		if (idents_are_distinct((*ep)->old_name, entry->old_name))
			continue;
		if (idents_are_distinct((*ep)->new_name, entry->new_name))
			continue;
	}

	*ep = entry;
}


int
lookup_rename(char *namespace, char *name,
			  char **use_namespace, char **use_name)
{
	RenameObject *entry;

	/*
	 * first we look for a table specific entry
	 */
	for (entry = rename_list; entry != NULL; entry = entry->next)
	{
		if (entry->old_name == NULL)
			continue;

		if (idents_are_distinct(entry->old_namespace, namespace) ||
			idents_are_distinct(entry->old_name, name))
			continue;

		/*
		 * This is a match.
		 */
		*use_namespace = entry->new_namespace;
		*use_name = entry->new_name;
		if (*use_namespace == NULL)
			return 0;
		else
			return 1;
	}

	/*
	 * second we look for a whole namespace
	 */
	for (entry = rename_list; entry != NULL; entry = entry->next)
	{
		if (entry->old_name != NULL)
			continue;

		if (idents_are_distinct(entry->old_namespace, namespace))
			continue;

		/*
		 * This is a match.
		 */
		*use_namespace = entry->new_namespace;
		*use_name = name;
		if (*use_namespace == NULL)
			return 0;
		else
			return 1;
	}

	/*
	 * No match found - just keep the item as it is
	 */
	*use_namespace = namespace;
	*use_name = name;
	return 1;
}


/* ----------
 * usage -
 * ----------
 */
static void
usage(void)
{
	fprintf(stderr,
			"usage: slony_logshipper [options] config_file [archive_file]\n"
			"\n"
			"    options:\n"
			"      -h    display this help text and exit\n"
			"      -v    display program version and exit\n"
			"      -q    quiet mode\n"
			"      -l    cause running daemon to reopen its logfile\n"
			"      -r    cause running daemon to resume after error\n"
			"      -t    cause running daemon to enter smart shutdown mode\n"
		"      -T    cause running daemon to enter immediate shutdown mode\n"
			"      -c    destroy existing semaphore set and message queue\n"
			"            (use with caution)\n"
			"      -f    stay in foreground (don't daemonize)\n"
			"      -w    enter smart shutdown mode immediately\n"
			"      -s    indicate (integer value) rescan interval\n"
			"\n");
	exit(1);
}


static int
get_current_at_counter(void)
{
	SlonDString ds;
	SlonDString query;
	PGresult   *res;
	char	   *s;

	if (namespace == NULL)
	{
		namespace = malloc(strlen(cluster_name) + 4);
		if (namespace == NULL)
		{
			errlog(LOG_ERROR, "out of memory\n");
			ipc_finish(true);
			return -1;
		}
		sprintf(namespace, "\"_%s\"", cluster_name);
	}

	if (dbconn == NULL)
	{
		dbconn = PQconnectdb(destination_conninfo);
		if (dbconn == NULL)
		{
			errlog(LOG_ERROR, "PQconnectdb() failed\n");
			ipc_finish(true);
			return -1;
		}
		PQsetNoticeProcessor(dbconn, notice_processor, NULL);
	}

	dstring_init(&query);
	slon_mkquery(&query, "select 1 from pg_catalog.pg_settings where name= 'application_name'; ");
	res = PQexec(dbconn, dstring_data(&query));
	dstring_free(&query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return -1;

	if (PQntuples(res) == 0)
	{
		PQclear(res);
	}
	else
	{
		PQclear(res);
		dstring_init(&query);
		slon_mkquery(&query, "SET application_name TO 'slony_logshipper'; ");
		res = PQexec(dbconn, dstring_data(&query));
		dstring_free(&query);
		PQclear(res);
	}

	dstring_init(&ds);
	slon_mkquery(&ds, "select at_counter from %s.sl_archive_tracking;",
				 namespace);
	res = PQexec(dbconn, dstring_data(&ds));
	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res)==0 )
	{
		errlog(LOG_ERROR, "cannot retrieve archive tracking status: %s\n",
			   PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(dbconn);
		ipc_finish(true);
		dstring_free(&ds);
		return -1;
	}

	s = PQgetvalue(res, 0, 0);
	if (strlen(s) > 20)
	{
		errlog(LOG_ERROR,
			 "value of sl_archive_tracking.at_counter is suspicious: '%s'\n",
			   s);
		PQclear(res);
		PQfinish(dbconn);
		ipc_finish(true);
		dstring_free(&ds);
		return -1;
	}

	strcpy(current_at_counter, s);
	PQclear(res);
	dstring_free(&ds);

	return 0;
}


static int
archscan(int optind, int argc, char **argv)
{
	DIR		   *dirp;
	struct dirent *dp;
	char		counter_done_buf[64];

	if (destination_conninfo != NULL)
	{
		if (get_current_at_counter() < 0)
			return -1;

		counter_done_buf[0] = '\0';
		while (strlen(counter_done_buf) + strlen(current_at_counter) < 20)
			strcat(counter_done_buf, "0");
		strcat(counter_done_buf, current_at_counter);
		strcat(counter_done_buf, ".sql");
	}
	else
	{
		counter_done_buf[0] = '\0';
	}

	/*
	 * Scan the archive directory for files that have not been processed yet
	 * according to the archive tracking.
	 */
	if ((dirp = opendir(archive_dir)) == NULL)
	{
		errlog(LOG_ERROR, "cannot open directory %s: %s\n", archive_dir,
			   strerror(errno));
		PQfinish(dbconn);
		ipc_finish(true);
		return -1;
	}
	while ((dp = readdir(dirp)) != NULL)
	{
		if (strlen(dp->d_name) > 24 &&
			strcmp(dp->d_name + strlen(dp->d_name) - 24,
				   counter_done_buf) <= 0)
		{
			continue;
		}

		if (strlen(dp->d_name) > 15 &&
			strncmp(dp->d_name, "slony1_log_", 11) == 0 &&
			strcmp(dp->d_name + strlen(dp->d_name) - 4, ".sql") == 0)
		{
			if (archscan_sort_in(&archscan_sort, dp->d_name, optind,
								 argc, argv) < 0)
			{
				PQfinish(dbconn);
				ipc_finish(true);
				return -1;
			}
		}
	}
	closedir(dirp);

	if (archscan_sort_out(archscan_sort) < 0)
	{
		PQfinish(dbconn);
		ipc_finish(true);
		return -1;
	}

	return 0;
}


static int
archscan_sort_in(archscan_entry ** entp, char *fname, int optind,
				 int argc, char **argv)
{
	archscan_entry *ent;
	char	   *cp1;

	/*
	 * Ignore files that compare higher or equal to any of our command line
	 * arguments.
	 */
	while (optind < argc)
	{
		if ((cp1 = strrchr(argv[optind], '/')) == NULL)
			cp1 = argv[optind];
		else
			cp1++;

		if (strcmp(fname, cp1) >= 0)
			return 0;

		optind++;
	}

	if (*entp == NULL)
	{
		ent = (archscan_entry *) malloc(sizeof(archscan_entry));
		if (ent == NULL)
		{
			errlog(LOG_ERROR, "out of memory in archscan_sort_in()\n");
			return -1;
		}
		if ((ent->fname = strdup(fname)) == NULL)
		{
			errlog(LOG_ERROR, "out of memory in archscan_sort_in()\n");
			return -1;
		}
		ent->left = ent->right = NULL;

		*entp = ent;
		return 0;
	}

	if (strcmp(fname, (*entp)->fname) < 0)
		return archscan_sort_in(&((*entp)->left), fname, optind, argc, argv);
	else
		return archscan_sort_in(&((*entp)->right), fname, optind, argc, argv);
}


static int
archscan_sort_out(archscan_entry * ent)
{
	char	   *buf;

	if (ent == NULL)
		return 0;

	if (archscan_sort_out(ent->left) < 0)
		return -1;

	buf = (char *) malloc(strlen(archive_dir) + strlen(ent->fname) + 2);
	if (buf == NULL)
	{
		errlog(LOG_ERROR, "out of memory in archscan_sort_out()\n");
		return -1;
	}
	strcpy(buf, archive_dir);
	strcat(buf, "/");
	strcat(buf, ent->fname);
	if (ipc_send_path(buf) < 0)
	{
		free(buf);
		return -1;
	}
	free(buf);

	if (archscan_sort_out(ent->right) < 0)
		return -1;

	free(ent->fname);
	free(ent);

	return 0;
}


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
