/*-------------------------------------------------------------------------
 * slonik.c
 *
 *	A configuration and admin script utility for Slony-I.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: slonik.c,v 1.5 2004-03-13 03:20:12 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "postgres.h"
#include "libpq-fe.h"

#include "slonik.h"
#include "config.h"


/*
 * Global data
 */
SlonikScript		   *parser_script = NULL;
int						parser_errors = 0;
int						current_try_level;


/*
 * Local functions
 */
static void				usage(void);
static SlonikAdmInfo   *get_adminfo(SlonikStmt *stmt, int no_id);
static SlonikAdmInfo   *get_active_adminfo(SlonikStmt *stmt, int no_id);
static SlonikAdmInfo   *get_checked_adminfo(SlonikStmt *stmt, int no_id);


static int				script_check(SlonikScript *script);
static int				script_check_adminfo(SlonikStmt *hdr, int no_id);
static int				script_check_stmts(SlonikScript *script, 
									SlonikStmt *stmt);
static int				script_exec(SlonikScript *script);
static int				script_exec_stmts(SlonikScript *script, 
									SlonikStmt *stmt);
static void				script_commit_all(SlonikStmt *stmt,
									SlonikScript *script);
static void				script_rollback_all(SlonikStmt *stmt,
									SlonikScript *script);
static void				script_disconnect_all(SlonikScript *script);


/* ----------
 * main
 * ----------
 */
int
main(int argc, const char *argv[])
{
	extern int		optind;
	extern char	   *optarg;
	int				opt;

	while ((opt = getopt(argc, (char **)argv, "h")) != EOF)
	{
		switch(opt)
		{
			case 'h':	parser_errors++;
						break;

			default:	printf("unknown option '%c'\n", opt);
						parser_errors++;
						break;
		}
	}

	if (parser_errors)
		usage();

	if (optind < argc)
	{
		while (optind < argc)
		{
			yyin = fopen(argv[optind], "r");
			current_file = (char *)argv[optind++];
			yylineno = 1;
			yyparse();
			fclose(yyin);

			if (parser_errors != 0)
				return -1;

			if (script_check(parser_script) != 0)
				return -1;
			if (script_exec(parser_script) != 0)
				return -1;
		}
	}
	else
	{
		yyin = stdin;
		yyparse();

		if (parser_errors != 0)
			return -1;

		if (script_check(parser_script) != 0)
			return -1;
		if (script_exec(parser_script) != 0)
			return -1;
	}

	return 0;
}


/* ----------
 * usage -
 * ----------
 */
static void
usage(void)
{
	fprintf(stderr, "usage: slonik [fname [...]]\n");
	exit (1);
}


/* ----------
 * script_check -
 * ----------
 */
static int
script_check(SlonikScript *script)
{
	SlonikAdmInfo	   *adminfo;
	SlonikAdmInfo	   *adminfo2;
	int					errors = 0;


	for (adminfo = script->adminfo_list; adminfo; adminfo = adminfo->next)
	{
		adminfo->script = script;

		for (adminfo2 = adminfo->next; adminfo2; adminfo2 = adminfo2->next)
		{
			if (adminfo2->no_id == adminfo->no_id)
			{
				printf("%s:%d: Error: duplicate admin conninfo for node %d\n",
						adminfo2->stmt_filename, adminfo2->stmt_lno,
						adminfo2->no_id);
				printf("%s:%d: location of the previous definition\n",
						adminfo->stmt_filename, adminfo->stmt_lno);
				errors++;
			}
		}
	}

	if (script_check_stmts(script, script->script_stmts) != 0)
		errors++;

	return -errors;
}


/* ----------
 * script_check_adminfo -
 * ----------
 */
static int
script_check_adminfo(SlonikStmt *hdr, int no_id)
{
	SlonikAdmInfo	   *adminfo;

	for (adminfo = hdr->script->adminfo_list; adminfo; adminfo = adminfo->next)
	{
		if (adminfo->no_id == no_id)
			return 0;
	}

	printf("%s:%d: Error: "
			"No admin conninfo provided for node %d\n",
			hdr->stmt_filename, hdr->stmt_lno, no_id);
	return -1;
}


/* ----------
 * script_check_stmts -
 * ----------
 */
static int
script_check_stmts(SlonikScript *script, SlonikStmt *hdr)
{
	int		errors = 0;

	while (hdr)
	{
		hdr->script = script;

		switch (hdr->stmt_type)
		{
			case STMT_TRY:
				{
					SlonikStmt_try *stmt =
							(SlonikStmt_try *)hdr;

					if (script_check_stmts(script, stmt->try_block) < 0)
						errors++;
					if (script_check_stmts(script, stmt->error_block) < 0)
						errors++;
					if (script_check_stmts(script, stmt->success_block) < 0)
						errors++;
				}
				break;

			case STMT_ECHO:
				break;

			case STMT_EXIT:
				break;

			case STMT_RESTART_NODE:
				{
					SlonikStmt_restart_node *stmt =
							(SlonikStmt_restart_node *)hdr;

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_ERROR:
				break;

			case STMT_INIT_CLUSTER:
				{
					SlonikStmt_init_cluster *stmt =
							(SlonikStmt_init_cluster *)hdr;

					/*
					 * Initial primary cluster node must be no_id 1
					 */
					if (stmt->no_id != 1)
					{
						printf("%s:%d: Error: "
								"Primary cluster node must be no_id 1\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					/*
					 * Check that we have conninfo to that
					 */
					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_STORE_NODE:
				{
					SlonikStmt_store_node *stmt =
							(SlonikStmt_store_node *)hdr;

					if (stmt->ev_origin < 0)
					{
						stmt->ev_origin = 1;
					}
					if (stmt->no_id < 2)
					{
						printf("%s:%d: Error: "
								"no_id must be >= 2\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->no_id == stmt->ev_origin)
					{
						printf("%s:%d: Error: "
								"ev_origin for store_node cannot "
								"be the new node\n",
								hdr->stmt_filename, hdr->stmt_lno);
					}

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;
				}
				break;

			case STMT_STORE_PATH:
				{
					SlonikStmt_store_path *stmt =
							(SlonikStmt_store_path *)hdr;

					if (script_check_adminfo(hdr, stmt->pa_client) < 0)
						errors++;

				}
				break;

			case STMT_STORE_LISTEN:
				{
					SlonikStmt_store_listen *stmt =
							(SlonikStmt_store_listen *)hdr;

					if (stmt->li_provider < 0)
					{
						stmt->li_provider = stmt->li_origin;
					}
					if (script_check_adminfo(hdr, stmt->li_receiver) < 0)
						errors++;
				}
				break;

			case STMT_DROP_LISTEN:
				{
					SlonikStmt_drop_listen *stmt =
							(SlonikStmt_drop_listen *)hdr;

					if (stmt->li_provider < 0)
					{
						stmt->li_provider = stmt->li_origin;
					}
					if (script_check_adminfo(hdr, stmt->li_receiver) < 0)
						errors++;
				}
				break;

			case STMT_CREATE_SET:
				{
					SlonikStmt_create_set *stmt =
							(SlonikStmt_create_set *)hdr;

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_SET_ADD_TABLE:
				{
					SlonikStmt_set_add_table *stmt =
							(SlonikStmt_set_add_table *)hdr;

					/*
					 * Check that we have the set_id and set_origin
					 * and that we can reach the origin.
					 */
					if (stmt->set_id < 0)
					{
						printf("%s:%d: Error: "
								"set id must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->set_origin < 0)
					{
						printf("%s:%d: Error: "
								"origin must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					else
					{
						if (script_check_adminfo(hdr, stmt->set_origin) < 0)
							errors++;
					}

					/*
					 * Check that we have the table id, name
					 * and what to use for the key.
					 */
					if (stmt->tab_id < 0)
					{
						printf("%s:%d: Error: "
								"table id must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->tab_fqname == NULL)
					{
						printf("%s:%d: Error: "
								"table FQ-name must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->use_serial && stmt->use_key != NULL)
					{
						printf("%s:%d: Error: "
								"unique key name or SERIAL are "
								"mutually exclusive\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (stmt->tab_comment == NULL)
						stmt->tab_comment = strdup(stmt->tab_fqname);
				}
				break;

			case STMT_TABLE_ADD_KEY:
				{
					SlonikStmt_table_add_key *stmt =
							(SlonikStmt_table_add_key *)hdr;

					/*
					 * Check that we have the node id and that
					 * we can reach it.
					 */
					if (stmt->no_id < 0)
					{
						printf("%s:%d: Error: "
								"node id must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					else
					{
						if (script_check_adminfo(hdr, stmt->no_id) < 0)
							errors++;
					}

					/*
					 * Check that we have the table name
					 */
					if (stmt->tab_fqname == NULL)
					{
						printf("%s:%d: Error: "
								"table FQ-name must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
				}
				break;

			case STMT_SUBSCRIBE_SET:
				{
					SlonikStmt_subscribe_set *stmt =
							(SlonikStmt_subscribe_set *)hdr;

					if (stmt->sub_setid < 0)
					{
						printf("%s:%d: Error: "
								"set id must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->sub_provider < 0)
					{
						printf("%s:%d: Error: "
								"provider must be specified\n",
								hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->sub_receiver) < 0)
						errors++;
				}
				break;

		}

		hdr = hdr->next;
	}

	return -errors;
}


/* ----------
 * script_exec -
 * ----------
 */
static int
script_exec(SlonikScript *script)
{
	if (script_exec_stmts(script, script->script_stmts) < 0)
	{
		/*
		 * Disconnect does an implicit rollback
		 */
		script_disconnect_all(script);
		return -1;
	}

	script_disconnect_all(script);
	return 0;
}


/* ----------
 * script_exec_stmts -
 * ----------
 */
static int
script_exec_stmts(SlonikScript *script, SlonikStmt *hdr)
{
	int		errors = 0;

	while (hdr && errors == 0)
	{
		hdr->script = script;

		switch (hdr->stmt_type)
		{
			case STMT_TRY:
				{
					SlonikStmt_try *stmt =
							(SlonikStmt_try *)hdr;
					int		rc;

					current_try_level++;
					rc = script_exec_stmts(script, stmt->try_block);
					current_try_level--;

					if (rc < 0)
					{
						script_rollback_all(hdr, script);
						if (stmt->error_block)
						{
							/*
							 * Successfull execution of ON ERROR case block
							 * suppressed setting the overall script on
							 * error.
							 */
							if (script_exec_stmts(script, stmt->error_block) < 0)
								errors++;
						}
						else
						{
							/*
							 * The try-block has no ON ERROR action,
							 * abort script execution.
							 */
							errors++;
						}
					}
					else
					{
						script_commit_all(hdr, script);
						/*
						 * If the try block has an ON SUCCESS block, 
						 * execute that.
						 */
						if (stmt->success_block)
							if (script_exec_stmts(script, stmt->success_block) < 0)
								errors++;
					}
				}
				break;

			case STMT_ECHO:
				{
					SlonikStmt_echo *stmt =
							(SlonikStmt_echo *)hdr;

					printf("%s:%d: %s\n",
							stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
							stmt->str);
				}
				break;

			case STMT_EXIT:
				{
					SlonikStmt_exit *stmt =
							(SlonikStmt_exit *)hdr;

					exit (stmt->exitcode);
				}
				break;

			case STMT_RESTART_NODE:
				{
					SlonikStmt_restart_node *stmt =
							(SlonikStmt_restart_node *)hdr;

					if (slonik_restart_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_ERROR:
				printf("slonik_exec_stmt: FATAL error: STMT_ERROR node "
						"made it into execution!\n");
				exit(-1);
				break;

			case STMT_INIT_CLUSTER:
				{
					SlonikStmt_init_cluster *stmt =
							(SlonikStmt_init_cluster *)hdr;

					if (slonik_init_cluster(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_NODE:
				{
					SlonikStmt_store_node *stmt =
							(SlonikStmt_store_node *)hdr;

					if (slonik_store_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_PATH:
				{
					SlonikStmt_store_path *stmt =
							(SlonikStmt_store_path *)hdr;

					if (slonik_store_path(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_LISTEN:
				{
					SlonikStmt_store_listen *stmt =
							(SlonikStmt_store_listen *)hdr;

					if (slonik_store_listen(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DROP_LISTEN:
				{
					SlonikStmt_drop_listen *stmt =
							(SlonikStmt_drop_listen *)hdr;

					if (slonik_drop_listen(stmt) < 0)
						errors++;
				}
				break;

			case STMT_CREATE_SET:
				{
					SlonikStmt_create_set *stmt =
							(SlonikStmt_create_set *)hdr;

					if (slonik_create_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_ADD_TABLE:
				{
					SlonikStmt_set_add_table *stmt =
							(SlonikStmt_set_add_table *)hdr;

					if (slonik_set_add_table(stmt) < 0)
						errors++;
				}
				break;

			case STMT_TABLE_ADD_KEY:
				{
					SlonikStmt_table_add_key *stmt =
							(SlonikStmt_table_add_key *)hdr;

					if (slonik_table_add_key(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SUBSCRIBE_SET:
				{
					SlonikStmt_subscribe_set *stmt =
							(SlonikStmt_subscribe_set *)hdr;

					if (slonik_subscribe_set(stmt) < 0)
						errors++;
				}
				break;

		}

		if (current_try_level == 0)
		{
			if (errors == 0)
			{
				script_commit_all(hdr, script);
			}
			else
			{
				script_rollback_all(hdr, script);
			}
		}

		hdr = hdr->next;
	}

	return -errors;
}


static void
script_commit_all(SlonikStmt *stmt, SlonikScript *script)
{
	SlonikAdmInfo  *adminfo;
	int				error = 0;

	for (adminfo = script->adminfo_list; 
			adminfo; adminfo = adminfo->next)
	{
		if (adminfo->dbconn != NULL && adminfo->have_xact)
		{
			if (db_commit_xact(stmt, adminfo) < 0)
				error = 1;
		}
		else
		{
			db_rollback_xact(stmt, adminfo);
		}
	}
}


static void
script_rollback_all(SlonikStmt *stmt, SlonikScript *script)
{
	SlonikAdmInfo  *adminfo;

	for (adminfo = script->adminfo_list; 
			adminfo; adminfo = adminfo->next)
	{
		if (adminfo->dbconn != NULL && adminfo->have_xact)
			db_rollback_xact(stmt, adminfo);
	}
}


static void
script_disconnect_all(SlonikScript *script)
{
	SlonikAdmInfo  *adminfo;
	SlonikStmt		eofstmt;

	eofstmt.stmt_type = STMT_ERROR;
	eofstmt.stmt_filename = script->filename;
	eofstmt.stmt_lno = -1;
	eofstmt.script = script;
	eofstmt.next = NULL;

	for (adminfo = script->adminfo_list; 
			adminfo; adminfo = adminfo->next)
	{
		db_disconnect(&eofstmt, adminfo);
	}
}


static SlonikAdmInfo *
get_adminfo(SlonikStmt *stmt, int no_id)
{
	SlonikAdmInfo  *adminfo;

	for (adminfo = stmt->script->adminfo_list; 
			adminfo; adminfo = adminfo->next)
	{
		if (adminfo->no_id == no_id)
			return adminfo;
	}

	return NULL;
}


static SlonikAdmInfo *
get_active_adminfo(SlonikStmt *stmt, int no_id)
{
	SlonikAdmInfo  *adminfo;

	if ((adminfo = get_adminfo(stmt, no_id)) == NULL)
	{
		printf("%s:%d: ERROR: no admin conninfo for node %d\n",
				stmt->stmt_filename, stmt->stmt_lno, no_id);
		return NULL;
	}

	if (adminfo->dbconn == NULL)
	{
		if (db_connect(stmt, adminfo) < 0)
			return NULL;

		if (db_get_version(stmt, adminfo, 
				&(adminfo->version_major), &(adminfo->version_minor)) < 0)
		{
			PQfinish(adminfo->dbconn);
			adminfo->dbconn = NULL;
			return NULL;
		}
	}

	return adminfo;
}


static SlonikAdmInfo *
get_checked_adminfo(SlonikStmt *stmt, int no_id)
{
	SlonikAdmInfo  *adminfo;
	int				db_no_id;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return NULL;

	if (!adminfo->nodeid_checked)
	{
		if ((db_no_id = db_get_nodeid(stmt, adminfo)) != no_id)
		{
			printf("%s:%d: database specified in %s:%d reports no_id %d\n",
					stmt->stmt_filename, stmt->stmt_lno,
					adminfo->stmt_filename, adminfo->stmt_lno,
					db_no_id);
			return NULL;
		}
		adminfo->nodeid_checked = true;
	}

	return adminfo;
}


static int
load_sql_script(SlonikStmt *stmt, SlonikAdmInfo *adminfo, char *fname, ...)
{
	PGresult	   *res;
	SlonDString		query;
	va_list			ap;
	int				rc;
	char			fnamebuf[1024];
	char			buf[4096];
	char			rex1[256];
	char			rex2[256];
	int				sed_pipe[2];

	if (db_begin_xact(stmt, adminfo) < 0)
		return -1;

	sprintf(rex1, "s/@CLUSTERNAME@/%s/g", stmt->script->clustername);
	sprintf(rex2, "s/@NAMESPACE@/\\\"_%s\\\"/g", stmt->script->clustername);
	va_start(ap, fname);
	vsnprintf(fnamebuf, sizeof(fnamebuf), fname, ap);
	va_end(ap);

	if (pipe(sed_pipe) < 0)
	{
		perror("pipe()");
		return -1;
	}

	switch (fork())
	{
		case -1:
			perror("fork()");
			return -1;

		case 0:
			dup2(sed_pipe[1], 1);
			close(sed_pipe[0]);
			close(sed_pipe[1]);

			execlp("sed", "sed", "-e", rex1, "-e", rex2, fnamebuf, NULL);
			perror("execlp()");
			exit(-1);

		default:
			close(sed_pipe[1]);
			break;
	}

	dstring_init(&query);
	while((rc = read(sed_pipe[0], buf, sizeof(buf))) > 0)
		dstring_nappend(&query, buf, rc);

	if (rc < 0)
	{
		perror("read()");
		close(sed_pipe[0]);
		dstring_free(&query);
		return -1;
	}
	close(sed_pipe[0]);
	dstring_terminate(&query);

	wait(&rc);
	if (rc != 0)
	{
		printf("%s:%d: command sed terminated with exitcode %d\n",
				stmt->stmt_filename, stmt->stmt_lno, rc);
		dstring_free(&query);
		return -1;
	}

	/*
	 * This little hoop is required because for some
	 * reason, 7.3 returns total garbage as a result
	 * code for such a big pile of commands. So we just
	 * fire that off, and then do one extra select and
	 * see if we have an aborted transaction.
	 */
	res = PQexec(adminfo->dbconn, dstring_data(&query));
	dstring_free(&query);
	PQclear(res);
	res = PQexec(adminfo->dbconn, "select 1;");
	rc = PQresultStatus(res);
	if (rc != PGRES_TUPLES_OK)
	{
		printf("%s:%d: loading of file %s: %s %s%s",
				stmt->stmt_filename, stmt->stmt_lno,
				fnamebuf, PQresStatus(rc),
				PQresultErrorMessage(res),
				PQerrorMessage(adminfo->dbconn));
		PQclear(res);
		return -1;
	}
	PQclear(res);

	return 0;
}


static int
load_slony_base(SlonikStmt *stmt, int no_id)
{
	SlonikAdmInfo  *adminfo;
	PGconn		   *dbconn;
	SlonDString		query;
	int				rc;

	int				use_major = 0;
	int				use_minor = 0;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return -1;
	
	dbconn = adminfo->dbconn;

	rc = db_check_namespace(stmt, adminfo, stmt->script->clustername);
	if (rc > 0)
	{
		printf("%s:%d: Error: namespace \"_%s\" "
				"already exists in database of node %d\n",
				stmt->stmt_filename, stmt->stmt_lno,
				stmt->script->clustername, no_id);
		return -1;
	}
	if (rc < 0)
	{
		printf("%s:%d: Error: cannot determine "
				" if namespace \"_%s\" exists in node %d\n",
				stmt->stmt_filename, stmt->stmt_lno,
				stmt->script->clustername, no_id);
		return -1;
	}

	switch (adminfo->version_major)
	{
		case 7:
			use_major = 7;

			switch (adminfo->version_minor)
			{
				case 3:
					use_minor = 3;
					break;

				case 4:
				case 5:
					use_minor = 4;
					break;

				default:
					printf("%s:%d: unsupported PostgreSQL "
							"version %d.%d\n",
							stmt->stmt_filename, stmt->stmt_lno,
							adminfo->version_major, adminfo->version_minor);
			}
			break;

		default:
			printf("%s:%d: unsupported PostgreSQL "
					"version %d.%d\n",
					stmt->stmt_filename, stmt->stmt_lno,
					adminfo->version_major, adminfo->version_minor);
	}

	dstring_init(&query);

	/* Create the cluster namespace */
	slon_mkquery(&query,
			"create schema \"_%s\";", stmt->script->clustername);
	if (db_exec_command(stmt, adminfo, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/* Load schema, DB version specific */
	db_notice_silent = true;
	if (load_sql_script(stmt, adminfo,
			"%s/xxid.v%d%d.sql", PGSHARE, use_major, use_minor) < 0
		|| load_sql_script(stmt, adminfo,
			"%s/slony1_base.sql", PGSHARE) < 0 
		|| load_sql_script(stmt, adminfo,
			"%s/slony1_base.v%d%d.sql", PGSHARE, use_major, use_minor) < 0
		|| load_sql_script(stmt, adminfo,
			"%s/slony1_funcs.sql", PGSHARE) < 0 
		|| load_sql_script(stmt, adminfo,
			"%s/slony1_funcs.v%d%d.sql", PGSHARE, use_major, use_minor) < 0)
	{
		db_notice_silent = false;
		dstring_free(&query);
		return -1;
	}
	db_notice_silent = false;

	return 0;
}


/* ------------------------------------------------------------
 * Statement specific processing functions follow
 * ------------------------------------------------------------
 */


int
slonik_restart_node(SlonikStmt_restart_node *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"notify \"_%s_Restart\"; ",
			stmt->hdr.script->clustername);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_init_cluster(SlonikStmt_init_cluster *stmt)
{
	SlonikAdmInfo  *adminfo;
	SlonDString		query;
	int				rc;

	adminfo = get_active_adminfo((SlonikStmt *)stmt, stmt->no_id);
	if (adminfo == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo) < 0)
		return -1;

	rc = load_slony_base((SlonikStmt *)stmt, stmt->no_id);
	if (rc < 0)
		return -1;

	/* call initializeLocalNode() and enableNode() */
	dstring_init(&query);
	slon_mkquery(&query,
			"select \"_%s\".initializeLocalNode(%d, '%q'); "
			"select \"_%s\".enableNode(%d); ",
			stmt->hdr.script->clustername, stmt->no_id, stmt->no_comment,
			stmt->hdr.script->clustername, stmt->no_id);
	if (db_exec_command((SlonikStmt *)stmt, adminfo, &query) < 0)
		rc = -1;
	else
		rc = 0;
	dstring_free(&query);

	return rc;
}


int
slonik_store_node(SlonikStmt_store_node *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonikAdmInfo  *adminfo2;
	SlonDString		query;
	int				rc;
	PGresult	   *res;
	int				ntuples;
	int				tupno;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	adminfo2 = get_checked_adminfo((SlonikStmt *)stmt, stmt->ev_origin);
	if (adminfo2 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;
	if (db_begin_xact((SlonikStmt *)stmt, adminfo2) < 0)
		return -1;

	/* Load the slony base tables */
	rc = load_slony_base((SlonikStmt *)stmt, stmt->no_id);
	if (rc < 0)
		return -1;

	/* call initializeLocalNode() and enableNode_int() */
	dstring_init(&query);
	slon_mkquery(&query,
			"select \"_%s\".initializeLocalNode(%d, '%q'); "
			"select \"_%s\".enableNode_int(%d); ",
			stmt->hdr.script->clustername, stmt->no_id, stmt->no_comment,
			stmt->hdr.script->clustername, stmt->no_id);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/*
	 * Duplicate the content of sl_node
	 */
	slon_mkquery(&query,
			"select no_id, no_active, no_comment "
			"from \"_%s\".sl_node; ",
			stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *)stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char   *no_id = PQgetvalue(res, tupno, 0);
		char   *no_active = PQgetvalue(res, tupno, 1);
		char   *no_comment = PQgetvalue(res, tupno, 2);

		slon_mkquery(&query,
				"select \"_%s\".storeNode_int(%s, '%q'); ",
				stmt->hdr.script->clustername, no_id, no_comment);
		if (*no_active == 't') 
		{
			slon_appendquery(&query,
				"select \"_%s\".enableNode_int(%s); ",
				stmt->hdr.script->clustername, no_id);
		}

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);

	/*
	 * Duplicate the content of sl_path
	 */
	slon_mkquery(&query,
			"select pa_server, pa_client, pa_conninfo, pa_connretry "
			"from \"_%s\".sl_path; ",
			stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *)stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char   *pa_server = PQgetvalue(res, tupno, 0);
		char   *pa_client = PQgetvalue(res, tupno, 1);
		char   *pa_conninfo = PQgetvalue(res, tupno, 2);
		char   *pa_connretry = PQgetvalue(res, tupno, 3);

		slon_mkquery(&query,
				"select \"_%s\".storePath_int(%s, %s, '%q', %s); ",
				stmt->hdr.script->clustername, 
				pa_server, pa_client, pa_conninfo, pa_connretry);

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);

	/*
	 * Duplicate the content of sl_listen
	 */
	slon_mkquery(&query,
			"select li_origin, li_provider, li_receiver "
			"from \"_%s\".sl_listen; ",
			stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *)stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char   *li_origin = PQgetvalue(res, tupno, 0);
		char   *li_provider = PQgetvalue(res, tupno, 1);
		char   *li_receiver = PQgetvalue(res, tupno, 2);

		slon_mkquery(&query,
				"select \"_%s\".storeListen_int(%s, %s, %s); ",
				stmt->hdr.script->clustername, 
				li_origin, li_provider, li_receiver);

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);

	/*
	 * Duplicate the content of sl_set
	 */
	slon_mkquery(&query,
			"select set_id, set_origin, set_comment "
			"from \"_%s\".sl_set; ",
			stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *)stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char   *set_id = PQgetvalue(res, tupno, 0);
		char   *set_origin = PQgetvalue(res, tupno, 1);
		char   *set_comment = PQgetvalue(res, tupno, 2);

		slon_mkquery(&query,
				"select \"_%s\".storeSet_int(%s, %s, '%q'); ",
				stmt->hdr.script->clustername, 
				set_id, set_origin, set_comment);

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);

	/*
	 * Duplicate the content of sl_subscribe
	 */
	slon_mkquery(&query,
			"select sub_set, sub_provider, sub_receiver, "
			"	sub_forward, sub_active "
			"from \"_%s\".sl_subscribe; ",
			stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *)stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char   *sub_set = PQgetvalue(res, tupno, 0);
		char   *sub_provider = PQgetvalue(res, tupno, 1);
		char   *sub_receiver = PQgetvalue(res, tupno, 2);
		char   *sub_forward = PQgetvalue(res, tupno, 3);
		char   *sub_active = PQgetvalue(res, tupno, 4);

		slon_mkquery(&query,
				"select \"_%s\".storeSubscribe_int(%s, %s, %s, '%q'); ",
				stmt->hdr.script->clustername, 
				sub_set, sub_provider, sub_receiver, sub_forward);
		if (*sub_active == 't')
		{
			slon_mkquery(&query,
				"select \"_%s\".enableSubscription_int(%s, %s); ",
				stmt->hdr.script->clustername, 
				sub_set, sub_receiver);
		}

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);


	/* On the existing node, call storeNode() and enableNode() */
	slon_mkquery(&query,
			"select \"_%s\".storeNode(%d, '%q'); "
			"select \"_%s\".enableNode(%d); ",
			stmt->hdr.script->clustername, stmt->no_id, stmt->no_comment,
			stmt->hdr.script->clustername, stmt->no_id);
	if (db_exec_command((SlonikStmt *)stmt, adminfo2, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_store_path(SlonikStmt_store_path *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->pa_client);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"select \"_%s\".storePath(%d, %d, '%q', %d); ",
			stmt->hdr.script->clustername,
			stmt->pa_server, stmt->pa_client, 
			stmt->pa_conninfo, stmt->pa_connretry);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_store_listen(SlonikStmt_store_listen *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->li_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"select \"_%s\".storeListen(%d, %d, %d); ",
			stmt->hdr.script->clustername,
			stmt->li_origin, stmt->li_provider, 
			stmt->li_receiver);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_drop_listen(SlonikStmt_drop_listen *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->li_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"select \"_%s\".dropListen(%d, %d, %d); ",
			stmt->hdr.script->clustername,
			stmt->li_origin, stmt->li_provider, 
			stmt->li_receiver);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_create_set(SlonikStmt_create_set *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"select \"_%s\".storeSet(%d, '%q'); ",
			stmt->hdr.script->clustername,
			stmt->set_id, stmt->set_comment);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_set_add_table(SlonikStmt_set_add_table *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;
	char		   *attkind;
	PGresult	   *res;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * Determine the attkind of the table.
	 * The stored procedure for KEY = SERIAL might actually
	 * add a bigserial column to the table.
	 */
	if (stmt->use_serial)
	{
		slon_mkquery(&query,
				"select \"_%s\".determineAttkindSerial('%q');",
				stmt->hdr.script->clustername,
				stmt->tab_fqname);
	}
	else
	{
		if (stmt->use_key == NULL)
		{
			slon_mkquery(&query,
					"select \"_%s\".determineAttkindUnique('%q', NULL);",
					stmt->hdr.script->clustername,
					stmt->tab_fqname);
		}
		else
		{
			slon_mkquery(&query,
					"select \"_%s\".determineAttkindUnique('%q', '%q');",
					stmt->hdr.script->clustername,
					stmt->tab_fqname, stmt->use_key);
		}
	}

	db_notice_silent = true;
	res = db_exec_select((SlonikStmt *)stmt, adminfo1, &query);
	db_notice_silent = false;
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	attkind = PQgetvalue(res, 0, 0);

	slon_mkquery(&query,
			"select \"_%s\".setAddTable(%d, %d, '%q', '%q', '%q'); ",
			stmt->hdr.script->clustername,
			stmt->set_id, stmt->tab_id,
			stmt->tab_fqname, attkind, stmt->tab_comment);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		PQclear(res);
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	PQclear(res);
	return 0;
}


int
slonik_table_add_key(SlonikStmt_table_add_key *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * call tableAddKey()
	 */
	db_notice_silent = true;
	slon_mkquery(&query,
			"select \"_%s\".tableAddKey('%q'); ",
			stmt->hdr.script->clustername,
			stmt->tab_fqname);
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		db_notice_silent = false;
		dstring_free(&query);
		return -1;
	}
	db_notice_silent = false;

	dstring_free(&query);
	return 0;
}


int
slonik_subscribe_set(SlonikStmt_subscribe_set *stmt)
{
	SlonikAdmInfo  *adminfo1;
	SlonDString		query;

	adminfo1 = get_active_adminfo((SlonikStmt *)stmt, stmt->sub_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *)stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
			"select \"_%s\".subscribeSet(%d, %d, %d, '%s'); ",
			stmt->hdr.script->clustername,
			stmt->sub_setid, stmt->sub_provider, 
			stmt->sub_receiver,
			(stmt->sub_forward) ? "t" : "f");
	if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


