/*-------------------------------------------------------------------------
 * slonik.c
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
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#else
#define sleep(x) Sleep(x*1000)
#define vsnprintf _vsnprintf
#endif

#include "postgres.h"
#include "libpq-fe.h"
#include "port.h"

#include "slonik.h"
#include "config.h"
#include "../parsestatements/scanner.h"
extern int STMTS[MAXSTATEMENTS];

/*
 * Global data
 */
SlonikScript *parser_script = NULL;
int			parser_errors = 0;
int			current_try_level;

static char myfull_path[MAXPGPATH];
static char share_path[MAXPGPATH];

/*
 * Local functions
 */
static void usage(void);
static SlonikAdmInfo *get_adminfo(SlonikStmt * stmt, int no_id);
static SlonikAdmInfo *get_active_adminfo(SlonikStmt * stmt, int no_id);
static SlonikAdmInfo *get_checked_adminfo(SlonikStmt * stmt, int no_id);
static int	slonik_repair_config(SlonikStmt_repair_config * stmt);


static int	script_check(SlonikScript * script);
static int	script_check_adminfo(SlonikStmt * hdr, int no_id);
static int	script_check_stmts(SlonikScript * script,
					SlonikStmt * stmt);
static int	script_exec(SlonikScript * script);
static int	script_exec_stmts(SlonikScript * script,
					SlonikStmt * stmt);
static void	script_commit_all(SlonikStmt * stmt,
					SlonikScript * script);
static void script_rollback_all(SlonikStmt * stmt,
					SlonikScript * script);
static void script_disconnect_all(SlonikScript * script);
static void replace_token(char *resout, char *lines, const char *token, 
					const char *replacement);

/* ----------
 * main
 * ----------
 */
int
main(int argc, const char *argv[])
{
	extern int	optind;
	int			opt;

	while ((opt = getopt(argc, (char **)argv, "hv")) != EOF)
	{
		switch (opt)
		{
			case 'h':
				parser_errors++;
				break;

			case 'v':
				printf("slonik version %s\n", SLONY_I_VERSION_STRING);
				exit(0);
				break;

			default:
				printf("unknown option '%c'\n", opt);
				parser_errors++;
				break;
		}
	}

	if (parser_errors)
		usage();

	/*
	 * We need to find a share directory like PostgreSQL. 
	 */
	if (strlen(PGSHARE) > 0)
	{
		strcpy(share_path, PGSHARE);
	}
	else
	{
		get_share_path(myfull_path, share_path);
	}

	if (optind < argc)
	{
		while (optind < argc)
		{
			FILE	   *fp;

			fp = fopen(argv[optind], "r");
			if (fp == NULL)
			{
				printf("could not open file '%s'\n", argv[optind]);
				return -1;
			}
			scan_new_input_file(fp);
			current_file = (char *)argv[optind++];
			yylineno = 1;
			yyparse();
			fclose(fp);

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
		scan_new_input_file(stdin);
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
	exit(1);
}


/* ----------
 * script_check -
 * ----------
 */
static int
script_check(SlonikScript * script)
{
	SlonikAdmInfo *adminfo;
	SlonikAdmInfo *adminfo2;
	int			errors = 0;


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
script_check_adminfo(SlonikStmt * hdr, int no_id)
{
	SlonikAdmInfo *adminfo;

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
script_check_stmts(SlonikScript * script, SlonikStmt * hdr)
{
	int			errors = 0;

	while (hdr)
	{
		hdr->script = script;

		switch (hdr->stmt_type)
		{
			case STMT_TRY:
				{
					SlonikStmt_try *stmt =
					(SlonikStmt_try *) hdr;

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

			case STMT_DATE:
				break;

			case STMT_EXIT:
				break;

			case STMT_RESTART_NODE:
				{
					SlonikStmt_restart_node *stmt =
					(SlonikStmt_restart_node *) hdr;

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_REPAIR_CONFIG:
				{
					SlonikStmt_repair_config *stmt =
					(SlonikStmt_repair_config *) hdr;

					if (stmt->ev_origin < 0)
					{
						stmt->ev_origin = 1;
					}
					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
					{
						errors++;
					}
				}
				break;

			case STMT_ERROR:
				break;

			case STMT_INIT_CLUSTER:
				{
					SlonikStmt_init_cluster *stmt =
					(SlonikStmt_init_cluster *) hdr;

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
					(SlonikStmt_store_node *) hdr;

					if (stmt->ev_origin < 0)
					{
						printf("%s:%d: Error: require EVENT NODE\n", 
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

					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;
				}
				break;

			case STMT_DROP_NODE:
				{
					SlonikStmt_drop_node *stmt =
					(SlonikStmt_drop_node *) hdr;

					if (stmt->ev_origin < 0)
					{
						printf("%s:%d: Error: require EVENT NODE\n", 
						       hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->ev_origin == stmt->no_id)
					{
						printf("%s:%d: Error: "
							   "Node ID and event node cannot be identical\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;
				}
				break;

			case STMT_FAILED_NODE:
				{
					SlonikStmt_failed_node *stmt =
					(SlonikStmt_failed_node *) hdr;

					if (stmt->backup_node < 0)
					{
						printf("%s:%d: Error: require BACKUP NODE\n", 
						       hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->backup_node == stmt->no_id)
					{
						printf("%s:%d: Error: "
							 "Node ID and backup node cannot be identical\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (script_check_adminfo(hdr, stmt->backup_node) < 0)
						errors++;
				}
				break;

			case STMT_UNINSTALL_NODE:
				{
					SlonikStmt_uninstall_node *stmt =
					(SlonikStmt_uninstall_node *) hdr;

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_CLONE_PREPARE:
				{
					SlonikStmt_clone_prepare *stmt =
					(SlonikStmt_clone_prepare *) hdr;

					if (stmt->no_id < 0)
					{
						printf("%s:%d: Error: "
							   "new node ID must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->no_provider) < 0)
						errors++;
				}
				break;

			case STMT_CLONE_FINISH:
				{
					SlonikStmt_clone_finish *stmt =
					(SlonikStmt_clone_finish *) hdr;

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_STORE_PATH:
				{
					SlonikStmt_store_path *stmt =
					(SlonikStmt_store_path *) hdr;

					if (script_check_adminfo(hdr, stmt->pa_client) < 0)
						errors++;

				}
				break;

			case STMT_DROP_PATH:
				{
					SlonikStmt_drop_path *stmt =
					(SlonikStmt_drop_path *) hdr;

					if (stmt->ev_origin < 0)
						stmt->ev_origin = stmt->pa_client;

					if (stmt->pa_server < 0)
					{
						printf("%s:%d: Error: "
							   "server must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (stmt->pa_client < 0)
					{
						printf("%s:%d: Error: "
							   "client must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;
				}
				break;

			case STMT_STORE_LISTEN:
				{
					SlonikStmt_store_listen *stmt =
					(SlonikStmt_store_listen *) hdr;

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
					(SlonikStmt_drop_listen *) hdr;

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
					(SlonikStmt_create_set *) hdr;

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_DROP_SET:
				{
					SlonikStmt_drop_set *stmt =
					(SlonikStmt_drop_set *) hdr;

					if (stmt->set_id < 0)
					{
						printf("%s:%d: Error: "
							   "set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_MERGE_SET:
				{
					SlonikStmt_merge_set *stmt =
					(SlonikStmt_merge_set *) hdr;

					if (stmt->set_id < 0)
					{
						printf("%s:%d: Error: "
							   "set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (stmt->add_id < 0)
					{
						printf("%s:%d: Error: "
							   "set id to merge (add) must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_SET_ADD_TABLE:
				{
					SlonikStmt_set_add_table *stmt =
					(SlonikStmt_set_add_table *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
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
					 * Check that we have the table id, name and what to use
					 * for the key.
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
					if (stmt->tab_comment == NULL)
						stmt->tab_comment = strdup(stmt->tab_fqname);
				}
				break;

			case STMT_SET_ADD_SEQUENCE:
				{
					SlonikStmt_set_add_sequence *stmt =
					(SlonikStmt_set_add_sequence *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
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
					 * Check that we have the table id, name and what to use
					 * for the key.
					 */
					if (stmt->seq_id < 0)
					{
						printf("%s:%d: Error: "
							   "sequence id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->seq_fqname == NULL)
					{
						printf("%s:%d: Error: "
							   "sequence FQ-name must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (stmt->seq_comment == NULL)
						stmt->seq_comment = strdup(stmt->seq_fqname);
				}
				break;

			case STMT_SET_DROP_TABLE:
				{
					SlonikStmt_set_drop_table *stmt =
					(SlonikStmt_set_drop_table *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
					 */
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
					 * Check that we have the table id, name and what to use
					 * for the key.
					 */
					if (stmt->tab_id < 0)
					{
						printf("%s:%d: Error: "
							   "table id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
				}
				break;

			case STMT_SET_DROP_SEQUENCE:
				{
					SlonikStmt_set_drop_sequence *stmt =
					(SlonikStmt_set_drop_sequence *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
					 */
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
					 * Check that we have the table id, name and what to use
					 * for the key.
					 */
					if (stmt->seq_id < 0)
					{
						printf("%s:%d: Error: "
							   "sequence id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
				}
				break;

			case STMT_SET_MOVE_TABLE:
				{
					SlonikStmt_set_move_table *stmt =
					(SlonikStmt_set_move_table *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
					 */
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
					 * Check that we have the table id and new set id
					 */
					if (stmt->tab_id < 0)
					{
						printf("%s:%d: Error: "
							   "table id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->new_set_id < 0)
					{
						printf("%s:%d: Error: "
							   "new set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
				}
				break;

			case STMT_SET_MOVE_SEQUENCE:
				{
					SlonikStmt_set_move_sequence *stmt =
					(SlonikStmt_set_move_sequence *) hdr;

					/*
					 * Check that we have the set_id and set_origin and that
					 * we can reach the origin.
					 */
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
					 * Check that we have the sequence id and new set id
					 */
					if (stmt->seq_id < 0)
					{
						printf("%s:%d: Error: "
							   "sequence id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->new_set_id < 0)
					{
						printf("%s:%d: Error: "
							   "new set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
				}
				break;


			case STMT_SUBSCRIBE_SET:
				{
					SlonikStmt_subscribe_set *stmt =
					(SlonikStmt_subscribe_set *) hdr;

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

					if (script_check_adminfo(hdr, stmt->sub_provider) < 0)
						errors++;
				}
				break;

			case STMT_UNSUBSCRIBE_SET:
				{
					SlonikStmt_unsubscribe_set *stmt =
					(SlonikStmt_unsubscribe_set *) hdr;

					if (stmt->sub_setid < 0)
					{
						printf("%s:%d: Error: "
							   "set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->sub_receiver) < 0)
						errors++;
				}
				break;
			case STMT_LOCK_SET:
				{
					SlonikStmt_lock_set *stmt =
					(SlonikStmt_lock_set *) hdr;

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

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_UNLOCK_SET:
				{
					SlonikStmt_unlock_set *stmt =
					(SlonikStmt_unlock_set *) hdr;

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

					if (script_check_adminfo(hdr, stmt->set_origin) < 0)
						errors++;
				}
				break;

			case STMT_MOVE_SET:
				{
					SlonikStmt_move_set *stmt =
					(SlonikStmt_move_set *) hdr;

					if (stmt->set_id < 0)
					{
						printf("%s:%d: Error: "
							   "set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->old_origin < 0)
					{
						printf("%s:%d: Error: "
							   "old origin must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->new_origin < 0)
					{
						printf("%s:%d: Error: "
							   "new origin must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->new_origin == stmt->old_origin)
					{
						printf("%s:%d: Error: "
							   "old and new origin cannot be identical\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->old_origin) < 0)
						errors++;
				}
				break;

			case STMT_DDL_SCRIPT:
				{
					SlonikStmt_ddl_script *stmt =
					(SlonikStmt_ddl_script *) hdr;

					if ((stmt->only_on_node > -1) && (stmt->ev_origin > -1)
							&& (stmt->ev_origin != stmt->only_on_node)) {
						printf ("If ONLY ON NODE is given, "
								"EVENT ORIGIN must be the same node");
						errors++;
					}
					if (stmt->only_on_node > -1)
						stmt->ev_origin = stmt->only_on_node;

					if (stmt->ev_origin < 0)
					{
						printf("%s:%d: Error: require EVENT NODE\n", 
						       hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->ddl_setid < 0)
					{
						printf("%s:%d: Error: "
							   "set id must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->ddl_fname == NULL)
					{
						printf("%s:%d: Error: "
							   "script file name must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;

					stmt->ddl_fd = fopen(stmt->ddl_fname, "r");
					if (stmt->ddl_fd == NULL)
					{
						printf("%s:%d: Error: "
							   "%s - %s\n",
							   hdr->stmt_filename, hdr->stmt_lno,
							   stmt->ddl_fname, strerror(errno));
						errors++;
					}
				}
				break;

			case STMT_UPDATE_FUNCTIONS:
				{
					SlonikStmt_update_functions *stmt =
					(SlonikStmt_update_functions *) hdr;

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;
				}
				break;

			case STMT_WAIT_EVENT:
				{
					SlonikStmt_wait_event *stmt =
					(SlonikStmt_wait_event *) hdr;

					if (stmt->wait_origin == -1)
					{
						printf("%s:%d: Error: "
							   "event origin to wait for must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->wait_confirmed == -1)
					{
						printf("%s:%d: Error: "
						   "confirming node to wait for must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->wait_on) < 0)
						errors++;

				}
				break;

			case STMT_SWITCH_LOG:
				{
					SlonikStmt_switch_log *stmt =
					(SlonikStmt_switch_log *) hdr;

					if (stmt->no_id == -1)
					{
						printf("%s:%d: Error: "
							   "node ID must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;

				}
				break;

			case STMT_SYNC:
				{
					SlonikStmt_sync *stmt =
					(SlonikStmt_sync *) hdr;

					if (stmt->no_id == -1)
					{
						printf("%s:%d: Error: "
							   "node ID must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->no_id) < 0)
						errors++;

				}
				break;
			case STMT_SLEEP:
				{
					SlonikStmt_sleep *stmt =
					(SlonikStmt_sleep *) hdr;

					if (stmt->num_secs < 0)
					{
						printf("%s:%d: Error: "
							   " sleep time (%d) must be positive\n",
						       hdr->stmt_filename, hdr->stmt_lno, stmt->num_secs);
						errors++;
					}


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
script_exec(SlonikScript * script)
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
script_exec_stmts(SlonikScript * script, SlonikStmt * hdr)
{
	int			errors = 0;

	while (hdr && errors == 0)
	{
		hdr->script = script;

		switch (hdr->stmt_type)
		{
			case STMT_TRY:
				{
					SlonikStmt_try *stmt =
					(SlonikStmt_try *) hdr;
					int			rc;

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
							 * suppressed setting the overall script on error.
							 */
							if (script_exec_stmts(script, stmt->error_block) < 0)
								errors++;
						}
						else
						{
							/*
							 * The try-block has no ON ERROR action, abort
							 * script execution.
							 */
							errors++;
						}
					}
					else
					{
						script_commit_all(hdr, script);

						/*
						 * If the try block has an ON SUCCESS block, execute
						 * that.
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
					(SlonikStmt_echo *) hdr;

					printf("%s:%d: %s\n",
						   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
						   stmt->str);
				}
				break;

			case STMT_DATE:
				{
					SlonikStmt_date *stmt =
					(SlonikStmt_date *) hdr;
                    char outstr[200];

                    struct tm *local;
                    time_t t;

                    t = time(NULL);
                    local = localtime(&t);
                    strftime(outstr, sizeof(outstr), stmt->fmt, local);
					printf("%s:%d: %s\n",
						   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
                           outstr);
				}
				break;

			case STMT_EXIT:
				{
					SlonikStmt_exit *stmt =
					(SlonikStmt_exit *) hdr;

					exit(stmt->exitcode);
				}
				break;

			case STMT_RESTART_NODE:
				{
					SlonikStmt_restart_node *stmt =
					(SlonikStmt_restart_node *) hdr;

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
					(SlonikStmt_init_cluster *) hdr;

					if (slonik_init_cluster(stmt) < 0)
					errors++;
				}
				break;

			case STMT_REPAIR_CONFIG:
				{
					SlonikStmt_repair_config *stmt =
					(SlonikStmt_repair_config *) hdr;

					if (slonik_repair_config(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_NODE:
				{
					SlonikStmt_store_node *stmt =
					(SlonikStmt_store_node *) hdr;

					if (slonik_store_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DROP_NODE:
				{
					SlonikStmt_drop_node *stmt =
					(SlonikStmt_drop_node *) hdr;

					if (slonik_drop_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_FAILED_NODE:
				{
					SlonikStmt_failed_node *stmt =
					(SlonikStmt_failed_node *) hdr;

					if (slonik_failed_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_UNINSTALL_NODE:
				{
					SlonikStmt_uninstall_node *stmt =
					(SlonikStmt_uninstall_node *) hdr;

					if (slonik_uninstall_node(stmt) < 0)
						errors++;
				}
				break;

			case STMT_CLONE_PREPARE:
				{
					SlonikStmt_clone_prepare *stmt =
					(SlonikStmt_clone_prepare *) hdr;

					if (slonik_clone_prepare(stmt) < 0)
						errors++;
				}
				break;

			case STMT_CLONE_FINISH:
				{
					SlonikStmt_clone_finish *stmt =
					(SlonikStmt_clone_finish *) hdr;

					if (slonik_clone_finish(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_PATH:
				{
					SlonikStmt_store_path *stmt =
					(SlonikStmt_store_path *) hdr;

					if (slonik_store_path(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DROP_PATH:
				{
					SlonikStmt_drop_path *stmt =
					(SlonikStmt_drop_path *) hdr;

					if (slonik_drop_path(stmt) < 0)
						errors++;
				}
				break;

			case STMT_STORE_LISTEN:
				{
					SlonikStmt_store_listen *stmt =
					(SlonikStmt_store_listen *) hdr;

					if (slonik_store_listen(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DROP_LISTEN:
				{
					SlonikStmt_drop_listen *stmt =
					(SlonikStmt_drop_listen *) hdr;

					if (slonik_drop_listen(stmt) < 0)
						errors++;
				}
				break;

			case STMT_CREATE_SET:
				{
					SlonikStmt_create_set *stmt =
					(SlonikStmt_create_set *) hdr;

					if (slonik_create_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DROP_SET:
				{
					SlonikStmt_drop_set *stmt =
					(SlonikStmt_drop_set *) hdr;

					if (slonik_drop_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_MERGE_SET:
				{
					SlonikStmt_merge_set *stmt =
					(SlonikStmt_merge_set *) hdr;

					if (slonik_merge_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_ADD_TABLE:
				{
					SlonikStmt_set_add_table *stmt =
					(SlonikStmt_set_add_table *) hdr;

					if (slonik_set_add_table(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_ADD_SEQUENCE:
				{
					SlonikStmt_set_add_sequence *stmt =
					(SlonikStmt_set_add_sequence *) hdr;

					if (slonik_set_add_sequence(stmt) < 0)
						errors++;
				}
				break;
			case STMT_SET_DROP_TABLE:
				{
					SlonikStmt_set_drop_table *stmt =
					(SlonikStmt_set_drop_table *) hdr;

					if (slonik_set_drop_table(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_DROP_SEQUENCE:
				{
					SlonikStmt_set_drop_sequence *stmt =
					(SlonikStmt_set_drop_sequence *) hdr;

					if (slonik_set_drop_sequence(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_MOVE_TABLE:
				{
					SlonikStmt_set_move_table *stmt =
					(SlonikStmt_set_move_table *) hdr;

					if (slonik_set_move_table(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SET_MOVE_SEQUENCE:
				{
					SlonikStmt_set_move_sequence *stmt =
					(SlonikStmt_set_move_sequence *) hdr;

					if (slonik_set_move_sequence(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SUBSCRIBE_SET:
				{
					SlonikStmt_subscribe_set *stmt =
					(SlonikStmt_subscribe_set *) hdr;

					if (slonik_subscribe_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_UNSUBSCRIBE_SET:
				{
					SlonikStmt_unsubscribe_set *stmt =
					(SlonikStmt_unsubscribe_set *) hdr;

					if (slonik_unsubscribe_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_LOCK_SET:
				{
					SlonikStmt_lock_set *stmt =
					(SlonikStmt_lock_set *) hdr;

					if (slonik_lock_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_UNLOCK_SET:
				{
					SlonikStmt_unlock_set *stmt =
					(SlonikStmt_unlock_set *) hdr;

					if (slonik_unlock_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_MOVE_SET:
				{
					SlonikStmt_move_set *stmt =
					(SlonikStmt_move_set *) hdr;

					if (slonik_move_set(stmt) < 0)
						errors++;
				}
				break;

			case STMT_DDL_SCRIPT:
				{
					SlonikStmt_ddl_script *stmt =
					(SlonikStmt_ddl_script *) hdr;

					if (slonik_ddl_script(stmt) < 0)
						errors++;
				}
				break;

			case STMT_UPDATE_FUNCTIONS:
				{
					SlonikStmt_update_functions *stmt =
					(SlonikStmt_update_functions *) hdr;

					if (slonik_update_functions(stmt) < 0)
						errors++;
				}
				break;

			case STMT_WAIT_EVENT:
				{
					SlonikStmt_wait_event *stmt =
					(SlonikStmt_wait_event *) hdr;

					if (slonik_wait_event(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SWITCH_LOG:
				{
					SlonikStmt_switch_log *stmt =
					(SlonikStmt_switch_log *) hdr;

					if (slonik_switch_log(stmt) < 0)
						errors++;
				}
				break;

			case STMT_SYNC:
				{
					SlonikStmt_sync *stmt =
					(SlonikStmt_sync *) hdr;

					if (slonik_sync(stmt) < 0)
						errors++;
				}
				break;
			case STMT_SLEEP:
				{
					SlonikStmt_sleep *stmt =
					(SlonikStmt_sleep *) hdr;

					if (slonik_sleep(stmt) < 0)
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
script_commit_all(SlonikStmt * stmt, SlonikScript * script)
{
	SlonikAdmInfo *adminfo;
	int			error = 0;

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
script_rollback_all(SlonikStmt * stmt, SlonikScript * script)
{
	SlonikAdmInfo *adminfo;

	for (adminfo = script->adminfo_list;
		 adminfo; adminfo = adminfo->next)
	{
		if (adminfo->dbconn != NULL && adminfo->have_xact)
			db_rollback_xact(stmt, adminfo);
	}
}


static void
script_disconnect_all(SlonikScript * script)
{
	SlonikAdmInfo *adminfo;
	SlonikStmt	eofstmt;

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
get_adminfo(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;

	for (adminfo = stmt->script->adminfo_list;
		 adminfo; adminfo = adminfo->next)
	{
		if (adminfo->no_id == no_id)
			return adminfo;
	}

	return NULL;
}


static SlonikAdmInfo *
get_active_adminfo(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;
	int version;

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

		version = db_get_version(stmt, adminfo);
		if (version < 0)
		{
			PQfinish(adminfo->dbconn);
			adminfo->dbconn = NULL;
			return NULL;
		}
		adminfo->pg_version = version;

		if (db_rollback_xact(stmt, adminfo) < 0)
		{
			PQfinish(adminfo->dbconn);
			adminfo->dbconn = NULL;
			return NULL;
		}
	}

	return adminfo;
}


static SlonikAdmInfo *
get_checked_adminfo(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;
	int			db_no_id;
	int			had_xact;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return NULL;

	if (!adminfo->nodeid_checked)
	{
		had_xact = adminfo->have_xact;

		if ((db_no_id = db_get_nodeid(stmt, adminfo)) != no_id)
		{
			printf("%s:%d: database specified in %s:%d reports no_id %d\n",
				   stmt->stmt_filename, stmt->stmt_lno,
				   adminfo->stmt_filename, adminfo->stmt_lno,
				   db_no_id);
			return NULL;
		}
		adminfo->nodeid_checked = true;

		if (!had_xact)
		{
			if (db_rollback_xact(stmt, adminfo) < 0)
				return NULL;
		}
	}

	return adminfo;
}


static int
load_sql_script(SlonikStmt * stmt, SlonikAdmInfo * adminfo, char *fname,...)
{
	PGresult   *res;
	SlonDString query;
	va_list		ap;
	int			rc;
	char		fnamebuf[1024];
	char		buf[4096];
	char		rex1[256];
	char		rex2[256];
	char		rex3[256];
	char		rex4[256];
	FILE	   *stmtp;


	if (db_begin_xact(stmt, adminfo) < 0)
		return -1;

	va_start(ap, fname);
	vsnprintf(fnamebuf, sizeof(fnamebuf), fname, ap);
	va_end(ap);

	stmtp = fopen(fnamebuf, "r");
	if (!stmtp)
	{
		printf("%s:%d: could not open file %s\n",
			   stmt->stmt_filename, stmt->stmt_lno, fnamebuf);
		return -1;
	}

	dstring_init(&query);

	sprintf(rex2, "\"_%s\"", stmt->script->clustername);

	while (fgets(rex1, 256, stmtp) != NULL)
	{
		rc = strlen(rex1);
		rex1[rc] = '\0';
		rex3[0] = '\0';
		replace_token(rex3, rex1, "@CLUSTERNAME@", stmt->script->clustername);
		replace_token(rex4, rex3, "@MODULEVERSION@", SLONY_I_VERSION_STRING);
		replace_token(buf, rex4, "@NAMESPACE@", rex2);
		rc = strlen(buf);
		dstring_nappend(&query, buf, rc);
	}

	fclose(stmtp);

	dstring_terminate(&query);

	res = PQexec(adminfo->dbconn, dstring_data(&query));

	rc = PQresultStatus(res);
	if ((rc != PGRES_TUPLES_OK) && (rc != PGRES_COMMAND_OK) && (rc != PGRES_EMPTY_QUERY))
	{
		printf("%s:%d: loading of file %s: %s %s%s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   fnamebuf, PQresStatus(rc),
			   PQresultErrorMessage(res),
			   PQerrorMessage(adminfo->dbconn));
		PQclear(res);
		return -1;
	}
	dstring_free(&query);
	PQclear(res);

	return 0;
}


static int
load_slony_base(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;
	PGconn	   *dbconn;
	SlonDString query;
	int			rc;

	int			use_major = 0;
	int			use_minor = 0;

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

	if (db_check_requirements(stmt, adminfo, stmt->script->clustername) < 0)
	{
		return -1;
	}

	/* determine what schema version we should load */
	if (adminfo->pg_version < 80300)	/* before 8.3 */
	{
		printf("%s:%d: unsupported PostgreSQL "
			"version %d.%d (versions < 8.3 are not supported by Slony-I >= 2.0)\n",
			stmt->stmt_filename, stmt->stmt_lno,
			(adminfo->pg_version/10000), ((adminfo->pg_version%10000)/100));
			return -1;
	}
	else if ((adminfo->pg_version >= 80300) && (adminfo->pg_version < 80400)) /* 8.3 */
	{
		use_major = 8;
		use_minor = 3;
	}
	else if ((adminfo->pg_version >= 80400) && (adminfo->pg_version < 80500)) /* 8.4 */
	{
		use_major = 8;
		use_minor = 4;   
	}		
	else if ((adminfo->pg_version >= 90000) && (adminfo->pg_version < 90100)) /* 9.4 */
	{
		/**
		 * 9.0 is so far just like 8.4
		 **/
		use_major=8;
		use_minor=4;
	}
	else	/* above 8.4 ??? */
	{
		use_major = 8;
		use_minor = 4;
		printf("%s:%d: Possible unsupported PostgreSQL "
			"version (%d) %d.%d, defaulting to 8.4 support\n",
                        stmt->stmt_filename, stmt->stmt_lno, adminfo->pg_version,
			(adminfo->pg_version/10000), ((adminfo->pg_version%10000)/100));
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
						   "%s/slony1_base.sql", share_path) < 0
		|| load_sql_script(stmt, adminfo,
			 "%s/slony1_base.v%d%d.sql", share_path, use_major, use_minor) < 0
		|| load_sql_script(stmt, adminfo,
						   "%s/slony1_funcs.sql", share_path) < 0
		|| load_sql_script(stmt, adminfo,
		   "%s/slony1_funcs.v%d%d.sql", share_path, use_major, use_minor) < 0)
	{
		db_notice_silent = false;
		dstring_free(&query);
		return -1;
	}
	db_notice_silent = false;

	dstring_free(&query);

	return 0;
}


static int
load_slony_functions(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;
	PGconn	   *dbconn;

	int			use_major = 0;
	int			use_minor = 0;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return -1;

	dbconn = adminfo->dbconn;

        /* determine what schema version we should load */

        if (adminfo->pg_version < 80300)        /* before 8.3 */
        {
                printf("%s:%d: unsupported PostgreSQL "
                        "version %d.%d\n",
                        stmt->stmt_filename, stmt->stmt_lno,
                        (adminfo->pg_version/10000), ((adminfo->pg_version%10000)/100));
			return -1;
        }
        else if ((adminfo->pg_version >= 80300) && adminfo->pg_version < 80400) /* 8.0 */
        {
                use_major = 8;
                use_minor = 3;
        }
	else if ((adminfo->pg_version >= 80400) && (adminfo->pg_version < 80500)) /* 8.4 */
	{
		use_major = 8;
		use_minor = 4;
	}
	else if ((adminfo->pg_version >= 90000) && (adminfo->pg_version < 90100)) /* 9.0 */
	{
		/**
		 * 9.0 is so far just like 8.4
		 */
		use_major = 8;
		use_minor = 4;
	}
        else    /* above 8.4 */
        {
                use_major = 8;
                use_minor = 3;
                printf("%s:%d: Possible unsupported PostgreSQL "
                       "version (%d) %d.%d, defaulting to 8.3 support\n",
		       stmt->stmt_filename, stmt->stmt_lno, adminfo->pg_version,
		       (adminfo->pg_version/10000), ((adminfo->pg_version%10000)/100));
        }

	/* Load schema, DB version specific */
	db_notice_silent = true;
	if (load_sql_script(stmt, adminfo,
						"%s/slony1_funcs.sql", share_path) < 0
		|| load_sql_script(stmt, adminfo,
		   "%s/slony1_funcs.v%d%d.sql", share_path, use_major, use_minor) < 0)
	{
		db_notice_silent = false;
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
slonik_restart_node(SlonikStmt_restart_node * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "notify \"_%s_Restart\"; ",
				 stmt->hdr.script->clustername);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


static int
slonik_repair_config(SlonikStmt_repair_config * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".updateReloid(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->only_on_node);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_init_cluster(SlonikStmt_init_cluster * stmt)
{
	SlonikAdmInfo *adminfo;
	SlonDString query;
	int			rc;

	adminfo = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo) < 0)
		return -1;

	rc = load_slony_base((SlonikStmt *) stmt, stmt->no_id);
	if (rc < 0)
		return -1;

	/* call initializeLocalNode() and enableNode() */
	dstring_init(&query);
	slon_mkquery(&query,
				 "select \"_%s\".initializeLocalNode(%d, '%q'); "
				 "select \"_%s\".enableNode(%d); ",
				 stmt->hdr.script->clustername, stmt->no_id, stmt->no_comment,
				 stmt->hdr.script->clustername, stmt->no_id);
	if (db_exec_command((SlonikStmt *) stmt, adminfo, &query) < 0)
		rc = -1;
	else
		rc = 0;
	dstring_free(&query);

	return rc;
}


int
slonik_store_node(SlonikStmt_store_node * stmt)
{
	SlonikAdmInfo *adminfo1 = NULL;
	SlonikAdmInfo *adminfo2;
	SlonDString query;
	int			rc;
	PGresult   *res;
	int			ntuples;
	int			tupno;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;
	

	adminfo2 = get_checked_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo2 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo2) < 0)
		return -1;

	dstring_init(&query);

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	
	/* Load the slony base tables */
	rc = load_slony_base((SlonikStmt *) stmt, stmt->no_id);
	if (rc < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/* call initializeLocalNode() and enableNode_int() */
	slon_mkquery(&query,
		     "select \"_%s\".initializeLocalNode(%d, '%q'); "
		     "select \"_%s\".enableNode_int(%d); ",
		     stmt->hdr.script->clustername, stmt->no_id, stmt->no_comment,
		     stmt->hdr.script->clustername, stmt->no_id);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *no_id = PQgetvalue(res, tupno, 0);
		char	   *no_active = PQgetvalue(res, tupno, 1);
		char	   *no_comment = PQgetvalue(res, tupno, 2);
		
		slon_mkquery(&query,
			     "select \"_%s\".storeNode_int(%s, '%q'); ",
			     stmt->hdr.script->clustername, no_id, no_comment);
		if (*no_active == 't')
		{
			slon_appendquery(&query,
					 "select \"_%s\".enableNode_int(%s); ",
					 stmt->hdr.script->clustername, no_id);
		}
		
		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *pa_server = PQgetvalue(res, tupno, 0);
		char	   *pa_client = PQgetvalue(res, tupno, 1);
		char	   *pa_conninfo = PQgetvalue(res, tupno, 2);
		char	   *pa_connretry = PQgetvalue(res, tupno, 3);

		slon_mkquery(&query,
			     "select \"_%s\".storePath_int(%s, %s, '%q', %s); ",
			     stmt->hdr.script->clustername,
			     pa_server, pa_client, pa_conninfo, pa_connretry);

		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *li_origin = PQgetvalue(res, tupno, 0);
		char	   *li_provider = PQgetvalue(res, tupno, 1);
		char	   *li_receiver = PQgetvalue(res, tupno, 2);
		
		slon_mkquery(&query,
			     "select \"_%s\".storeListen_int(%s, %s, %s); ",
			     stmt->hdr.script->clustername,
			     li_origin, li_provider, li_receiver);
		
		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *set_id = PQgetvalue(res, tupno, 0);
		char	   *set_origin = PQgetvalue(res, tupno, 1);
		char	   *set_comment = PQgetvalue(res, tupno, 2);

		slon_mkquery(&query,
			     "select \"_%s\".storeSet_int(%s, %s, '%q'); ",
			     stmt->hdr.script->clustername,
			     set_id, set_origin, set_comment);

		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *sub_set = PQgetvalue(res, tupno, 0);
		char	   *sub_provider = PQgetvalue(res, tupno, 1);
		char	   *sub_receiver = PQgetvalue(res, tupno, 2);
		char	   *sub_forward = PQgetvalue(res, tupno, 3);
		char	   *sub_active = PQgetvalue(res, tupno, 4);
			
		slon_mkquery(&query,
			     "select \"_%s\".subscribeSet_int(%s, %s, %s, '%q', 'f'); ",
			     stmt->hdr.script->clustername,
			     sub_set, sub_provider, sub_receiver, sub_forward);
		if (*sub_active == 't')
		{
			slon_appendquery(&query,
					 "select \"_%s\".enableSubscription_int(%s, %s, %s); ",
					 stmt->hdr.script->clustername,
					 sub_set, sub_provider, sub_receiver);
		}
		
		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			PQclear(res);
			return -1;
		}
	}
	PQclear(res);
	
	/*
	 * Set our own event seqno in case the node id was used before and our
	 * confirms.
	 */
	slon_mkquery(&query,
		     "select ev_origin, max(ev_seqno) "
		     "    from \"_%s\".sl_event "
		     "    group by ev_origin; ",
		     stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *) stmt, adminfo2, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	ntuples = PQntuples(res);
	for (tupno = 0; tupno < ntuples; tupno++)
	{
		char	   *ev_origin_c = PQgetvalue(res, tupno, 0);
		char	   *ev_seqno_c = PQgetvalue(res, tupno, 1);

		if (stmt->no_id == (int)strtol(ev_origin_c, NULL, 10))
		{
			slon_mkquery(&query,
				     "select \"pg_catalog\".setval('\"_%s\".sl_event_seq', "
				     "'%s'::int8 + 1); ",
				     stmt->hdr.script->clustername, ev_seqno_c);
		}
		else
		{
			slon_appendquery(&query,
					 "insert into \"_%s\".sl_confirm "
					 "    (con_origin, con_received, con_seqno, con_timestamp) "
					 "    values "
					 "    (%s, %d, '%s', CURRENT_TIMESTAMP); ",
					 stmt->hdr.script->clustername,
					 ev_origin_c, stmt->no_id, ev_seqno_c);
		}

		if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo2, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_drop_node(SlonikStmt_drop_node * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".dropNode(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->no_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


typedef struct
{
	int			no_id;
	SlonikAdmInfo *adminfo;
	int			has_slon;
	int			slon_pid;
	int			num_sets;
}	failnode_node;


typedef struct
{
	int			set_id;
	int			num_directsub;
	int			num_subscribers;
	failnode_node **subscribers;
	failnode_node *max_node;
	int64		max_seqno;
}	failnode_set;


int
slonik_failed_node(SlonikStmt_failed_node * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	int			num_nodes;
	int			num_sets;
	int			n,
				i,
				j,
				k;

	failnode_node *nodeinfo;
	failnode_set *setinfo;
	char	   *configbuf;

	PGresult   *res1;
	PGresult   *res2;
	PGresult   *res3;
	int64		max_seqno_total = 0;
	failnode_node *max_node_total = NULL;

	int			rc = 0;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->backup_node);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * On the backup node select a list of all active nodes except for the
	 * failed node.
	 */
	slon_mkquery(&query,
				 "select no_id from \"_%s\".sl_node "
				 "    where no_id <> %d "
				 "    and no_active "
				 "    order by no_id; ",
				 stmt->hdr.script->clustername,
				 stmt->no_id);
	res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	if (res1 == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	num_nodes = PQntuples(res1);

	/*
	 * Get a list of all sets that are subscribed more than once directly from
	 * the origin
	 */
	slon_mkquery(&query,
				 "select S.set_id, count(S.set_id) "
				 "    from \"_%s\".sl_set S, \"_%s\".sl_subscribe SUB "
				 "    where S.set_id = SUB.sub_set "
				 "    and S.set_origin = %d "
				 "    and SUB.sub_provider = %d "
				 "    and SUB.sub_active "
				 "    group by set_id ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->no_id, stmt->no_id);
	res2 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	if (res2 == NULL)
	{
		PQclear(res1);
		dstring_free(&query);
		return -1;
	}
	num_sets = PQntuples(res2);

	/*
	 * Allocate and initialize memory to hold some config info
	 */
	configbuf = malloc(
					   MAXALIGN(sizeof(failnode_node) * num_nodes) +
					   MAXALIGN(sizeof(failnode_set) * num_sets) +
					   sizeof(failnode_node *) * num_nodes * num_sets);
	memset(configbuf, 0,
		   MAXALIGN(sizeof(failnode_node) * num_nodes) +
		   MAXALIGN(sizeof(failnode_set) * num_sets) +
		   sizeof(failnode_node *) * num_nodes * num_sets);

	nodeinfo = (failnode_node *) configbuf;
	setinfo = (failnode_set *) (configbuf +
								MAXALIGN(sizeof(failnode_node) * num_nodes));
	for (i = 0; i < num_sets; i++)
	{
		setinfo[i].subscribers = (failnode_node **)
			(configbuf + MAXALIGN(sizeof(failnode_node) * num_nodes) +
			 MAXALIGN(sizeof(failnode_set) * num_sets) +
			 sizeof(failnode_node *) * num_nodes * i);
	}

	/*
	 * Connect to all these nodes and determine if there is a node daemon
	 * running on that node.
	 */
	for (i = 0; i < num_nodes; i++)
	{
		nodeinfo[i].no_id = (int)strtol(PQgetvalue(res1, i, 0), NULL, 10);
		nodeinfo[i].adminfo = get_active_adminfo((SlonikStmt *) stmt,
												 nodeinfo[i].no_id);
		if (nodeinfo[i].adminfo == NULL)
		{
			PQclear(res1);
			free(configbuf);
			dstring_free(&query);
			return -1;
		}

		slon_mkquery(&query,
					 "lock table \"_%s\".sl_config_lock; "
					 "select nl_backendpid from \"_%s\".sl_nodelock "
					 "    where nl_nodeid = \"_%s\".getLocalNodeId('_%s') and "
					 "       exists (select 1 from pg_catalog.pg_stat_activity "
					 "                 where procpid = nl_backendpid);",
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername);
		res3 = db_exec_select((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query);
		if (res3 == NULL)
		{
			PQclear(res1);
			PQclear(res2);
			free(configbuf);
			dstring_free(&query);
			return -1;
		}
		if (PQntuples(res3) == 0)
		{
			nodeinfo[i].has_slon = false;
			nodeinfo[i].slon_pid = 0;
		}
		else
		{
			nodeinfo[i].has_slon = true;
			nodeinfo[i].slon_pid = (int)strtol(PQgetvalue(res3, 0, 0), NULL, 10);
		}
		PQclear(res3);
	}
	PQclear(res1);

	/*
	 * For every set we're interested in lookup the direct subscriber nodes.
	 */
	for (i = 0; i < num_sets; i++)
	{
		setinfo[i].set_id = (int)strtol(PQgetvalue(res2, i, 0), NULL, 10);
		setinfo[i].num_directsub = (int)strtol(PQgetvalue(res2, i, 1), NULL, 10);

		if (setinfo[i].num_directsub <= 1)
			continue;

		slon_mkquery(&query,
					 "select sub_receiver "
					 "    from \"_%s\".sl_subscribe "
					 "    where sub_set = %d "
					 "    and sub_provider = %d "
					 "    and sub_active and sub_forward; ",
					 stmt->hdr.script->clustername,
					 setinfo[i].set_id,
					 stmt->no_id);

		res3 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res3 == NULL)
		{
			free(configbuf);
			dstring_free(&query);
			return -1;
		}
		n = PQntuples(res3);

		for (j = 0; j < n; j++)
		{
			int			sub_receiver = (int)strtol(PQgetvalue(res3, j, 0), NULL, 10);

			for (k = 0; k < num_nodes; k++)
			{
				if (nodeinfo[k].no_id == sub_receiver)
				{
					setinfo[i].subscribers[setinfo[i].num_subscribers] =
						&nodeinfo[k];
					setinfo[i].num_subscribers++;
					break;
				}
			}
			if (k == num_nodes)
			{
				printf("node %d not found - inconsistent configuration\n",
					   sub_receiver);
				free(configbuf);
				PQclear(res3);
				PQclear(res2);
				dstring_free(&query);
				return -1;
			}
		}
		PQclear(res3);
	}
	PQclear(res2);

	/*
	 * Execute the failedNode() procedure, first on the backup node, then on
	 * all other nodes.
	 */
	slon_mkquery(&query,
				 "select \"_%s\".failedNode(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->no_id, stmt->backup_node);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		free(configbuf);
		dstring_free(&query);
		return -1;
	}
	for (i = 0; i < num_nodes; i++)
	{
		if (nodeinfo[i].no_id == stmt->backup_node)
			continue;

		if (db_exec_command((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query) < 0)
		{
			free(configbuf);
			dstring_free(&query);
			return -1;
		}
	}

	/*
	 * Big danger from now on, we commit the work done so far
	 */
	for (i = 0; i < num_nodes; i++)
	{
		if (db_commit_xact((SlonikStmt *) stmt, nodeinfo[i].adminfo) < 0)
		{
			free(configbuf);
			dstring_free(&query);
			return -1;
		}
	}

	/*
	 * Wait until all slon replication engines that were running have
	 * restarted.
	 */
	n = 0;
	while (n < num_nodes)
	{
		sleep(1);
		n = 0;
		for (i = 0; i < num_nodes; i++)
		{
			if (!nodeinfo[i].has_slon)
			{
				n++;
				continue;
			}

			slon_mkquery(&query,
						 "select nl_backendpid from \"_%s\".sl_nodelock "
						 "    where nl_backendpid <> %d; ",
						 stmt->hdr.script->clustername,
						 nodeinfo[i].slon_pid);
			res1 = db_exec_select((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query);
			if (res1 == NULL)
			{
				free(configbuf);
				dstring_free(&query);
				return -1;
			}
			if (PQntuples(res1) == 1)
			{
				nodeinfo[i].has_slon = false;
				n++;
			}

			PQclear(res1);
			if (db_rollback_xact((SlonikStmt *) stmt, nodeinfo[i].adminfo) < 0)
			{
				free(configbuf);
				dstring_free(&query);
				return -1;
			}
		}
	}

	/*
	 * Determine the absolutely last event sequence known from the failed
	 * node.
	 */
	slon_mkquery(&query,
				 "select max(ev_seqno) "
				 "    from \"_%s\".sl_event "
				 "    where ev_origin = %d; ",
				 stmt->hdr.script->clustername,
				 stmt->no_id);
	for (i = 0; i < num_nodes; i++)
	{
		res1 = db_exec_select((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query);
		if (res1 != NULL)
		{
			if (PQntuples(res1) == 1)
			{
				int64		max_seqno;

				slon_scanint64(PQgetvalue(res1, 0, 0), &max_seqno);
				if (max_seqno > max_seqno_total)
				{
					max_seqno_total = max_seqno;
					max_node_total = &nodeinfo[i];
				}
			}
			PQclear(res1);
		}
		else
			rc = -1;
	}

	/*
	 * For every set determine the direct subscriber with the highest applied
	 * sync, preferring the backup node.
	 */
	for (i = 0; i < num_sets; i++)
	{
		setinfo[i].max_node = NULL;
		setinfo[i].max_seqno = 0;

		if (setinfo[i].num_directsub <= 1)
		{
			int64		ev_seqno;

			slon_mkquery(&query,
						 "select max(ev_seqno) "
						 "	from \"_%s\".sl_event "
						 "	where ev_origin = %d "
						 "	and ev_type = 'SYNC'; ",
						 stmt->hdr.script->clustername,
						 stmt->no_id);
			res1 = db_exec_select((SlonikStmt *) stmt,
								  adminfo1, &query);
			if (res1 == NULL)
			{
				free(configbuf);
				dstring_free(&query);
				return -1;
			}
			slon_scanint64(PQgetvalue(res1, 0, 0), &ev_seqno);

			setinfo[i].max_seqno = ev_seqno;

			PQclear(res1);

			continue;
		}

		slon_mkquery(&query,
					 "select ssy_seqno "
					 "    from \"_%s\".sl_setsync "
					 "    where ssy_setid = %d; ",
					 stmt->hdr.script->clustername,
					 setinfo[i].set_id);

		for (j = 0; j < setinfo[i].num_subscribers; j++)
		{
			int64		ssy_seqno;

			res1 = db_exec_select((SlonikStmt *) stmt,
								  setinfo[i].subscribers[j]->adminfo, &query);
			if (res1 == NULL)
			{
				free(configbuf);
				dstring_free(&query);
				return -1;
			}
			if (PQntuples(res1) == 1)
			{
				slon_scanint64(PQgetvalue(res1, 0, 0), &ssy_seqno);

				if (setinfo[i].subscribers[j]->no_id == stmt->backup_node)
				{
					if (ssy_seqno >= setinfo[i].max_seqno)
					{
						setinfo[i].max_node = setinfo[i].subscribers[j];
						setinfo[i].max_seqno = ssy_seqno;
					}
				}
				else
				{
					if (ssy_seqno > setinfo[i].max_seqno)
					{
						setinfo[i].max_node = setinfo[i].subscribers[j];
						setinfo[i].max_seqno = ssy_seqno;
					}
				}

				if (ssy_seqno > max_seqno_total)
					max_seqno_total = ssy_seqno;
			}
			else
			{
				printf("can't get setsync status for set %d from node %d\n",
					   setinfo[i].set_id, setinfo[i].subscribers[j]->no_id);
				rc = -1;
			}

			PQclear(res1);
		}
	}

	/*
	 * Now switch the backup node to receive all sets from those highest
	 * nodes.
	 */
	for (i = 0; i < num_sets; i++)
	{
		int			use_node;
		SlonikAdmInfo *use_adminfo;

		if (setinfo[i].num_directsub <= 1)
		{
			use_node = stmt->backup_node;
			use_adminfo = adminfo1;
		}
		else if (setinfo[i].max_node == NULL)
		{
			printf("no setsync status for set %d found at all\n",
				   setinfo[i].set_id);
			rc = -1;
			use_node = stmt->backup_node;
			use_adminfo = adminfo1;
		}
		else
		{
			printf("IMPORTANT: Last known SYNC for set %d = "
				   INT64_FORMAT "\n",
				   setinfo[i].set_id,
				   setinfo[i].max_seqno);
			use_node = setinfo[i].max_node->no_id;
			use_adminfo = setinfo[i].max_node->adminfo;

			setinfo[i].max_node->num_sets++;
		}

		if (use_node != stmt->backup_node)
		{
			slon_mkquery(&query,
						 "select \"_%s\".storeListen(%d,%d,%d); "
						 "select \"_%s\".subscribeSet_int(%d,%d,%d,'t','f'); ",
						 stmt->hdr.script->clustername,
						 stmt->no_id, use_node, stmt->backup_node,
						 stmt->hdr.script->clustername,
						 setinfo[i].set_id, use_node, stmt->backup_node);
			if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
				rc = -1;
		}
	}

	/*
	 * Commit the transaction on the backup node to activate those changes.
	 */
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
		rc = -1;

	/*
	 * Now execute all FAILED_NODE events on the node that had the highest of
	 * all events alltogether.
	 */
	if (max_node_total != NULL)
	{
		for (i = 0; i < num_sets; i++)
		{
			char		ev_seqno_c[64];
			char		ev_seqfake_c[64];

			sprintf(ev_seqno_c, INT64_FORMAT, setinfo[i].max_seqno);
			sprintf(ev_seqfake_c, INT64_FORMAT, ++max_seqno_total);

			slon_mkquery(&query,
						 "select \"_%s\".failedNode2(%d,%d,%d,'%s','%s'); ",
						 stmt->hdr.script->clustername,
						 stmt->no_id, stmt->backup_node,
						 setinfo[i].set_id, ev_seqno_c, ev_seqfake_c);
			if (db_exec_command((SlonikStmt *) stmt,
								max_node_total->adminfo, &query) < 0)
				rc = -1;
		}
	}

	/*
	 * commit all open transactions despite of all possible errors
	 */
	for (i = 0; i < num_nodes; i++)
	{
		if (db_commit_xact((SlonikStmt *) stmt, nodeinfo[i].adminfo) < 0)
			rc = -1;
	}

	free(configbuf);
	dstring_free(&query);
	return rc;
}


int
slonik_uninstall_node(SlonikStmt_uninstall_node * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".uninstallNode(); ",
				 stmt->hdr.script->clustername);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		printf("Failed to exec uninstallNode() for node %d\n", stmt->no_id);
		dstring_free(&query);
		return -1;
	}
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		printf("Failed to commit uninstallNode() for node %d\n", stmt->no_id);
		dstring_free(&query);
		return -1;
	}

	slon_mkquery(&query,
				 "drop schema \"_%s\" cascade; ",
				 stmt->hdr.script->clustername);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		printf("Failed to drop schema for node %d\n", stmt->no_id);
		dstring_free(&query);
		return -1;
	}
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		printf("Failed to commit drop schema for node %d\n", stmt->no_id);
		dstring_free(&query);
		return -1;
	}

	db_disconnect((SlonikStmt *) stmt, adminfo1);

	dstring_free(&query);
	return 0;
}


int
slonik_clone_prepare(SlonikStmt_clone_prepare * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_provider);
	if (adminfo1 == NULL)
		return -1;

	dstring_init(&query);

	if (stmt->no_comment == NULL)
		slon_mkquery(&query,
				 "select \"_%s\".cloneNodePrepare(%d, %d, 'Node %d'); ",
				 stmt->hdr.script->clustername,
				 stmt->no_id, stmt->no_provider,
				 stmt->no_id);
	else
		slon_mkquery(&query,
				 "select \"_%s\".cloneNodePrepare(%d, %d, '%q'); ",
				 stmt->hdr.script->clustername,
				 stmt->no_id, stmt->no_provider,
				 stmt->no_comment);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_clone_finish(SlonikStmt_clone_finish * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".cloneNodeFinish(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->no_id, stmt->no_provider);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_store_path(SlonikStmt_store_path * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->pa_client);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".storePath(%d, %d, '%q', %d); ",
				 stmt->hdr.script->clustername,
				 stmt->pa_server, stmt->pa_client,
				 stmt->pa_conninfo, stmt->pa_connretry);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_drop_path(SlonikStmt_drop_path * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".dropPath(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->pa_server, stmt->pa_client);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_store_listen(SlonikStmt_store_listen * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->li_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".storeListen(%d, %d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->li_origin, stmt->li_provider,
				 stmt->li_receiver);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_drop_listen(SlonikStmt_drop_listen * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->li_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".dropListen(%d, %d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->li_origin, stmt->li_provider,
				 stmt->li_receiver);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


#define CREATESET_DEFCOMMENT "A replication set so boring no one thought to give it a name"
int
slonik_create_set(SlonikStmt_create_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;
	const char *comment;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	if (stmt->set_comment == NULL)
		comment = CREATESET_DEFCOMMENT;
	else
		comment = stmt->set_comment;

	dstring_init(&query);

	slon_mkquery(&query,
		     "select \"_%s\".storeSet(%d, '%q'); ",
		     stmt->hdr.script->clustername,
		     stmt->set_id, comment);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_drop_set(SlonikStmt_drop_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".dropSet(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_merge_set(SlonikStmt_merge_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".mergeSet(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->add_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_set_add_table(SlonikStmt_set_add_table * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;
	char	   *idxname;
	PGresult   *res;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	if (stmt->use_key == NULL)
	{
		slon_mkquery(&query,
			     "select \"_%s\".determineIdxnameUnique('%q', NULL); ",
			     stmt->hdr.script->clustername, stmt->tab_fqname);

	}
	else
	{
		slon_mkquery(&query,
			     "select \"_%s\".determineIdxnameUnique('%q', '%q'); ",
			     stmt->hdr.script->clustername,
			     stmt->tab_fqname, stmt->use_key);
	}

	db_notice_silent = true;
	res = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	db_notice_silent = false;
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	idxname = PQgetvalue(res, 0, 0);

	slon_mkquery(&query,
				 "select \"_%s\".setAddTable(%d, %d, '%q', '%q', '%q'); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->tab_id,
				 stmt->tab_fqname, idxname, stmt->tab_comment);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
slonik_set_add_sequence(SlonikStmt_set_add_sequence * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * call setAddSequence()
	 */
	db_notice_silent = true;
	slon_mkquery(&query,
				 "select \"_%s\".setAddSequence(%d, %d, '%q', '%q'); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->seq_id, stmt->seq_fqname,
				 stmt->seq_comment);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
slonik_set_drop_table(SlonikStmt_set_drop_table * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".setDropTable(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->tab_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_free(&query);
	return 0;
}


int
slonik_set_drop_sequence(SlonikStmt_set_drop_sequence * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * call setDropSequence()
	 */
	db_notice_silent = true;
	slon_mkquery(&query,
				 "select \"_%s\".setDropSequence(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->seq_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
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
slonik_set_move_table(SlonikStmt_set_move_table * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".setMoveTable(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->tab_id, stmt->new_set_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_free(&query);
	return 0;
}


int
slonik_set_move_sequence(SlonikStmt_set_move_sequence * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".setMoveSequence(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->seq_id, stmt->new_set_id);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_free(&query);
	return 0;
}

int
slonik_subscribe_set(SlonikStmt_subscribe_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonikAdmInfo *adminfo2;
	SlonDString query;
	PGresult    *res1;
	int reshape=0;
	int origin_id;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->sub_provider);
	if (adminfo1 == NULL)
		return -1;

	
	dstring_init(&query);

	/**
	 * If the receiver is already subscribed to
	 * the set through a different provider
	 * slonik will need to tell the receiver
	 * about this change directly.
	 */

	slon_mkquery(&query,"select count(*) FROM \"_%s\".sl_subscribe " \
				 "where sub_set=%d AND sub_receiver=%d " \
				 " and sub_active=true and sub_provider<>%d",
				 stmt->hdr.script->clustername,
				 stmt->sub_setid,stmt->sub_receiver,
				 stmt->sub_provider);
	
	res1 = db_exec_select((SlonikStmt*) stmt,adminfo1,&query);
	if(res1 == NULL) {
		dstring_free(&query);
		return -1;
	}
	if (strtol(PQgetvalue(res1, 0, 0), NULL, 10) > 0)
	{
		reshape=1;
	}
	PQclear(res1);
	dstring_reset(&query);

	slon_mkquery(&query,
				 "select set_origin from \"_%s\".sl_set where" \
				 " set_id=%d",stmt->hdr.script->clustername,
				 stmt->sub_setid);
	res1 = db_exec_select((SlonikStmt*)stmt,adminfo1,&query);
	if(res1==NULL) 
	{
		PQclear(res1);
		dstring_free(&query);
		return -1;

	}
	origin_id = atoi(PQgetvalue(res1,0,0));
	if(origin_id <= 0 ) 
	{
		PQclear(res1);
		dstring_free(&query);
		return -1;
		
	}
	PQclear(res1);
	dstring_reset(&query);
	adminfo2 = get_active_adminfo((SlonikStmt *) stmt, origin_id);
	if (db_begin_xact((SlonikStmt *) stmt, adminfo2) < 0)
		return -1;
	slon_mkquery(&query,
				 "select \"_%s\".subscribeSet(%d, %d, %d, '%s', '%s'); ",
				 stmt->hdr.script->clustername,
				 stmt->sub_setid, stmt->sub_provider,
				 stmt->sub_receiver,
				 (stmt->sub_forward) ? "t" : "f",
				 (stmt->omit_copy) ? "t" : "f");
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo2, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_reset(&query);
	if(reshape)
	{
		adminfo2 = get_active_adminfo((SlonikStmt *) stmt, stmt->sub_receiver);
		if(adminfo2 == NULL) 
		{
			printf("can not find conninfo for receiver node %d\n",
				   stmt->sub_receiver);
			return -1;
		}
		slon_mkquery(&query,
					 "select \"_%s\".reshapeSubscription(%d,%d,%d);",
					 stmt->hdr.script->clustername,
					 stmt->sub_provider,stmt->sub_setid,
					 stmt->sub_receiver);	
		if (db_exec_command((SlonikStmt *) stmt, adminfo2, &query) < 0)
		{
			printf("error reshaping subscriber\n");		
		}
		
		dstring_free(&query);
	}
	return 0;
}


int
slonik_unsubscribe_set(SlonikStmt_unsubscribe_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->sub_receiver);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".unsubscribeSet(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->sub_setid, stmt->sub_receiver);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_lock_set(SlonikStmt_lock_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;
	PGresult   *res1;
	PGresult   *res2;
	char	   *maxxid_lock;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (adminfo1->have_xact)
	{
		printf("%s:%d: cannot lock set - admin connection already in transaction\n",
			   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
		return -1;
	}

	/*
	 * We issue the lockSet() and get the current xmax
	 */
	dstring_init(&query);
	slon_mkquery(&query,
		     "select \"_%s\".lockSet(%d); "
		     "select pg_catalog.txid_snapshot_xmax(pg_catalog.txid_current_snapshot());",
		     stmt->hdr.script->clustername,
		     stmt->set_id);
	res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	if (res1 == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	maxxid_lock = PQgetvalue(res1, 0, 0);

	/*
	 * Now we need to commit this already and wait until xmin is >= that xmax
	 */
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		dstring_free(&query);
		PQclear(res1);
		return -1;
	}

	slon_mkquery(&query,
		     "select pg_catalog.txid_snapshot_xmin(pg_catalog.txid_current_snapshot()) >= '%s'; ",
		     maxxid_lock);

	while (true)
	{
		res2 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res2 == NULL)
		{
			PQclear(res1);
			dstring_free(&query);
			return -1;
		}

		if (*PQgetvalue(res2, 0, 0) == 't')
			break;

		PQclear(res2);
		if (db_rollback_xact((SlonikStmt *) stmt, adminfo1) < 0)
		{
			dstring_free(&query);
			PQclear(res1);
			return -1;
		}

		sleep(1);
	}

	PQclear(res1);
	PQclear(res2);
	dstring_free(&query);
	return 0;
}


int
slonik_unlock_set(SlonikStmt_unlock_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".unlockSet(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_move_set(SlonikStmt_move_set * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->old_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".moveSet(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->new_origin);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_ddl_script(SlonikStmt_ddl_script * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;
	SlonDString script;
	int			rc;
	int			num_statements = -1, stmtno;
	char		buf[4096];
	char		rex1[256];
	char		rex2[256];
	char		rex3[256];
	char		rex4[256];

#define PARMCOUNT 1  

	const char *params[PARMCOUNT];
	int paramlens[PARMCOUNT];
	int paramfmts[PARMCOUNT];

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&script);

	sprintf(rex2, "\"_%s\"", stmt->hdr.script->clustername);

	while (fgets(rex1, 256, stmt->ddl_fd) != NULL)
	{
		rc = strlen(rex1);
		rex1[rc] = '\0';
		replace_token(rex3, rex1, "@CLUSTERNAME@", stmt->hdr.script->clustername);
		replace_token(rex4, rex3, "@MODULEVERSION@", SLONY_I_VERSION_STRING);
		replace_token(buf, rex4, "@NAMESPACE@", rex2);
		rc = strlen(buf);
		dstring_nappend(&script, buf, rc);
	}
	dstring_terminate(&script);

	dstring_init(&query);
	slon_mkquery(&query,
		     "select \"_%s\".ddlScript_prepare(%d, %d); ",
		     stmt->hdr.script->clustername,
		     stmt->ddl_setid, /* dstring_data(&script),  */ 
		     stmt->only_on_node);

	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/* Split the script into a series of SQL statements - each needs to
	   be submitted separately */
	num_statements = scan_for_statements (dstring_data(&script));
	printf("DDL script consisting of %d SQL statements\n", num_statements);

	/* OOPS!  Something went wrong !!! */
	if ((num_statements < 0) || (num_statements >= MAXSTATEMENTS)) {
		printf("DDL - number of statements invalid - %d not between 0 and %d\n", num_statements, MAXSTATEMENTS);
		return -1;
	}
	for (stmtno=0; stmtno < num_statements;  stmtno++) {
		int startpos, endpos;
		char *dest;
		if (stmtno == 0)
			startpos = 0;
		else
			startpos = STMTS[stmtno-1];
		endpos = STMTS[stmtno];
		dest = (char *) malloc (endpos - startpos + 1);
		if (dest == 0) {
			printf("DDL Failure - could not allocate %d bytes of memory\n", endpos - startpos + 1);
			return -1;
		}
		strncpy(dest, dstring_data(&script) + startpos, endpos-startpos);
		dest[STMTS[stmtno]-startpos] = 0;
		slon_mkquery(&query, "%s", dest);
		printf("DDL Statement %d: (%d,%d) [%s]\n", stmtno, startpos, endpos, dest);
		free(dest);

		if (db_exec_command((SlonikStmt *)stmt, adminfo1, &query) < 0)
		{
			dstring_free(&query);
			return -1;
		}
	}
	
	printf("Submit DDL Event to subscribers...\n");

	slon_mkquery(&query, "select \"_%s\".ddlScript_complete(%d, $1::text, %d); ", 
		     stmt->hdr.script->clustername,
		     stmt->ddl_setid,  
		     stmt->only_on_node);

	paramlens[PARMCOUNT-1] = 0;
	paramfmts[PARMCOUNT-1] = 0;
	params[PARMCOUNT-1] = dstring_data(&script);

	if (db_exec_evcommand_p((SlonikStmt *)stmt, adminfo1, &query,
				PARMCOUNT, NULL, params, paramlens, paramfmts, 0) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	
	dstring_free(&script);
	dstring_free(&query);
	return 0;
}


int
slonik_update_functions(SlonikStmt_update_functions * stmt)
{
	SlonikAdmInfo *adminfo;
	PGresult   *res;
	SlonDString query;

	adminfo = get_checked_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo) < 0)
		return -1;

	/*
	 * Check if the currently loaded schema has the function slonyVersion()
	 * defined.
	 */
	dstring_init(&query);
	slon_mkquery(&query,
				 "select true from pg_proc P, pg_namespace N "
				 "    where P.proname = 'slonyversion' "
				 "    and P.pronamespace = N.oid "
				 "    and N.nspname = '_%s';",
				 stmt->hdr.script->clustername);
	res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
	if (res == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	if (PQntuples(res) == 0)
	{
		/*
		 * No - this must be a 1.0.2 or earlier. Generate a query to call
		 * upgradeSchema() from 1.0.2.
		 */
		PQclear(res);
		slon_mkquery(&query,
					 "select \"_%s\".upgradeSchema('1.0.2'); ",
					 stmt->hdr.script->clustername);
	}
	else
	{
		/*
		 * Yes - call the function and generate a query to call
		 * upgradeSchema() from that version later.
		 */
		PQclear(res);
		slon_mkquery(&query,
					 "select \"_%s\".slonyVersion(); ",
					 stmt->hdr.script->clustername);
		res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
		if (res == NULL)
		{
			dstring_free(&query);
			return -1;
		}
		if (PQntuples(res) != 1)
		{
			printf("%s:%d: failed to determine current schema version\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
			PQclear(res);
			dstring_free(&query);
			return -1;
		}
		slon_mkquery(&query,
					 "select \"_%s\".upgradeSchema('%s'); ",
					 stmt->hdr.script->clustername,
					 PQgetvalue(res, 0, 0));
		PQclear(res);
	}

	/*
	 * Load the new slony1_functions.sql
	 */
	if (load_slony_functions((SlonikStmt *) stmt, stmt->no_id) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/*
	 * Call the upgradeSchema() function
	 */
	if (db_exec_command((SlonikStmt *) stmt, adminfo, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	/*
	 * Finally restart the node.
	 */

	return slonik_restart_node((SlonikStmt_restart_node *) stmt);
}


int
slonik_wait_event(SlonikStmt_wait_event * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonikAdmInfo *adminfo;
	SlonikScript *script = stmt->hdr.script;
	SlonDString query;
	PGresult   *res;
	time_t		timeout;
	time_t		now;
	int			all_confirmed = 0;
	char		seqbuf[64];

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->wait_on);
	if (adminfo1 == NULL)
		return -1;

	time(&timeout);
	timeout += stmt->wait_timeout;
	dstring_init(&query);

	while (!all_confirmed)
	{
		all_confirmed = 1;

		for (adminfo = script->adminfo_list;
			 adminfo; adminfo = adminfo->next)
		{
			/*
			 * If a specific event origin is given, skip all other nodes
			 * last_event.
			 */
			if (stmt->wait_origin > 0 && stmt->wait_origin != adminfo->no_id)
				continue;

			/*
			 * If we have not generated any event on that node, there is
			 * nothing to wait for.
			 */
			if (adminfo->last_event < 0)
			{
				if (adminfo->no_id == stmt->wait_origin)
				{
					printf("%s:%d: Error: "
					   "script did not generate any event on node %d\n",
							   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
							   stmt->wait_origin);
					return -1;
				}
				continue;
			}

			if (stmt->wait_confirmed < 0)
			{
				sprintf(seqbuf, INT64_FORMAT, adminfo->last_event);
				slon_mkquery(&query,
							 "select no_id, max(con_seqno) "
						   "    from \"_%s\".sl_node N, \"_%s\".sl_confirm C"
							 "    where N.no_id <> %d "
							 "    and N.no_active "
							 "    and N.no_id = C.con_received "
							 "    and C.con_origin = %d "
							 "    group by no_id "
							 "    having max(con_seqno) < '%s'; ",
							 stmt->hdr.script->clustername,
							 stmt->hdr.script->clustername,
							 adminfo->no_id, adminfo->no_id,
							 seqbuf);
			}
			else
			{
				sprintf(seqbuf, INT64_FORMAT, adminfo->last_event);
				slon_mkquery(&query,
							 "select no_id, max(con_seqno) "
						   "    from \"_%s\".sl_node N, \"_%s\".sl_confirm C"
							 "    where N.no_id = %d "
							 "    and N.no_active "
							 "    and N.no_id = C.con_received "
							 "    and C.con_origin = %d "
							 "    group by no_id "
							 "    having max(con_seqno) < '%s'; ",
							 stmt->hdr.script->clustername,
							 stmt->hdr.script->clustername,
							 stmt->wait_confirmed, adminfo->no_id,
							 seqbuf);
			}

			res = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
			if (res == NULL)
			{
				dstring_free(&query);
				return -1;
			}
			if (PQntuples(res) > 0)
				all_confirmed = 0;
			PQclear(res);

			if (!all_confirmed)
				break;
		}

		if (all_confirmed)
			break;

		/*
		 * Abort on timeout
		 */
		time(&now);
		if (stmt->wait_timeout > 0 && now > timeout)
		{
			printf("%s:%d: timeout exceeded while waiting for event confirmation\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
			dstring_free(&query);
			return -1;
		}

		sleep(1);
	}

	dstring_free(&query);

	return 0;
}


int
slonik_switch_log(SlonikStmt_switch_log * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".logswitch_start(); ",
				 stmt->hdr.script->clustername);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}


int
slonik_sync(SlonikStmt_sync * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "select \"_%s\".createEvent('_%s', 'SYNC'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername);
	if (db_exec_evcommand((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		dstring_free(&query);
		return -1;
	}

	dstring_free(&query);
	return 0;
}

int
slonik_sleep(SlonikStmt_sleep * stmt)
{
	sleep(stmt->num_secs);
	return 0;
}


/*
 * scanint8 --- try to parse a string into an int8.
 *
 * If errorOK is false, ereport a useful error message if the string is bad.
 * If errorOK is true, just return "false" for bad input.
 */
int
slon_scanint64(char *str, int64 * result)
{
	char	   *ptr = str;
	int64		tmp = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be broken
	 * for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char)*ptr))
		ptr++;

	/* handle sign */
	if (*ptr == '-')
	{
		ptr++;
		sign = -1;

		/*
		 * Do an explicit check for INT64_MIN.	Ugly though this is, it's
		 * cleaner than trying to get the loop below to handle it portably.
		 */
#ifndef INT64_IS_BUSTED
		if (strcmp(ptr, "9223372036854775808") == 0)
		{
			*result = -INT64CONST(0x7fffffffffffffff) - 1;
			return true;
		}
#endif
	}
	else if (*ptr == '+')
		ptr++;

	/* require at least one digit */
	if (!isdigit((unsigned char)*ptr))
		return false;

	/* process digits */
	while (*ptr && isdigit((unsigned char)*ptr))
	{
		int64		newtmp = tmp * 10 + (*ptr++ - '0');

		if ((newtmp / 10) != tmp)		/* overflow? */
			return false;
		tmp = newtmp;
	}

	/* trailing junk? */
	if (*ptr)
		return false;

	*result = (sign < 0) ? -tmp : tmp;

	return true;
}


/*
 * make a copy of the array of lines, with token replaced by replacement
 * the first time it occurs on each line.
 *
 * This does most of what sed was used for in the shell script, but
 * doesn't need any regexp stuff.
 */
static void
replace_token(char *resout, char *lines, const char *token, const char *replacement)
{
	int			numlines = 1;
	int			i,
				o;
	char		result_set[4096];
	int			toklen,
				replen;

	for (i = 0; lines[i]; i++)
		numlines++;

	toklen = strlen(token);
	replen = strlen(replacement);

	for (i = o = 0; i < numlines; i++, o++)
	{
		/* just copy pointer if NULL or no change needed */
		if (!lines[i] || (strncmp((const char *)lines + i, token, toklen)))
		{
			if (lines[i] == 0x0d)		/* ||(lines[i] == 0x0a)) */
				break;

			result_set[o] = lines[i];
			continue;
		}
		/* if we get here a change is needed - set up new line */
		strncpy((char *)result_set + o, replacement, replen);
		o += replen - 1;
		i += toklen - 1;
	}

	result_set[o] = '\0';
	memcpy(resout, result_set, o);
}


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
