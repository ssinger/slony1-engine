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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <alloca.h>
#else
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#define sleep(x) Sleep(x*1000)
#endif


#include "types.h"
#include "libpq-fe.h"
#include "slonik.h"


#ifdef MSVC
#include "config_msvc.h"
#else
#include "config.h"
#endif
#include "../parsestatements/scanner.h"
extern int	STMTS[MAXSTATEMENTS];


#ifdef HAVE_PGPORT
#undef USE_REPL_SNPRINTF
#include "port.h"
#endif

#define MAXPGPATH 256

/*
 * Global data
 */
SlonikScript *parser_script = NULL;
int			parser_errors = 0;
int			current_try_level;
int			block_stmt_no = 0;
int			last_event_node = -1;
int			auto_wait_disabled = 0;

static char share_path[MAXPGPATH];
#if HAVE_PGPORT
static char myfull_path[MAXPGPATH];
#endif




typedef struct
{
	int			no_id;
	SlonikAdmInfo *adminfo;
	int			has_slon;
	int			slon_pid;
	int			num_sets;
	int64		max_seqno;
	bool		failover_candidate;
}	failnode_node;


typedef struct
{
	int			set_id;
	int			num_subscribers;
	failnode_node **subscribers;
	failnode_node *max_node;
	int64		max_seqno;
	int			old_origin;
	int			backup_origin;
}	failnode_set;


typedef struct
{
	char	   *old_str;
	int			old_len;
	char	   *new_str;
}	replacement_token;


/*
 * Local functions
 */
static void usage(void);
static SlonikAdmInfo *get_adminfo(SlonikStmt * stmt, int no_id);
static SlonikAdmInfo *get_active_adminfo(SlonikStmt * stmt, int no_id);
static SlonikAdmInfo *get_checked_adminfo(SlonikStmt * stmt, int no_id);
static int	slonik_repair_config(SlonikStmt_repair_config * stmt);
static int	slonik_resubscribe_node(SlonikStmt_resubscribe_node * stmt);

static int	script_check(SlonikScript * script);
static int	script_check_adminfo(SlonikStmt * hdr, int no_id);
static int script_check_stmts(SlonikScript * script,
				   SlonikStmt * stmt);
static int	script_exec(SlonikScript * script);
static int script_exec_stmts(SlonikScript * script,
				  SlonikStmt * stmt);
static void script_commit_all(SlonikStmt * stmt,
				  SlonikScript * script);
static void script_rollback_all(SlonikStmt * stmt,
					SlonikScript * script);
static void script_disconnect_all(SlonikScript * script);
static void replace_tokens(SlonDString *dest, SlonDString *src, 
					replacement_token *replacements);
static int slonik_set_add_single_table(SlonikStmt_set_add_table * stmt,
							SlonikAdmInfo * adminfo1,
							const char *fqname);
static int	slonik_get_next_tab_id(SlonikStmt * stmt);
static int	slonik_get_next_sequence_id(SlonikStmt * stmt);
static int	find_origin(SlonikStmt * stmt, int set_id);
static int slonik_set_add_single_sequence(SlonikStmt * stmt,
							   SlonikAdmInfo * adminfo1,
							   const char *seq_name,
							   int set_id,
							   const char *seq_comment,
							   int seq_id);

static int slonik_add_dependent_sequences(SlonikStmt_set_add_table * stmt,
							   SlonikAdmInfo * adminfo1,
							   const char *table_name);
static int slonik_is_slony_installed(SlonikStmt * stmt,
						  SlonikAdmInfo * adminfo);
static int slonik_submitEvent(SlonikStmt * stmt,
				   SlonikAdmInfo * adminfo,
				   SlonDString * query,
				   SlonikScript * script,
				   int suppress_wait_for);

static size_t slonik_get_last_event_id(SlonikStmt * stmt,
						 SlonikScript * script,
						 const char *event_filter,
						 int64 **events);
static int slonik_wait_config_caughtup(SlonikAdmInfo * adminfo1,
							SlonikStmt * stmt,
							int ignore_node);
static int64 get_last_escaped_event_id(SlonikStmt * stmt,
						  int node_id,
						  int *skip_node_list);


static int fail_node_restart(SlonikStmt_failed_node * stmt,
				  failed_node_entry * node_entry,
				  failnode_node * nodeinfo);


static int
fail_node_promote(SlonikStmt_failed_node * stmt,
				  failed_node_entry * node_entry,
				  failnode_node * nodeinfo
				  ,int *fail_node_ids,
				  SlonDString * failed_node_list);

/* ----------
 * main
 * ----------
 */
int
main(int argc, const char *argv[])
{
	extern int	optind;
	int			opt;

	while ((opt = getopt(argc, (char **) argv, "hvw")) != EOF)
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
			case 'w':
				auto_wait_disabled = 1;
				break;

			default:
				printf("unknown option '%c'\n", opt);
				parser_errors++;
				break;
		}
	}

	if (parser_errors)
		usage();

#ifdef HAVE_PGPORT
	/*
	 * We need to find a share directory like PostgreSQL.
	 */
        if (find_my_exec(argv[0],myfull_path) < 0)
        {
                strcpy(share_path, PGSHARE);
        }
        else
        {
                get_share_path(myfull_path, share_path);
        }
#else
	strcpy(share_path, PGSHARE);
#endif

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
			current_file = (char *) argv[optind++];
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
				{
					SlonikStmt_exit *stmt =
					(SlonikStmt_exit *) hdr;

					if ((stmt->exitcode < 0) || (stmt->exitcode > 255))
					{
						printf("%s:%d: Error: exitcode was %d - must be in range [0-255]\n",
						  hdr->stmt_filename, hdr->stmt_lno, stmt->exitcode);
						errors++;
					}
				}
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
			case STMT_RESUBSCRIBE_NODE:
				{
					SlonikStmt_resubscribe_node *stmt =
					(SlonikStmt_resubscribe_node *) hdr;

					if (stmt->no_origin < 0)
					{
						printf("%s:%d: Error: require ORIGIN node\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->no_provider < 0)
					{

						printf("%s:%d: Error: require PROVIDER node\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->no_receiver < 0)
					{
						printf("%s:%d: Error: require RECEIVER node\n",
							   hdr->stmt_filename, hdr->stmt_lno);
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
					if (stmt->no_id_list == NULL ||
						stmt->no_id_list[0] == -1)
					{
						printf("%s:%d: Error: A node id must be provided",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					else
					{
						int			cnt;

						for (cnt = 0; stmt->no_id_list[cnt] != -1; cnt++)
						{
							if (stmt->no_id_list[cnt] == stmt->ev_origin)
							{
								printf("%s:%d: Error: "
									   "Node ID (%d) and event node cannot be identical\n",
									   hdr->stmt_filename, hdr->stmt_lno,
									   stmt->no_id_list[cnt]);
								errors++;
							}
						}

					}
					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;
				}
				break;

			case STMT_FAILED_NODE:
				{
					SlonikStmt_failed_node *stmt =
					(SlonikStmt_failed_node *) hdr;
					failed_node_entry *node = NULL;

					if (stmt->nodes == NULL)
					{
						printf("%s:%d: Error: require at least one failed node\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					for (node = stmt->nodes; node != NULL;
						 node = node->next)
					{
						if (node->backup_node < 0)
						{
							printf("%s:%d: Error: require BACKUP NODE\n",
								   hdr->stmt_filename, hdr->stmt_lno);
							errors++;
						}
						if (node->backup_node == node->no_id)
						{
							printf("%s:%d: Error: "
							 "Node ID and backup node cannot be identical\n",
								   hdr->stmt_filename, hdr->stmt_lno);
							errors++;
						}
						if (script_check_adminfo(hdr, node->backup_node) < 0)
							errors++;
					}
					/**
					 * todo: verify that one backup node isn't also
					 * a failing node.
					 */
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
					if (stmt->set_origin > 0)
					{
						if (script_check_adminfo(hdr, stmt->set_origin) < 0)
							errors++;
					}

					if (stmt->tab_fqname == NULL &&
						stmt->tables == NULL)
					{
						printf("%s:%d: Error: "
							   "'fully qualfied name' or 'tables' must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->tab_fqname != NULL &&
						stmt->tables != NULL)
					{
						printf("%s:%d: Error: "
						   "'fully qualified name' and 'tables' can not both"
							   " be specified", hdr->stmt_filename,
							   hdr->stmt_lno);
						errors++;
					}
					if (stmt->tables != NULL &&
						stmt->use_key != NULL)
					{

						printf("%s:%d: ERROR: "
							   "'key' can not be used with the 'tables' "
							   "option.", hdr->stmt_filename,
							   hdr->stmt_lno);
						errors++;
					}

					if (stmt->tab_comment == NULL && stmt->tab_fqname != NULL)
						stmt->tab_comment = strdup(stmt->tab_fqname);
					else if (stmt->tab_comment == NULL)
						stmt->tab_comment = strdup("replicated table");
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
					if (stmt->set_origin >= 0)
					{
						if (script_check_adminfo(hdr, stmt->set_origin) < 0)
							errors++;
					}

					if (stmt->seq_fqname == NULL &&
						stmt->sequences == NULL)
					{
						printf("%s:%d: Error: "
						 "sequence FQ-name or sequences must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->seq_fqname != NULL &&
						stmt->sequences != NULL)
					{
						printf("%s:%d: Error: "
						"'fully qualified name' and 'sequences' can not both"
							   " be specified", hdr->stmt_filename,
							   hdr->stmt_lno);
						errors++;
					}

					if (stmt->seq_comment == NULL &&
						stmt->seq_fqname != NULL)
						stmt->seq_comment = strdup(stmt->seq_fqname);
					else if (stmt->seq_comment == NULL)
						stmt->seq_comment = strdup("replicated sequence");
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
					else if (script_check_adminfo(hdr, stmt->set_origin) < 0)
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

					if (stmt->ev_origin < 0)
					{
						printf("%s:%d: Error: require EVENT NODE\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->ddl_fname == NULL && stmt->ddl_sql == NULL)
					{
						printf("%s:%d: Error: "
							   "script file name or sql must be specified\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if (stmt->ddl_fname != NULL && stmt->ddl_sql != NULL)
					{
						printf("%s:%d: Error: "
							   "script file name and sql are mutually exclusive\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}
					if ((stmt->only_on_node > 0) && (stmt->only_on_nodes != NULL))
					{
						printf("%s:%d: Error: "
						"cannot specify singular node as well as node list\n",
							   hdr->stmt_filename, hdr->stmt_lno);
						errors++;
					}

					if (script_check_adminfo(hdr, stmt->ev_origin) < 0)
						errors++;

					if (stmt->ddl_fname != NULL)
					{
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
	int64	   *events;
	size_t		event_length;
	int			idx = 0;
	SlonikAdmInfo *curAdmInfo;

	event_length = slonik_get_last_event_id(hdr, script, "ev_type <> 'SYNC' ",
											&events);
	for (curAdmInfo = script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
	{
		curAdmInfo->last_event = events[idx];
		idx++;
		if (idx > event_length)
			break;
	}
	free(events);
	block_stmt_no = 0;
	while (hdr && errors == 0)
	{
		hdr->script = script;
		block_stmt_no++;

		switch (hdr->stmt_type)
		{
			case STMT_TRY:
				{
					SlonikStmt_try *stmt =
					(SlonikStmt_try *) hdr;
					int			rc;
					int			saved_stmt_no;

					current_try_level++;
					saved_stmt_no = block_stmt_no;
					rc = script_exec_stmts(script, stmt->try_block);
					current_try_level--;
					block_stmt_no = saved_stmt_no;

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
					block_stmt_no--;
				}
				break;

			case STMT_DATE:
				{
					SlonikStmt_date *stmt =
					(SlonikStmt_date *) hdr;
					char		outstr[200];

					struct tm  *local;
					time_t		t;

					t = time(NULL);
					local = localtime(&t);
					strftime(outstr, sizeof(outstr), stmt->fmt, local);
					printf("%s:%d: %s\n",
						   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
						   outstr);
					block_stmt_no--;
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
			case STMT_RESUBSCRIBE_NODE:
				{
					SlonikStmt_resubscribe_node *stmt =
					(SlonikStmt_resubscribe_node *) hdr;

					if (slonik_resubscribe_node(stmt) < 0)
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

	for (adminfo = script->adminfo_list;
		 adminfo; adminfo = adminfo->next)
	{
		if (adminfo->dbconn != NULL && adminfo->have_xact)
		{
			db_commit_xact(stmt, adminfo);
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
	int			version;

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
	SlonDString file_content;
	SlonDString file_rewritten;
	va_list		ap;
	int			rc;
	char		fnamebuf[1024];
	char		buf[4096];
	size_t		num_read;
	FILE	   *stmtp;
	replacement_token	replacements[5];

	if (db_begin_xact(stmt, adminfo, true) < 0)
		return -1;

	/*
	 * Open the requested SQL file
	 */
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

	/*
	 * Initialize string objects and read the content of the SQL file
	 * into file_content.
	 */
	dstring_init(&file_content);
	dstring_init(&file_rewritten);
	while (!feof(stmtp))
	{
		num_read = fread(buf, 1, sizeof(buf), stmtp);
		if (num_read == 0)
			break;
		dstring_nappend(&file_content, buf, num_read);
	}
	fclose(stmtp);
	dstring_terminate(&file_content);

	/*
	 * Setup the string replacement table and call replace_tokens() to
	 * perform all the substitutions.
	 */
	replacements[0].old_str = "@CLUSTERNAME@";
	replacements[0].old_len = strlen(replacements[0].old_str);
	replacements[0].new_str = stmt->script->clustername;
	replacements[1].old_str = "@MODULEVERSION@";
	replacements[1].old_len = strlen(replacements[1].old_str);
	replacements[1].new_str = SLONY_I_VERSION_STRING;
	replacements[2].old_str = "@NAMESPACE@";
	replacements[2].old_len = strlen(replacements[2].old_str);
	replacements[2].new_str = alloca(strlen(stmt->script->clustername) + 4);
	sprintf(replacements[2].new_str, "\"_%s\"", stmt->script->clustername);
	replacements[3].old_str = "@FUNCVERSION@";
	replacements[3].old_len = strlen(replacements[3].old_str);
#define EXPAND2(x) #x
#define EXPAND(x) EXPAND2(x)
	replacements[3].new_str = EXPAND(SLONY_I_FUNC_VERSION_STRING);
#undef EXPAND
#undef EXPAND2
	replacements[4].old_str = NULL;
	replacements[4].old_len = 0;
	replacements[4].new_str = NULL;

	replace_tokens(&file_rewritten, &file_content, replacements);

	/*
	 * Execute the resulting SQL.
	 */
	res = PQexec(adminfo->dbconn, dstring_data(&file_rewritten));

	rc = PQresultStatus(res);
	if ((rc != PGRES_TUPLES_OK) && (rc != PGRES_COMMAND_OK) && (rc != PGRES_EMPTY_QUERY))
	{
		printf("%s:%d: loading of file %s: %s %s%s",
			   stmt->stmt_filename, stmt->stmt_lno,
			   fnamebuf, PQresStatus(rc),
			   PQresultErrorMessage(res),
			   PQerrorMessage(adminfo->dbconn));
		PQclear(res);
		dstring_free(&file_content);
		dstring_free(&file_rewritten);

		return -1;
	}
	PQclear(res);
	dstring_free(&file_content);
	dstring_free(&file_rewritten);

	return 0;
}


static int
load_slony_base(SlonikStmt * stmt, int no_id)
{
	SlonikAdmInfo *adminfo;
	int			rc;
	SlonDString query;

	int			use_major = 0;
	int			use_minor = 0;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return -1;

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
		(adminfo->pg_version / 10000), ((adminfo->pg_version % 10000) / 100));
		return -1;
	}
	else if ((adminfo->pg_version >= 80300) && (adminfo->pg_version < 80400))	/* 8.3 */
	{
		use_major = 8;
		use_minor = 3;
	}
	else if ((adminfo->pg_version >= 80400) && (adminfo->pg_version < 80500))	/* 8.4 */
	{
		use_major = 8;
		use_minor = 4;   
	}		
	else if ((adminfo->pg_version >= 90000) && (adminfo->pg_version < 90500)) /* 9.x */
	{
		/**
		 * 9.0 and 9.1 and 9.2, 9.3,9.4 are so far just like 8.4
		 **/
		use_major = 8;
		use_minor = 4;
	}
	else	/* above 9.4 ??? */
	{
		use_major = 8;
		use_minor = 4;
		printf("%s:%d: Possible unsupported PostgreSQL "
			   "version (%d) %d.%d, defaulting to 8.4 support\n",
			   stmt->stmt_filename, stmt->stmt_lno, adminfo->pg_version,
		(adminfo->pg_version / 10000), ((adminfo->pg_version % 10000) / 100));
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
			 "%s/slony1_base.%s.sql", share_path, SLONY_I_VERSION_STRING) < 0
		|| load_sql_script(stmt, adminfo,
						   "%s/slony1_base.v%d%d.%s.sql", share_path,
						   use_major, use_minor,
						   SLONY_I_VERSION_STRING) < 0
		|| load_sql_script(stmt, adminfo,
						   "%s/slony1_funcs.%s.sql", share_path,
						   SLONY_I_VERSION_STRING) < 0
		|| load_sql_script(stmt, adminfo,
					   "%s/slony1_funcs.v%d%d.%s.sql", share_path, use_major,
						   use_minor, SLONY_I_VERSION_STRING) < 0)
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


	int			use_major = 0;
	int			use_minor = 0;

	if ((adminfo = get_active_adminfo(stmt, no_id)) == NULL)
		return -1;


	/* determine what schema version we should load */

	if (adminfo->pg_version < 80300)	/* before 8.3 */
	{
		printf("%s:%d: unsupported PostgreSQL "
			   "version %d.%d\n",
			   stmt->stmt_filename, stmt->stmt_lno,
		(adminfo->pg_version / 10000), ((adminfo->pg_version % 10000) / 100));
		return -1;
	}
	else if ((adminfo->pg_version >= 80300) && adminfo->pg_version < 80400)		/* 8.0 */
	{
		use_major = 8;
		use_minor = 3;
	}
	else if ((adminfo->pg_version >= 80400) && (adminfo->pg_version < 80500))	/* 8.4 */
	{
		use_major = 8;
		use_minor = 4;
	}
	else if ((adminfo->pg_version >= 90000) && (adminfo->pg_version < 90500)) /* 9.0, 9.1, 9.2 */
	{
		/**
		 * 9.0 and 9.1 and 9.2 and 9.3,9.4 are so far just like 8.4
		 */
		use_major = 8;
		use_minor = 4;
	}
	else	/* above 8.4 */
	{
		use_major = 8;
		use_minor = 4;
		printf("%s:%d: Possible unsupported PostgreSQL "
			   "version (%d) %d.%d, defaulting to 8.4 support\n",
			   stmt->stmt_filename, stmt->stmt_lno, adminfo->pg_version,
		(adminfo->pg_version / 10000), ((adminfo->pg_version % 10000) / 100));
	}

	/* Load schema, DB version specific */
	db_notice_silent = true;
	if (load_sql_script(stmt, adminfo,
						"%s/slony1_funcs.%s.sql",
						share_path, SLONY_I_VERSION_STRING) < 0
		|| load_sql_script(stmt, adminfo,
					   "%s/slony1_funcs.v%d%d.%s.sql", share_path, use_major,
						   use_minor, SLONY_I_VERSION_STRING) < 0)
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
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".updateReloid(%d, %d); ",
				 stmt->hdr.script->clustername,
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo, true) < 0)
		return -1;

	rc = load_slony_base((SlonikStmt *) stmt, stmt->no_id);
	if (rc < 0)
		return -1;

	/* call initializeLocalNode() and enableNode() */
	dstring_init(&query);
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".initializeLocalNode(%d, '%q'); "
				 "select \"_%s\".enableNode(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
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

	if (!auto_wait_disabled)
	{
		rc = slonik_wait_config_caughtup(adminfo2, &stmt->hdr, stmt->no_id);
		if (rc < 0)
			return rc;
	}

	if (db_begin_xact((SlonikStmt *) stmt, adminfo2, false) < 0)
		return -1;

	dstring_init(&query);

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, true) < 0)
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
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".initializeLocalNode(%d, '%q'); "
				 "select \"_%s\".enableNode_int(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
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
					 "lock table \"_%s\".sl_config_lock;"
					 "select \"_%s\".storePath_int(%s, %s, '%q', %s); ",
					 stmt->hdr.script->clustername,
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
					 "lock table \"_%s\".sl_config_lock;"
				  "select \"_%s\".subscribeSet_int(%s, %s, %s, '%q', 'f'); ",
					 stmt->hdr.script->clustername,
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

		if (stmt->no_id == (int) strtol(ev_origin_c, NULL, 10))
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
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo2, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	SlonikAdmInfo *adminfo2;
	SlonDString query;
	SlonDString node_list;
	SlonikAdmInfo *curAdmInfo;
	int			rc;
	int			no_id_idx;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	for (no_id_idx = 0; stmt->no_id_list[no_id_idx] != -1; no_id_idx++)
	{
		int64		ev_id;

		if (!auto_wait_disabled)
		{
			for (curAdmInfo = stmt->hdr.script->adminfo_list;
				 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
			{
				int			skip = 0;
				int			list_idx;
				SlonikAdmInfo *fake_admin_info = NULL;

				/**
				 * If we have admin info for any of the nodes being dropped
				 * we disable 'wait for' on that node.
				 */
				for (list_idx = 0; stmt->no_id_list[list_idx] != -1; list_idx++)
				{


					if (curAdmInfo->no_id == stmt->no_id_list[list_idx])
					{
						skip = 1;
						break;
					}
				}
				if (skip)
					continue;
				/**
				 * find the last event (including SYNC events)
				 * from the node being dropped that is visible on
				 * any of the remaining nodes.
				 * we must wait for ALL remaining nodes to get caught up.
				 *
				 * we can't ignore SYNC events even though the dropped
				 * node is not an origin it might have been an old
				 * origin before a FAILOVER. Some behind node still
				 * might need to get caught up from its provider.
				 */
				ev_id = get_last_escaped_event_id((SlonikStmt *) stmt,
												  stmt->no_id_list[no_id_idx],
												  stmt->no_id_list);
				if (ev_id > 0)
				{
					SlonikStmt_wait_event wait_event;

					wait_event.hdr = *(SlonikStmt *) stmt;
					wait_event.wait_origin = stmt->no_id_list[no_id_idx];
					wait_event.wait_on = stmt->ev_origin;
					wait_event.wait_confirmed = stmt->ev_origin;
					wait_event.wait_timeout = 0;
					wait_event.ignore_nodes = 0;

					adminfo2 = get_adminfo(&stmt->hdr, stmt->no_id_list[no_id_idx]);
					if (adminfo2 == NULL)
					{
						fake_admin_info = malloc(sizeof(SlonikAdmInfo));
						memset(fake_admin_info, 0, sizeof(SlonikAdmInfo));
						fake_admin_info->next = stmt->hdr.script->adminfo_list->next;
						stmt->hdr.script->adminfo_list->next = fake_admin_info;

					}
					else
						adminfo2->last_event = ev_id;

					printf("debug: waiting for %d," INT64_FORMAT " on %d\n",
						   wait_event.wait_origin, ev_id, wait_event.wait_on);
					rc = slonik_wait_event(&wait_event);
					if (rc < 0)
					{
						return rc;
					}
					if (fake_admin_info != NULL)
					{
						stmt->hdr.script->adminfo_list->next = fake_admin_info->next;
						free(fake_admin_info);
					}

				}
#if 0 
				if (slonik_is_slony_installed((SlonikStmt *) stmt, curAdmInfo) > 0)
				{
					rc = slonik_wait_config_caughtup(curAdmInfo, (SlonikStmt *) stmt,
													 -1);
					if (rc < 0)
						return rc;
				}
#endif
			}

		}

	}


	dstring_init(&query);
	dstring_init(&node_list);
	slon_mkquery(&node_list,"%d",stmt->no_id_list[0]);
	for (no_id_idx = 1; stmt->no_id_list[no_id_idx] != -1; no_id_idx++)
	{
		slon_appendquery(&node_list,",%d",stmt->no_id_list[no_id_idx]);
		
	}
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".dropNode(ARRAY[%s]); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 dstring_data(&node_list));
	
	/**
	 * we disable auto wait because we perform a wait
	 * above ignoring the node being dropped.
	 */
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, true) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	for (no_id_idx = 0; stmt->no_id_list[no_id_idx] != -1; no_id_idx++)
	{	   
		/**
		 * if we have a conninfo for the node being dropped
		 * we want to clear out the last seqid.
		 */
		adminfo2 = get_adminfo(&stmt->hdr, stmt->no_id_list[no_id_idx]);
		if (adminfo2 != NULL)
		{
			adminfo2->last_event = -1;
		}
	}
	dstring_free(&node_list);
	dstring_free(&query);
	return 0;
}



int
slonik_failed_node(SlonikStmt_failed_node * stmt)
{
	SlonikAdmInfo *adminfo1;
	SlonDString query;
	SlonDString failed_node_list;

	int			i;
	int			num_origins = 0;
	int			cur_origin_idx = 0;
	char	  **failnodebuf;
	int		  **set_list = 0;
	PGresult   *res1;
	PGresult   *res2;
	PGresult   *res3;
	int64	   *max_seqno_total = 0;
	failnode_node **max_node_total = NULL;
	failed_node_entry *node_entry = stmt->nodes;
	int		   *fail_node_ids = NULL;
	bool         missing_paths=false;
	int			rc = 0;


	/**
	 *	The failover procedure (at a high level) is as follows
	 *
	 * 1. Check to see if the failover will leave orphan nodes
	 * 2. Get a list of failover candidates for each failed node.
	 * 3. validate that we have conninfo to all of them
	 * 4. blank their paths to the failed nodes
	 * 5. Wait for slons to restart
	 * 6. for each failed node get the highest xid for each candidate
	 * 7. execute FAILOVER on the highest canidate
	 * 8. MOVE SET to the backup node.
	 */


	dstring_init(&failed_node_list);
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next)
	{
		if (node_entry == stmt->nodes)
			slon_appendquery(&failed_node_list, "%d", node_entry->no_id);
		else
			slon_appendquery(&failed_node_list, ",%d", node_entry->no_id);
		num_origins++;
	}
	dstring_terminate(&failed_node_list);



	/**
	 * peform some memory allocations
	 */
	dstring_init(&query);
	failnodebuf = (char **) malloc(sizeof(char *) * num_origins);
	set_list = (int **) malloc(sizeof(int *) * num_origins);
	max_seqno_total = (int64 *) malloc(sizeof(int64) * num_origins);
	max_node_total = (failnode_node **) malloc(sizeof(failnode_node *) *
											   num_origins);
	fail_node_ids = (int *) malloc(sizeof(int) * num_origins + 1);
	memset(failnodebuf, 0, sizeof(char *) * num_origins);
	memset(max_node_total, 0, sizeof(failnode_node *) * num_origins);
	memset(max_seqno_total, 0, sizeof(int64) * num_origins);
	memset(fail_node_ids, -1, sizeof(int) * (num_origins + 1));
	memset(set_list, 0, sizeof(int *) * num_origins);


	/**
	 * validate that the list of failed nodes is complete.
	 * This means that for any providers in sl_subscribe 
	 * that have failed  must have  a path between
	 * the receiver node and the specified backup node.
	 *
	 * If this isn't the case then the user must also include
	 * the receiver node as a failed node, or add in the path
	 * before calling FAILOVER.
	 *
	 * We only check sl_subscribe on arbitrary backup node
	 */
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{
		int num_orphans=0;
		int res_idx;
		slon_mkquery(&query,
					 " select sub_receiver FROM \"_%s\".sl_subscribe left join "
					 "\"_%s\".sl_path on (pa_server="
					 " %d and pa_client=sub_receiver) where sub_receiver not "
					 "in (%s) and sub_provider=%d and sub_receiver <> %d and pa_client is null"
					 , stmt->hdr.script->clustername
					 , stmt->hdr.script->clustername
					 , node_entry->backup_node
					 ,  dstring_data(&failed_node_list)
					 ,  node_entry->no_id,node_entry->backup_node);
		adminfo1 = get_active_adminfo((SlonikStmt *) stmt, node_entry->backup_node);
		if (adminfo1 == NULL)
		{
			printf("%s:%d no admin conninfo for node %d\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   node_entry->backup_node);
			rc = -1;
			goto cleanup;
		}			 
		res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res1 == NULL)
		{
			rc = -1;
			goto cleanup;
		}
		num_orphans = PQntuples(res1);
		for(res_idx=0; res_idx < num_orphans; res_idx++)
		{
			char * receiver=PQgetvalue(res1,res_idx,0);
			printf("%s:%d can't failover node %s does not have a path to %d\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,receiver,
				   node_entry->backup_node);
			missing_paths=true;
				   

		}
		PQclear(res1);
		
		/**
		 * make sure that the backup_node is a forwarding subscriber of ALL
		 * sets the the failed node is an origin for.
		 */
		slon_mkquery(&query,"select set.set_id FROM \"_%s\".sl_set set " \
					 " left outer join \"_%s\".sl_subscribe sub on (" \
					 "    set.set_id=sub.sub_set and sub.sub_receiver=%d " \
					 "    and sub.sub_forward=true) " \
					 " where sub.sub_set is null and set.set_origin=%d "
					 , stmt->hdr.script->clustername
					 , stmt->hdr.script->clustername
					 , node_entry->backup_node
					 , node_entry->no_id);
		
		res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res1 == NULL)
		{
			rc = -1;
			goto cleanup;
		}
		if(PQntuples(res1)!=0) 
		{
			printf("%s:%d node %d is not a forwarding subscriber of set %s " \
				   "from node %d \n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   node_entry->backup_node,PQgetvalue(res1,0,0),
				   node_entry->no_id);
			missing_paths=true;
		}
		PQclear(res1);


	}
	if ( missing_paths )
	{
		rc=-1;
		goto cleanup;

	}


	/**
	 * get the list of failover candidates for each of the
	 * failed nodes.
	 */
	cur_origin_idx = 0;
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{
		failnode_node *nodeinfo;
		bool		has_candidate = false;

		adminfo1 = get_active_adminfo((SlonikStmt *) stmt, node_entry->backup_node);
		if (adminfo1 == NULL)
		{
			printf("%s:%d no admin conninfo for node %d\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   node_entry->backup_node);
			rc = -1;
			goto cleanup;
		}

	

		if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		{
			printf("%s:%d can not connect to node %d\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   node_entry->backup_node);
			rc = -1;
			goto cleanup;
		}

		fail_node_ids[cur_origin_idx] = node_entry->no_id;


	

	
		/*
		 * On the backup node select a list of all failover candidate nodes
		 * except for the failed nodes.
		 */
		slon_mkquery(&query,
					 "select distinct no_id, backup_id from "
					 " \"_%s\".sl_node left join \"_%s\".sl_failover_targets"
					 "    on (sl_node.no_id=sl_failover_targets.backup_id "
					 "        and set_origin=%d )"
					 "    where no_id not in ( %s ) "
					 "    and ( backup_id not in ( %s ) or backup_id is null) "
					 "    order by no_id; ",
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 node_entry->no_id,
					 dstring_data(&failed_node_list),
					 dstring_data(&failed_node_list));
		res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res1 == NULL)
		{
			rc = -1;
			goto cleanup;
		}
		node_entry->num_nodes = PQntuples(res1);


		/*
		 * Get a list of all sets that are subscribed more than once directly
		 * from the origin
		 */
		slon_mkquery(&query,
					 "select S.set_id, count(S.set_id) "
					 "    from \"_%s\".sl_set S, \"_%s\".sl_subscribe SUB "
					 "    where S.set_id = SUB.sub_set "
					 "    and S.set_origin = %d "
					 "    and SUB.sub_active "
					 "    group by set_id ",
					 stmt->hdr.script->clustername,

					 stmt->hdr.script->clustername,
					 node_entry->no_id);
		res2 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res2 == NULL)
		{
			PQclear(res1);
			rc = -1;
			goto cleanup;
		}
		node_entry->num_sets = PQntuples(res2);

		/*
		 * Allocate and initialize memory to hold some config info
		 */
		failnodebuf[cur_origin_idx] = malloc(sizeof(failnode_node)
											 * (node_entry->num_nodes
												+ node_entry->num_sets
												* node_entry->num_nodes));
		memset(failnodebuf[cur_origin_idx], 0, sizeof(failnode_node)
			   * (node_entry->num_nodes + (node_entry->num_sets
										   * node_entry->num_nodes)));
		set_list[cur_origin_idx] = malloc(sizeof(int) * node_entry->num_sets);
		memset(set_list[cur_origin_idx], 0,
			   sizeof(int) * node_entry->num_sets);
		nodeinfo = (failnode_node *) failnodebuf[cur_origin_idx];

		for (i = 0; i < PQntuples(res2); i++)
		{
			set_list[cur_origin_idx][i] = (int) strtol(PQgetvalue(res2, i, 0),
													   NULL, 10);
		}

		/*
		 * Connect to all these nodes and determine if there is a node daemon
		 * running on that node.
		 */

		for (i = 0; i < node_entry->num_nodes; i++)
		{
			const char * pidcolumn;
			has_candidate = true;
			nodeinfo[i].no_id = (int) strtol(PQgetvalue(res1, i, 0), NULL, 10);
			nodeinfo[i].adminfo = get_active_adminfo((SlonikStmt *) stmt,
													 nodeinfo[i].no_id);
			if (nodeinfo[i].adminfo == NULL)
			{
				printf("%s:%d error no conninfo for candidate for %d\n",
					   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno
					   ,nodeinfo[i].no_id);
				PQclear(res1);
				rc = -1;
				goto cleanup;
			}

			if (! PQgetisnull(res1, i, 1) )
			{
				nodeinfo[i].failover_candidate = true;
			}
			else
				nodeinfo[i].failover_candidate = false;
			
			if (nodeinfo[i].adminfo->pg_version >= 90200)  
				pidcolumn="pid";
			else 
				pidcolumn="procpid";
			slon_mkquery(&query,
						 "lock table \"_%s\".sl_config_lock; "
						 "select nl_backendpid from \"_%s\".sl_nodelock "
				   "    where nl_nodeid = \"_%s\".getLocalNodeId('_%s') and "
				  "       exists (select 1 from pg_catalog.pg_stat_activity "
						 "                 where %s = nl_backendpid);",
						 stmt->hdr.script->clustername,
						 stmt->hdr.script->clustername,
						 stmt->hdr.script->clustername,
						 stmt->hdr.script->clustername,
						 pidcolumn);
			res3 = db_exec_select((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query);
			if (res3 == NULL)
			{
				PQclear(res1);
				PQclear(res2);
				rc = -1;
				goto cleanup;
			}
			if (PQntuples(res3) == 0)
			{
				nodeinfo[i].has_slon = false;
				nodeinfo[i].slon_pid = 0;
			}
			else
			{
				nodeinfo[i].has_slon = true;
				nodeinfo[i].slon_pid = (int) strtol(PQgetvalue(res3, 0, 0), NULL, 10);
			}
			PQclear(res3);
		}
		PQclear(res1);
		PQclear(res2);
		if (!has_candidate && node_entry->num_sets > 0 )
		{
			printf("%s:%d error no failover candidates for %d\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno
				   ,node_entry->no_id);
			rc = -1;
			goto cleanup;
		}
	

		/*
		 * Execute the preFailover() procedure on all failover candidate nodes
		 * to stop them from receiving new messages from the failed node.
		 */

		for (i = 0; i < node_entry->num_nodes; i++)
		{
			printf("executing preFailover(%d,%d) on %d\n",
				   node_entry->no_id,
				   nodeinfo[i].failover_candidate,
				   nodeinfo[i].no_id);
			slon_mkquery(&query,
						 "lock table \"_%s\".sl_config_lock; "
						 "select \"_%s\".preFailover(%d,%s); ",
						 stmt->hdr.script->clustername,
						 stmt->hdr.script->clustername,
						 node_entry->no_id, nodeinfo[i].failover_candidate ? "true" : "false");
			if (db_exec_command((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query) < 0)
			{
				rc = -1;
				goto cleanup;
			}
			if (db_commit_xact((SlonikStmt *) stmt, nodeinfo[i].adminfo) < 0)
			{
				rc = -1;
				goto cleanup;
			}
		}

	}

	/*
	 * Wait until all slon replication engines that were running have
	 * restarted.
	 */
	cur_origin_idx = 0;
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{
		failnode_node *nodeinfo = (failnode_node *) failnodebuf[cur_origin_idx];

		rc = fail_node_restart(stmt, node_entry, nodeinfo);
		if (rc < 0)
		{
			goto cleanup;
		}

	}

	/**
	 * promote the most ahead node to be the new (temporary) origin
	 * for each of the failed nodes.
	 */
	cur_origin_idx = 0;
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{

		
		failnode_node *nodeinfo = (failnode_node *) failnodebuf[cur_origin_idx];
		
		if ( node_entry->num_nodes == 0 )
			continue;

		rc = fail_node_promote(stmt, node_entry, nodeinfo,
							   fail_node_ids,&failed_node_list);
		if (rc < 0)
		{
			goto cleanup;
		}
	}


	/**
	 * MOVE SET to move the sets to the desired origin.
	 */
	cur_origin_idx = 0;
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{
		SlonikStmt_lock_set lock_set;
		SlonikStmt_move_set move_set;
		SlonikStmt_wait_event wait_event;
		int wait_rc=0;
		if (node_entry->temp_backup_node == node_entry->backup_node)
			continue;
		lock_set.hdr = stmt->hdr;
		lock_set.set_origin = node_entry->temp_backup_node;
		for (i = 0; i < node_entry->num_sets; i++)
		{
			lock_set.set_id = set_list[cur_origin_idx][i];
			if (slonik_lock_set(&lock_set) < 0)
			{
				printf("%s:%d error locking set %d on %d for MOVE SET\n",
					   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
					   lock_set.set_id, lock_set.set_origin);
				rc=-1;
				continue;
			}
			move_set.hdr = stmt->hdr;
			move_set.old_origin = node_entry->temp_backup_node;
			move_set.new_origin = node_entry->backup_node;
			move_set.set_id = set_list[cur_origin_idx][i];
			if (slonik_move_set(&move_set) < 0)
			{
				printf("%s:%d error moving set %d on %d\n",
					   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
					   lock_set.set_id, lock_set.set_origin);
				rc=-1;
				continue;
			}
			/**
			 * now wait until the MOVE_SET completes.
			 * FAILOVER is not finished until backup_node is the
			 * origin.
			 */
			wait_event.hdr = *(SlonikStmt *) stmt;
			wait_event.wait_origin = node_entry->temp_backup_node;
			wait_event.wait_on = node_entry->temp_backup_node;
			wait_event.wait_confirmed = node_entry->backup_node;
			wait_event.wait_timeout = 0;
			wait_event.ignore_nodes = fail_node_ids;
			adminfo1 = get_active_adminfo((SlonikStmt *) stmt,
										  node_entry->temp_backup_node);
			if (db_commit_xact((SlonikStmt *) stmt,
							   adminfo1) < 0)
			{
				rc = -1;
				goto cleanup;
			}
			wait_rc = slonik_wait_event(&wait_event);
			if (wait_rc < 0)
			{
				/**
				 * pretty serious? how do we recover?
				 */
				printf("%s:%d error waiting for event\n",
					   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
				rc=wait_rc;
			}
			/**
			 * commit the transaction - in case the WAIT FOR
			 * above created one.
			 */
			if (db_commit_xact((SlonikStmt *) stmt,
							   adminfo1) < 0)
			{
				rc = -1;
				goto cleanup;
			}

		}
	}



cleanup:
	cur_origin_idx = 0;
	for (node_entry = stmt->nodes; node_entry != NULL;
		 node_entry = node_entry->next, cur_origin_idx++)
	{
		if (failnodebuf[cur_origin_idx])
			free(failnodebuf[cur_origin_idx]);
		if (set_list[cur_origin_idx])
			free(set_list[cur_origin_idx]);
	}
	free(failnodebuf);
	free(set_list);
	free(max_seqno_total);
	free(max_node_total);
	free(fail_node_ids);
	dstring_free(&query);
	return rc;
}

/**
 * A helper function used during the failover process.
 * This function will check to see which nodes need to have there
 * slons restarted.
 */
static int
fail_node_restart(SlonikStmt_failed_node * stmt,
				  failed_node_entry * node_entry,
				  failnode_node * nodeinfo)
{
	int			n = 0;
	int			i = 0;
	SlonDString query;
	PGresult   *res1;

	dstring_init(&query);

	while (n < node_entry->num_nodes)
	{
		sleep(1);
		n = 0;
		for (i = 0; i < node_entry->num_nodes; i++)
		{
			if (!nodeinfo[i].has_slon)
			{
				n++;
				continue;
			}

			slon_mkquery(&query,
						 "select nl_backendpid from \"_%s\".sl_nodelock "
						 "    where nl_backendpid <> %d "
						 "    and nl_nodeid = \"_%s\".getLocalNodeId('_%s');",
						 stmt->hdr.script->clustername,
						 nodeinfo[i].slon_pid,
						 stmt->hdr.script->clustername,
						 stmt->hdr.script->clustername
				);
			res1 = db_exec_select((SlonikStmt *) stmt, nodeinfo[i].adminfo, &query);
			if (res1 == NULL)
			{
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
				dstring_free(&query);
				return -1;
			}
		}
	}
	dstring_free(&query);
	return 0;
}


/**
 * A helper function used during the failover process.
 * This function will promote the most-ahead failover candidate
 * to be the new (at least temporary) set origin.
 */
int
fail_node_promote(SlonikStmt_failed_node * stmt,
				  failed_node_entry * node_entry,
				  failnode_node * nodeinfo,
				  int *fail_node_ids,
	              SlonDString * failed_node_list)
{
	int64		max_seqno = 0;
	int			max_node_idx = -1;
	int			backup_idx = 0;
	char		ev_seqno_c[64];
	SlonDString query;
	int			rc = 0;
	int			i;
	PGresult   *res1;
	SlonikAdmInfo *adminfo1;
	SlonikStmt_wait_event wait_event;
	int64 backup_node_seqno = 0;
	
	dstring_init(&query);
	

	/*
	 * For every node determine the one with the event , preferring the backup
	 * node.
	 */
	for (i = 0; i < node_entry->num_nodes; i++)
	{

		int64		ev_seqno;

		//if (!nodeinfo[i].failover_candidate)
		//	continue;
		
		slon_mkquery(&query,
					 "select max(ev_seqno) "
					 "	from \"_%s\".sl_event "
					 "	where ev_origin = %d; ",
					 stmt->hdr.script->clustername,
					 node_entry->no_id);
		res1 = db_exec_select((SlonikStmt *) stmt,
							  nodeinfo[i].adminfo, &query);
		if (res1 == NULL)
		{
			rc=-1;
			goto cleanup;
		}
		slon_scanint64(PQgetvalue(res1, 0, 0), &ev_seqno);
		if (nodeinfo[i].no_id == node_entry->backup_node) 
		{
			backup_idx = i;
			backup_node_seqno = ev_seqno;
		}
		nodeinfo[i].max_seqno = ev_seqno;

		if (nodeinfo[i].max_seqno > max_seqno  && nodeinfo[i].failover_candidate )
		{
			max_seqno = nodeinfo[i].max_seqno;
			max_node_idx = i;
		}
		PQclear(res1);

	}
	if( max_node_idx == -1)
	{
		/**
		 * no maximum ahead node was found. 
		 */
	}

	if (nodeinfo[max_node_idx].no_id != node_entry->backup_node)
	{
		if (nodeinfo[max_node_idx].max_seqno ==
			nodeinfo[backup_idx].max_seqno)
			max_node_idx = backup_idx;
	}


	

	/*
	 * Now execute all FAILED_NODE events on the most ahead candidate
	 * 
	 * If there is no failover candiate we use the requested backup node.
	 */
	if(max_node_idx < 0) 
	{
		max_node_idx = backup_idx;
		max_seqno = backup_node_seqno;
	}
	adminfo1 = nodeinfo[max_node_idx].adminfo;
	sprintf(ev_seqno_c, INT64_FORMAT, max_seqno);
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".failedNode2(%d,%d,'%s',ARRAY[%s]); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 node_entry->no_id, nodeinfo[max_node_idx].no_id
				 ,ev_seqno_c,dstring_data(failed_node_list));
	printf("NOTICE: executing \"_%s\".failedNode2 on node %d\n",
		   stmt->hdr.script->clustername,
		   nodeinfo[max_node_idx].no_id);
	node_entry->temp_backup_node = nodeinfo[max_node_idx].no_id;

	if (db_exec_evcommand((SlonikStmt *) stmt,
						  adminfo1,
						  &query) < 0)
	{
		rc = -1;
		goto cleanup;
	}
	/**
	 * now wait for the FAILOVER_NODE event to be confirmed
	 * by all nodes
	 */

	/*
	 * commit all open transactions despite of all possible errors
	 */
	for (i = 0; i < node_entry->num_nodes; i++)
	{
		if (db_commit_xact((SlonikStmt *) stmt,
						   nodeinfo[i].adminfo) < 0)
		{
			rc = -1;
			goto cleanup;
		}
	}


	wait_event.hdr = *(SlonikStmt *) stmt;
	wait_event.wait_origin = nodeinfo[max_node_idx].no_id;
	wait_event.wait_on = nodeinfo[max_node_idx].no_id;;
	wait_event.wait_confirmed = -1;
	wait_event.wait_timeout = 0;
	wait_event.ignore_nodes = fail_node_ids;

	if (db_commit_xact((SlonikStmt *) stmt,
					   adminfo1) < 0)
	{
		rc = -1;
		goto cleanup;
	}
	rc = slonik_wait_event(&wait_event);
	if (rc < 0)
	{
		/**
		 * pretty serious? how do we recover?
		 */
		printf("%s:%d error waiting for event\n",
			   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
	}

	/**
	 * now failedNod3e on the temp backup node.
	 */
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".failedNode3(%d,%d,'%s'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 node_entry->no_id, nodeinfo[max_node_idx].no_id
				 ,ev_seqno_c);
	printf("NOTICE: executing \"_%s\".failedNode3 on node %d\n",
		   stmt->hdr.script->clustername,
		   nodeinfo[max_node_idx].no_id);

	if (db_exec_evcommand((SlonikStmt *) stmt,
						  adminfo1,
						  &query) < 0)
	{
		rc = -1;
		goto cleanup;
	}

cleanup:

	/*
	 * commit all open transactions despite of all possible errors
	 */
	for (i = 0; i < node_entry->num_nodes; i++)
	{
		if (db_commit_xact((SlonikStmt *) stmt,
						   nodeinfo[i].adminfo) < 0)
			rc = -1;
	}

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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".uninstallNode(); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
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

	/**
	 * if we have a conninfo for the node being uninstalled
	 * we want to clear out the last seqid.
	 */
	if (adminfo1 != NULL)
	{
		adminfo1->last_event = -1;
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
	int			rc;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_provider);
	if (adminfo1 == NULL)
		return -1;

	if (!auto_wait_disabled)
	{
		rc = slonik_wait_config_caughtup(adminfo1, &stmt->hdr, -1);
		if (rc < 0)
			return rc;
	}

	dstring_init(&query);

	if (stmt->no_comment == NULL)
		slon_mkquery(&query,
					 "lock table \"_%s\".sl_config_lock; "
					 "select \"_%s\".cloneNodePrepare(%d, %d, 'Node %d'); ",
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 stmt->no_id, stmt->no_provider,
					 stmt->no_id);
	else
		slon_mkquery(&query,
					 "lock table \"_%s\".sl_config_lock; "
					 "select \"_%s\".cloneNodePrepare(%d, %d, '%q'); ",
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 stmt->no_id, stmt->no_provider,
					 stmt->no_comment);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".storePath(%d, %d, '%q', %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->pa_server, stmt->pa_client,
				 stmt->pa_conninfo, stmt->pa_connretry);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, 1) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".dropPath(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->pa_server, stmt->pa_client);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".storeListen(%d, %d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->li_origin, stmt->li_provider,
				 stmt->li_receiver);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".dropListen(%d, %d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->li_origin, stmt->li_provider,
				 stmt->li_receiver);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	SlonikAdmInfo *curAdmInfo;
	int64	   *drop_set_events;
	int64	   *cached_events;
	size_t		event_size;
	int			adm_idx;
	int			rc;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;

	if (!auto_wait_disabled)
	{
		/**
		 * loop through each node and make sure there are no
		 * pending DROP SET commands.
		 *
		 * if there is a DROP SET command from the node
		 * in sl_event then we wait until all other nodes are
		 * caughtup to that.
		 *
		 */
		event_size = slonik_get_last_event_id((SlonikStmt *) stmt,
											  stmt->hdr.script,
											  "ev_type='DROP_SET'",
											  &drop_set_events);

		/**
		 * copy the 'real' last event to storage
		 * and update the AdmInfo structure with the last 'DROP SET' id.
		 */
		cached_events = malloc(sizeof(int64) * event_size);
		adm_idx = 0;
		for (curAdmInfo = stmt->hdr.script->adminfo_list;
			 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
		{
			cached_events[adm_idx] = curAdmInfo->last_event;
			curAdmInfo->last_event = drop_set_events[adm_idx];
			adm_idx++;
		}
		rc = slonik_wait_config_caughtup(adminfo1, &stmt->hdr, -1);
		if (rc < 0)
			return rc;
		/***
		 * reset the last_event values in the AdmInfo to
		 * the values we saved above.
		 */
		adm_idx = 0;
		for (curAdmInfo = stmt->hdr.script->adminfo_list;
			 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
		{
			curAdmInfo->last_event = cached_events[adm_idx];
			adm_idx++;
		}
		free(cached_events);
		free(drop_set_events);

	}

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	if (stmt->set_comment == NULL)
		comment = CREATESET_DEFCOMMENT;
	else
		comment = stmt->set_comment;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".storeSet(%d, '%q'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->set_id, comment);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".dropSet(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->set_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	PGresult   *res;
	bool		in_progress = 1;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->set_origin);
	if (adminfo1 == NULL)
		return -1;


	/**
	 * The event node (the origin) should be caught up
	 * with itself before submitting a merge set.
	 * this ensures no subscriptions involving the set
	 * are still in progress.
	 *
	 * (we could also check for the event number of any
	 * unconfirmed subscriptions and wait for that
	 * but we don't)
	 */



	dstring_init(&query);

	slon_mkquery(&query, "select \"_%s\".isSubscriptionInProgress(%d)"
				 " or \"_%s\".isSubscriptionInProgress(%d)"
				 ,stmt->hdr.script->clustername,
				 stmt->add_id, stmt->hdr.script->clustername,
				 stmt->set_id);

	while (in_progress)
	{
		char	   *result;

		if (current_try_level > 0)
		{
			if (current_try_level == 1 && block_stmt_no == 1)
			{
				/**
				 * on the first command of the try block we can
				 * still a abort the txn and restart it later,
				 * after the wait for has been complete.
				 */
			}
			else
			{
				printf("%s:%d Error: a subscription is in progress. "
					   "slonik can not wait for it to finish inside of a "
				   "try block", stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
				return -1;
			}
		}

		if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
			return -1;

		res = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (res == NULL)
		{
			dstring_free(&query);
			return -1;

		}
		result = PQgetvalue(res, 0, 0);
		if (result != NULL && (*result == 't' ||
							   *result == 'T'))
		{
			printf("%s:%d subscription in progress before mergeSet. waiting\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
			fflush(stdout);
			db_rollback_xact((SlonikStmt *) stmt, adminfo1);
			sleep(5);
		}
		else
			in_progress = false;
		if (result != NULL)
			PQclear(res);
	}

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".mergeSet(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->add_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	int			origin = stmt->set_origin;
	SlonDString query;
	PGresult   *result;
	int			idx;
	char	   *table_name;
	int			rc;

	if (stmt->set_origin < 0)
	{
		origin = find_origin((SlonikStmt *) stmt, stmt->set_id);
		if (origin < 0)
		{

			printf("%s:%d:Error: unable to determine the origin for set %d",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno, stmt->set_id);
			return -1;
		}
	}

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, origin);
	if (adminfo1 == NULL)
		return -1;

	/**
	 * make sure that the origin node is caught up with
	 * the last event node *before* opening the transaciton
	 * the 'set add table' will take place in.
	 *
	 * Once that transaction is opened and has a lock
	 * on sl_config_lock the slon for the node	won't be
	 * able to process many event types from other nodes.
	 * So if the origin is behind it will not be able to catch up
	 * until the transaction completes.
	 *
	 * The slonik_submitEvent(...) that submits the
	 * setAddTable event might 'wait' until the
	 * origin is caught up, so we make sure that the
	 * origin is caught up before opening the
	 * transaction.
	 **/
	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;
	dstring_init(&query);
	slon_mkquery(&query, "lock table \"_%s\".sl_event_lock;"
				 "select \"_%s\".createEvent('_%s', 'SYNC'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername);
	slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query, stmt->hdr.script, 0);
	dstring_terminate(&query);
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		dstring_free(&query);
		return -1;
	}




	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	/**
	 * obtain a lock on sl_event_lock.
	 * it must be obtained before calling setAddTable()
	 * and it must be obtained before querying the catalog.
	 */
	dstring_init(&query);
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername
		);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		printf("%s:%d:Error: unable to lock sl_event_lock\n",
			   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
		dstring_terminate(&query);
		return -1;

	}
	dstring_reset(&query);
	if (stmt->tab_fqname == NULL &&
		stmt->tables != NULL)
	{
		/**
		 * query the catalog to get a list of tables.
		 */
		slon_mkquery(&query, "select table_schema || '.' || table_name "
					 "from information_schema.tables where "
					 "table_schema || '.'||table_name ~ E'%s' "
					 " and table_type='BASE TABLE' order by 1", stmt->tables);
		result = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (result == NULL)
		{
			printf("%s:%d:Error unable to search for a list of tables. "
				   "Perhaps your regular expression '%s' is invalid.",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   stmt->tables);
			dstring_terminate(&query);
			return -1;

		}
		rc = 0;
		for (idx = 0; idx < PQntuples(result); idx++)
		{

			table_name = PQgetvalue(result, idx, 0);
			rc = slonik_set_add_single_table(stmt, adminfo1,
											 table_name);
			if (rc < 0)
			{
				PQclear(result);
				dstring_terminate(&query);
				return rc;
			}
		}
		PQclear(result);
	}
	else
		rc = slonik_set_add_single_table(stmt, adminfo1, stmt->tab_fqname);
	dstring_terminate(&query);
	return rc;
}
int
slonik_set_add_single_table(SlonikStmt_set_add_table * stmt,
							SlonikAdmInfo * adminfo1,
							const char *fqname)
{
	SlonDString query;
	char	   *idxname;
	PGresult   *res;
	int			tab_id = -1;
	int			rc = 0;

	dstring_init(&query);
	if (stmt->use_key == NULL)
	{
		slon_mkquery(&query,
					 "select \"_%s\".determineIdxnameUnique('%q', NULL); ",
					 stmt->hdr.script->clustername, fqname);

	}
	else
	{
		slon_mkquery(&query,
					 "select \"_%s\".determineIdxnameUnique('%q', '%q'); ",
					 stmt->hdr.script->clustername,
					 fqname, stmt->use_key);
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

	if (stmt->tab_id < 0)
	{
		tab_id = slonik_get_next_tab_id((SlonikStmt *) stmt);
		if (tab_id < 0)
		{
			PQclear(res);
			dstring_free(&query);
			return -1;
		}
	}
	else
		tab_id = stmt->tab_id;

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".setAddTable(%d, %d, '%q', '%q', '%q'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->set_id, tab_id,
				 fqname, idxname, stmt->tab_comment);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
	{
		PQclear(res);
		dstring_free(&query);
		return -1;
	}
	if (stmt->add_sequences)
		rc = slonik_add_dependent_sequences(stmt, adminfo1, fqname);

	dstring_free(&query);
	PQclear(res);
	return rc;
}


int
slonik_set_add_sequence(SlonikStmt_set_add_sequence * stmt)
{
	SlonikAdmInfo *adminfo1;
	int			origin = stmt->set_origin;
	int			rc;
	const char *sequence_name;
	SlonDString query;
	int			idx;
	PGresult   *result;


	if (stmt->set_origin < 0)
	{
		origin = find_origin((SlonikStmt *) stmt, stmt->set_id);
		if (origin < 0)
		{
			printf("%s:%d:Error: unable to determine the origin for set %d",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno, stmt->set_id);
			return -1;
		}
	}

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, origin);
	if (adminfo1 == NULL)
		return -1;

	/**
	 * make sure that the origin node is caught up with
	 * the last event node *before* opening the transaciton
	 * the 'set add table' will take place in.
	 *
	 * Once that transaction is opened and has a lock
	 * on sl_config_lock the slon for the node	won't be
	 * able to process many event types from other nodes.
	 * So if the origin is behind it will not be able to catch up
	 * until the transaction completes.
	 *
	 * The slonik_submitEvent(...) that submits the
	 * setAddSequence event might 'wait' until the
	 * origin is caught up, so we make sure that the
	 * origin is caught up before opening the
	 * transaction.
	 **/
	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;
	dstring_init(&query);
	slon_mkquery(&query, "lock table \"_%s\".sl_event_lock;"
				 "select \"_%s\".createEvent('_%s', 'SYNC'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername);
	slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query, stmt->hdr.script, 0);
	dstring_terminate(&query);
	if (db_commit_xact((SlonikStmt *) stmt, adminfo1) < 0)
	{
		dstring_free(&query);
		return -1;
	}



	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	/**
	 * obtain a lock on sl_event_lock.
	 * it must be obtained before calling setAddSequence()
	 * and it must be obtained before querying the catalog.
	 */
	dstring_init(&query);
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername
		);
	if (db_exec_command((SlonikStmt *) stmt, adminfo1, &query) < 0)
	{
		printf("%s:%d:Error: unable to lock sl_event_lock\n",
			   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno);
		dstring_terminate(&query);
		return -1;

	}
	dstring_reset(&query);

	if (stmt->seq_fqname == NULL &&
		stmt->sequences != NULL)
	{
		/**
		 * query the catalog to get a list of tables.
		 */
		slon_mkquery(&query, "select sequence_schema || '.' || sequence_name "
					 "from information_schema.sequences where "
					 "sequence_schema || '.'||sequence_name ~ '%s' "
					 "order by 1", stmt->sequences);
		result = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
		if (result == NULL)
		{
			printf("%s:%d:Error unable to search for a list of sequences. "
				   "Perhaps your regular expression '%s' is invalid.",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   stmt->sequences);
			dstring_terminate(&query);
			return -1;

		}
		rc = 0;
		for (idx = 0; idx < PQntuples(result); idx++)
		{

			sequence_name = PQgetvalue(result, idx, 0);
			rc = slonik_set_add_single_sequence((SlonikStmt *) stmt, adminfo1,
												sequence_name,
												stmt->set_id,
												stmt->seq_comment, -1);

			if (rc < 0)
			{
				PQclear(result);
				dstring_terminate(&query);
				return rc;
			}
		}
		PQclear(result);

	}
	else
		rc = slonik_set_add_single_sequence((SlonikStmt *) stmt, adminfo1,
											stmt->seq_fqname,
											stmt->set_id, stmt->seq_comment,
											stmt->seq_id);

	dstring_terminate(&query);
	return rc;
}
int
slonik_set_add_single_sequence(SlonikStmt * stmt,
							   SlonikAdmInfo * adminfo1,
							   const char *seq_name,
							   int set_id,
							   const char *seq_comment,
							   int seq_id)
{
	SlonDString query;

	/*
	 * call setAddSequence()
	 */

	db_notice_silent = true;

	if (seq_id < 0)
	{
		seq_id = slonik_get_next_sequence_id((SlonikStmt *) stmt);
		if (seq_id < 0)
			return -1;
	}

	dstring_init(&query);
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".setAddSequence(%d, %d, '%q', '%q'); ",
				 stmt->script->clustername,
				 stmt->script->clustername,
				 set_id, seq_id, seq_name,
				 seq_comment);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".setDropTable(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->tab_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	/*
	 * call setDropSequence()
	 */
	db_notice_silent = true;
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".setDropSequence(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->seq_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".setMoveTable(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->tab_id, stmt->new_set_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".setMoveSequence(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->seq_id, stmt->new_set_id);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	PGresult   *res1;
	int			reshape = 0;
	int			origin_id;
	int			rc;

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

	/**
	 * we don't actually want to execute that query until
	 * the provider node is caught up with all other nodes wrt config data.
	 *
	 * this is because we don't want to pick the origin based on
	 * stale data.
	 *
	 * @note an alternative might be to contact all adminconninfo
	 * nodes looking for the set origin and then submit the
	 * set origin to that.	This avoids the wait for and is probably
	 * what we should do.
	 */
	if (!auto_wait_disabled)
	{
		rc = slonik_wait_config_caughtup(adminfo1, &stmt->hdr, -1);
		if (rc < 0)
			return rc;
	}

	slon_mkquery(&query, "select count(*) FROM \"_%s\".sl_subscribe " \
				 "where sub_set=%d AND sub_receiver=%d " \
				 " and sub_active=true and sub_provider<>%d",
				 stmt->hdr.script->clustername,
				 stmt->sub_setid, stmt->sub_receiver,
				 stmt->sub_provider);

	res1 = db_exec_select(&stmt->hdr, adminfo1, &query);
	if (res1 == NULL)
	{
		dstring_free(&query);
		return -1;
	}
	if (PQntuples(res1) > 0)
	{
		if (strtol(PQgetvalue(res1, 0, 0), NULL, 10) > 0)
		{
			reshape = 1;
		}
	}
	PQclear(res1);
	dstring_reset(&query);

	slon_mkquery(&query,
				 "select set_origin from \"_%s\".sl_set where" \
				 " set_id=%d", stmt->hdr.script->clustername,
				 stmt->sub_setid);
	res1 = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	if (res1 == NULL || PQntuples(res1) <= 0)
	{
		printf("%s:%d error: can not determine set origin for set %d\n",
			   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno, stmt->sub_setid);
		if (res1 != NULL)
			PQclear(res1);
		dstring_free(&query);
		return -1;

	}
	origin_id = atoi(PQgetvalue(res1, 0, 0));
	if (origin_id <= 0)
	{
		PQclear(res1);
		dstring_free(&query);
		return -1;

	}
	PQclear(res1);
	dstring_reset(&query);
	adminfo2 = get_active_adminfo((SlonikStmt *) stmt, origin_id);
	if (db_begin_xact((SlonikStmt *) stmt, adminfo2, false) < 0)
		return -1;
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".subscribeSet(%d, %d, %d, '%s', '%s'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->sub_setid, stmt->sub_provider,
				 stmt->sub_receiver,
				 (stmt->sub_forward) ? "t" : "f",
				 (stmt->omit_copy) ? "t" : "f");
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo2, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_reset(&query);
	if (reshape)
	{
		adminfo2 = get_active_adminfo((SlonikStmt *) stmt, stmt->sub_receiver);
		if (adminfo2 == NULL)
		{
			printf("can not find conninfo for receiver node %d\n",
				   stmt->sub_receiver);
			return -1;
		}
		slon_mkquery(&query,
					 "lock table \"_%s\".sl_config_lock;"
					 "select \"_%s\".reshapeSubscription(%d,%d,%d);",
					 stmt->hdr.script->clustername,
					 stmt->hdr.script->clustername,
					 stmt->sub_setid,
					 stmt->sub_provider,
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".unsubscribeSet(%d, %d,false); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->sub_setid, stmt->sub_receiver);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".lockSet(%d); "
				 "select pg_catalog.txid_snapshot_xmax(pg_catalog.txid_current_snapshot());",
				 stmt->hdr.script->clustername,
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".unlockSet(%d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".moveSet(%d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->set_id, stmt->new_origin);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
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
	SlonDString query,
				equery;
	SlonDString script_content;
	SlonDString script_rewritten;
	PGresult   *res1;
	size_t		num_read;
	int			num_statements = -1,
				stmtno;
	char		buf[4096];
	replacement_token	replacements[4];

#define PARMCOUNT 1

	const char *params[PARMCOUNT];

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->ev_origin);
	if (adminfo1 == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&script_content);
	dstring_init(&script_rewritten);

	if (stmt->ddl_fd != NULL)
	{
		while (!feof(stmt->ddl_fd))
		{
			num_read = fread(buf, 1, sizeof(buf), stmt->ddl_fd);
			if (num_read == 0)
				break;
			dstring_nappend(&script_content, buf, num_read);
		}
		fclose(stmt->ddl_fd);
		stmt->ddl_fd = NULL;
	} else {
		if (stmt->ddl_sql == NULL)
		{
			printf("internal error - ddl_fd == NULL and ddl_sql == NULL\n");
			return -1;
		}
		dstring_append(&script_content, stmt->ddl_sql);
	}
	dstring_terminate(&script_content);

	/*
	 * Setup the string replacement table and call replace_tokens() to
	 * perform all the substitutions.
	 */
	replacements[0].old_str = "@CLUSTERNAME@";
	replacements[0].old_len = strlen(replacements[0].old_str);
	replacements[0].new_str = stmt->hdr.script->clustername;
	replacements[1].old_str = "@MODULEVERSION@";
	replacements[1].old_len = strlen(replacements[1].old_str);
	replacements[1].new_str = SLONY_I_VERSION_STRING;
	replacements[2].old_str = "@NAMESPACE@";
	replacements[2].old_len = strlen(replacements[2].old_str);
	replacements[2].new_str = alloca(strlen(stmt->hdr.script->clustername) + 4);
	sprintf(replacements[2].new_str, "\"_%s\"", stmt->hdr.script->clustername);
	replacements[3].old_str = NULL;
	replacements[3].old_len = 0;
	replacements[3].new_str = NULL;

	replace_tokens(&script_rewritten, &script_content, replacements);
	dstring_free(&script_content);

	/*
	 * This prepares the statement that will be run over and over for each DDL
	 * statement
	 */
	dstring_init(&equery);
	if ((stmt->only_on_nodes == NULL) && (stmt->only_on_node < 0))
	{
		slon_mkquery(&equery,
					 "select \"_%s\".ddlCapture($1, NULL::text);",
					 stmt->hdr.script->clustername);
	}
	else
	{
		if (stmt->only_on_node > 0)
		{
			slon_mkquery(&equery,
						 "select \"_%s\".ddlCapture($1, '%d');",
						 stmt->hdr.script->clustername, stmt->only_on_node);
		}
		else
		{						/* stmt->only_on_nodes is populated */
			slon_mkquery(&equery,
						 "select \"_%s\".ddlCapture($1, '%s');",
						 stmt->hdr.script->clustername, stmt->only_on_nodes);
		}
	}

	/*
	 * Split the script into a series of SQL statements - each needs to be
	 * submitted separately
	 */
	num_statements = scan_for_statements(dstring_data(&script_rewritten));

	/* OOPS!  Something went wrong !!! */
	if ((num_statements < 0) || (num_statements >= MAXSTATEMENTS))
	{
		printf("DDL - number of statements invalid - %d not between 0 and %d\n", num_statements, MAXSTATEMENTS);
		dstring_free(&script_rewritten);
		return -1;
	}
	dstring_init(&query);
	for (stmtno = 0; stmtno < num_statements; stmtno++)
	{
		int			startpos,
					endpos;
		char	   *dest;

		if (stmtno == 0)
			startpos = 0;
		else
			startpos = STMTS[stmtno - 1];
		endpos = STMTS[stmtno];
		dest = (char *) malloc(endpos - startpos + 1);
		if (dest == 0)
		{
			printf("DDL Failure - could not allocate %d bytes of memory\n", endpos - startpos + 1);
			return -1;
		}
		strncpy(dest, dstring_data(&script_rewritten) + startpos, endpos - startpos);
		dest[STMTS[stmtno] - startpos] = '\0';
		slon_mkquery(&query, "%s", dest);
		free(dest);

		params[PARMCOUNT - 1] = dstring_data(&query);

		res1 = PQexecParams(adminfo1->dbconn, dstring_data(&equery), 1, NULL, params, NULL, NULL, 0);
		if (PQresultStatus(res1) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "%s [%s] - %s",
					PQresStatus(PQresultStatus(res1)),
					dstring_data(&query), PQresultErrorMessage(res1));
			PQclear(res1);
			return -1;
		}
		PQclear(res1);
	}

	/*
	 * Finally call ddlScript_complete()
	 */
	if ((stmt->only_on_nodes == NULL) && (stmt->only_on_node < 0))
	{
		slon_mkquery(&query,
					 "select \"_%s\".ddlScript_complete(NULL::text);",
					 stmt->hdr.script->clustername);
	}
	else
	{
		if (stmt->only_on_node > 0)
		{
			slon_mkquery(&query,
						 "select \"_%s\".ddlScript_complete('%d');",
						 stmt->hdr.script->clustername, stmt->only_on_node);
		}
		else
		{						/* stmt->only_on_nodes is populated */
			slon_mkquery(&query,
						 "select \"_%s\".ddlScript_complete('%s');",
						 stmt->hdr.script->clustername, stmt->only_on_nodes);
		}
	}
	res1 = PQexec(adminfo1->dbconn, dstring_data(&query));
	if (PQresultStatus(res1) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "%s [%s] - %s",
				PQresStatus(PQresultStatus(res1)),
				dstring_data(&query), PQresultErrorMessage(res1));
		PQclear(res1);
		dstring_free(&equery);
		dstring_free(&script_rewritten);
		dstring_free(&query);
		return -1;
	}
	PQclear(res1);

	dstring_free(&equery);
	dstring_free(&script_rewritten);
	dstring_free(&query);
	return 0;
}


int
slonik_update_functions(SlonikStmt_update_functions * stmt)
{
	SlonikAdmInfo *adminfo;
	PGresult   *res;
	SlonDString query;

	adminfo = get_active_adminfo((SlonikStmt *) stmt, stmt->no_id);
	if (adminfo == NULL)
		return -1;

	if (db_begin_xact((SlonikStmt *) stmt, adminfo, false) < 0)
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
	char		seqbuf[NAMEDATALEN];
	int			loop_count = 0;
	SlonDString outstanding_nodes;
	int			tupindex;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->wait_on);
	if (adminfo1 == NULL)
		return -1;

	time(&timeout);
	timeout += stmt->wait_timeout;
	dstring_init(&query);
	dstring_init(&outstanding_nodes);
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
				SlonDString ignore_condition;
				int		   *node_ptr;

				sprintf(seqbuf, INT64_FORMAT, adminfo->last_event);
				dstring_init(&ignore_condition);

				for (node_ptr = stmt->ignore_nodes; node_ptr != NULL &&
					 *node_ptr != -1; node_ptr++)
				{
					if (node_ptr != stmt->ignore_nodes)
						slon_appendquery(&ignore_condition,
										 ",%d", *node_ptr);
					else
						slon_appendquery(&ignore_condition,
									  "and N.no_id not in ( %d ", *node_ptr);
				}
				if (node_ptr != stmt->ignore_nodes)
					slon_appendquery(&ignore_condition, ")");
				else
					slon_appendquery(&ignore_condition, "");
				dstring_terminate(&ignore_condition);
				slon_mkquery(&query,
							 "select no_id, max(con_seqno) "
						   "    from \"_%s\".sl_node N, \"_%s\".sl_confirm C"
							 "    where N.no_id <> %d "
							 "    and N.no_active "
							 "    %s              "
							 "    and N.no_id = C.con_received "
							 "    and C.con_origin = %d "
							 "    group by no_id "
							 "    having max(con_seqno) < '%s'; ",
							 stmt->hdr.script->clustername,
							 stmt->hdr.script->clustername,
							 adminfo->no_id, dstring_data(&ignore_condition),
							 adminfo->no_id,
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
			{
				all_confirmed = 0;
				dstring_reset(&outstanding_nodes);
				for (tupindex = 0; tupindex < PQntuples(res); tupindex++)
				{
					char	   *node = PQgetvalue(res, tupindex, 0);
					char	   *last_event = PQgetvalue(res, tupindex, 1);

					if (last_event == 0)
						last_event = "null";
					slon_appendquery(&outstanding_nodes, "%snode %s only on event %s"
									 ,tupindex == 0 ? "" : ", "
									 ,node, last_event);

				}
				dstring_terminate(&outstanding_nodes);
			}
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

		loop_count++;
		if (loop_count % 10 == 0 && stmt->wait_confirmed >= 0)
		{
			sprintf(seqbuf, INT64_FORMAT, adminfo->last_event);
			printf("%s:%d: waiting for event (%d,%s) to be confirmed on node %d\n"
				   ,stmt->hdr.stmt_filename, stmt->hdr.stmt_lno
				   ,stmt->wait_origin, seqbuf,
				   stmt->wait_confirmed);
			fflush(stdout);
		}
		else if (loop_count % 10 == 0)
		{
			sprintf(seqbuf, INT64_FORMAT, adminfo->last_event);
			printf("%s:%d: waiting for event (%d,%s).  %s\n",
				   stmt->hdr.stmt_filename, stmt->hdr.stmt_lno,
				   stmt->wait_origin, seqbuf,
				   dstring_data(&outstanding_nodes));
			fflush(stdout);

		}
		sleep(1);
	}
	dstring_free(&outstanding_nodes);
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".logswitch_start(); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
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

	if (db_begin_xact((SlonikStmt *) stmt, adminfo1, false) < 0)
		return -1;

	dstring_init(&query);

	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock;"
				 "select \"_%s\".createEvent('_%s', 'SYNC'); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo1, &query,
						   stmt->hdr.script, 1) < 0)
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
slon_scanint64(char *str, int64 *result)
{
	char	   *ptr = str;
	int64		tmp = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be broken
	 * for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char) *ptr))
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
	if (!isdigit((unsigned char) *ptr))
		return false;

	/* process digits */
	while (*ptr && isdigit((unsigned char) *ptr))
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


/**
* Substitute strings in a dstring, creating a new dstring as a result.
*
*/
static void
replace_tokens(SlonDString *dest, SlonDString *src, 
		replacement_token *replacements)
{
	char   *src_cp = dstring_data(src);

	while (*src_cp != '\0')
	{
		replacement_token  *tp;
		int					found = false;
		
		for (tp = replacements; tp->old_str != NULL; tp++)
		{
			if (strncmp(src_cp, tp->old_str, tp->old_len) == 0)
			{
				dstring_append(dest, tp->new_str);
				src_cp += tp->old_len;
				found = true;
				break;
			}
		}

		if (!found)
		{
			dstring_addchar(dest, *src_cp++);
		}
	}
	dstring_terminate(dest);
}


/**
* checks all nodes (that an admin conninfo exists for)
* to find the next available table id.
*
*
*/
static int
slonik_get_next_tab_id(SlonikStmt * stmt)
{
	SlonikAdmInfo *adminfo_def;
	SlonDString query;
	int			max_tab_id = 0;
	int			tab_id = 0;
	char	   *tab_id_str;
	PGresult   *res;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select max(tab_id) FROM \"_%s\".sl_table",
				 stmt->script->clustername);

	for (adminfo_def = stmt->script->adminfo_list;
		 adminfo_def; adminfo_def = adminfo_def->next)
	{
		SlonikAdmInfo *adminfo = get_active_adminfo(stmt,
													adminfo_def->no_id);

		if (adminfo == NULL)
		{

			printf("%s:%d: Error: could not connect to node %d for next table"
				   " id",
				   stmt->stmt_filename, stmt->stmt_lno,
				   adminfo_def->no_id);
			dstring_terminate(&query);
			return -1;
		}
		if (slonik_is_slony_installed(stmt, adminfo) > 0)
		{
			res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
			if (res == NULL)
			{
				printf("%s:%d: Error:could not query node %d for next table id",
					   stmt->stmt_filename, stmt->stmt_lno,
					   adminfo->no_id);
				if (res != NULL)
					PQclear(res);
				dstring_terminate(&query);
				return -1;
			}
		}
		else
		{
			/**
			 * if slony is not yet installed on the node we can skip it.
			 */
			continue;
		}
		if (PQntuples(res) > 0)
		{
			tab_id_str = PQgetvalue(res, 0, 0);
			if (tab_id_str != NULL)
				tab_id = strtol(tab_id_str, NULL, 10);
			else
			{
				PQclear(res);
				continue;
			}
			if (tab_id > max_tab_id)
				max_tab_id = tab_id;
		}
		PQclear(res);
	}							/* for */
	dstring_terminate(&query);
	return max_tab_id + 1;
}



/**
* checks all nodes (that an admin conninfo exists for)
* to find the next available sequence id.
*
*
*/
static int
slonik_get_next_sequence_id(SlonikStmt * stmt)
{
	SlonikAdmInfo *adminfo_def;
	SlonDString query;
	int			max_seq_id = 0;
	int			seq_id = 0;
	char	   *seq_id_str;
	PGresult   *res;
	int			rc;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select max(seq_id) FROM \"_%s\".sl_sequence",
				 stmt->script->clustername);

	for (adminfo_def = stmt->script->adminfo_list;
		 adminfo_def; adminfo_def = adminfo_def->next)
	{
		SlonikAdmInfo *adminfo = get_active_adminfo(stmt,
													adminfo_def->no_id);

		if (adminfo == NULL)
		{

			printf("%s:%d: Error: could not query node %d for next sequence id",
				   stmt->stmt_filename, stmt->stmt_lno,
				   adminfo_def->no_id);
			dstring_terminate(&query);
			return -1;
		}
		if ((rc = slonik_is_slony_installed(stmt, adminfo)) > 0)
		{
			res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
			if (res == NULL)
			{

				printf("%s:%d: Error: could not query node %d for next "
					   "sequence id",
					   stmt->stmt_filename, stmt->stmt_lno,
					   adminfo->no_id);
				if (res != NULL)
					PQclear(res);
				dstring_terminate(&query);
				return -1;
			}
		}
		else if (rc < 0)
		{
			return rc;
		}
		else
		{
			continue;
		}
		if (PQntuples(res) > 0)
		{
			seq_id_str = PQgetvalue(res, 0, 0);
			if (seq_id_str != NULL)
				seq_id = strtol(seq_id_str, NULL, 10);
			else
			{
				PQclear(res);
				continue;
			}
			if (seq_id > max_seq_id)
				max_seq_id = seq_id;
		}
		PQclear(res);
	}
	dstring_terminate(&query);
	return max_seq_id + 1;
}

/**
* find the origin node for a particular set.
* This function will query the first admin node it
* finds to determine the origin of the set.
*
* If the node doesn't know about the set then
* it will query the next admin node until it finds
* one that does.
*
*/
static int
find_origin(SlonikStmt * stmt, int set_id)
{

	SlonikAdmInfo *adminfo_def;
	SlonDString query;
	PGresult   *res;
	int			origin_id = -1;
	char	   *origin_id_str;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select set_origin from \"_%s\".\"sl_set\" where set_id=%d",
				 stmt->script->clustername, set_id);

	for (adminfo_def = stmt->script->adminfo_list;
		 adminfo_def; adminfo_def = adminfo_def->next)
	{
		SlonikAdmInfo *adminfo = get_active_adminfo(stmt,
													adminfo_def->no_id);

		if (adminfo == NULL)
			continue;
		res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
		if (res == NULL)
		{
			printf("%s:%d: warning: could not query node %d for origin",
				   stmt->stmt_filename, stmt->stmt_lno,
				   adminfo->no_id);
			continue;
		}
		if (PQntuples(res) > 0)
		{
			origin_id_str = PQgetvalue(res, 0, 0);
			if (origin_id_str != NULL)
			{
				origin_id = strtol(origin_id_str, NULL, 10);
				PQclear(res);
			}
			else
			{
				PQclear(res);
				continue;

			}
		}
		if (origin_id >= 0)
			break;
	}							/* for */

	dstring_terminate(&query);


	return origin_id;
}


/**
* adds any sequences that table_name depends on to the replication
* set.
*
*
*
*/
int
slonik_add_dependent_sequences(SlonikStmt_set_add_table * stmt,
							   SlonikAdmInfo * adminfo1,
							   const char *table_name)
{

	SlonDString query;
	PGresult   *result;
	int			idx = 0;
	const char *seq_name;
	char	   *comment;
	int			rc;

	dstring_init(&query);
	slon_mkquery(&query,
				 "select pg_get_serial_sequence('%s',column_name) "
				 "FROM information_schema.columns where table_schema ||"
				 "'.' || table_name='%s'",
				 table_name, table_name);
	result = db_exec_select((SlonikStmt *) stmt, adminfo1, &query);
	if (result == NULL)
	{
		dstring_terminate(&query);
		return -1;
	}
	for (idx = 0; idx < PQntuples(result); idx++)
	{

		if (!PQgetisnull(result, idx, 0))
		{
			seq_name = PQgetvalue(result, idx, 0);
			/**
			 * add the sequence to the replication set
			 */
			comment = malloc(strlen(table_name) + strlen("sequence for ") + 1);
			sprintf(comment, "sequence for %s", table_name);
			rc = slonik_set_add_single_sequence((SlonikStmt *) stmt, adminfo1,
												seq_name,
												stmt->set_id,
												comment, -1);
			free(comment);
			if (rc < 0)
			{
				PQclear(result);
				dstring_terminate(&query);
				return rc;
			}

		}

	}							/* for */
	PQclear(result);
	dstring_terminate(&query);
	return 0;

}


/**
* checks to see if slony is installed on the given node.
*
* this function will check to see if slony tables exist
* on the node by querying the information_schema.
*
* returns:
*		 -1 => could not query information schema
*		  0 => slony not installed
*		  1 => slony is installed.
*/
static int
slonik_is_slony_installed(SlonikStmt * stmt,
						  SlonikAdmInfo * adminfo)
{
	SlonDString query;
	PGresult   *res;
	int			rc = -1;
	bool		txn_open = adminfo->have_xact;

	if (db_begin_xact(stmt, adminfo, true) < 0)
		return -1;

	dstring_init(&query);
	slon_mkquery(&query, "select count(*) FROM information_schema"
				 ".tables where table_schema='_%s' AND table_name"
				 "='sl_table'", stmt->script->clustername);
	res = db_exec_select((SlonikStmt *) stmt, adminfo, &query);
	if (res == NULL || PQntuples(res) <= 0)
		rc = -1;
	else if (strncmp(PQgetvalue(res, 0, 0), "1", 1) == 0)
		rc = 1;
	else
		rc = 0;

	if (res != NULL)
		PQclear(res);
	if (!txn_open)
		db_rollback_xact(stmt, adminfo);
	dstring_terminate(&query);
	return rc;

}

/* slonik_submitEvent(stmt, adminfo, query, script, suppress_wait_for)
 *
 * Wraps former requests to generate events, folding together the
 * logic for whether or not to do auto wait for or suppress this into
 * one place.
 */

static int
slonik_submitEvent(SlonikStmt * stmt,
				   SlonikAdmInfo * adminfo,
				   SlonDString * query,
				   SlonikScript * script,
				   int suppress_wait_for)
{
	int			rc;
	int			recreate_txn = 0;

	if (last_event_node >= 0 &&
		last_event_node != adminfo->no_id
		&& !suppress_wait_for)
	{
		SlonikStmt_wait_event wait_event;

		/**
		 * the last event node is not the current event node.
		 * time to wait.
		 */

		if (current_try_level != 0)
		{
			if (current_try_level == 1 && block_stmt_no == 1)
			{
				recreate_txn = 1;
				db_rollback_xact(stmt, adminfo);
			}
			else
			{
				printf("%s:%d Error: the event origin can not be changed "
					   "inside of a try block",
					   stmt->stmt_filename, stmt->stmt_lno);
				return -1;
			}
		}

		/**
		 * for now we generate a 'fake' Slonik_wait_event structure
		 *
		 */
		wait_event.hdr = *stmt;
		wait_event.wait_origin = last_event_node;
		wait_event.wait_on = last_event_node;
		wait_event.wait_confirmed = adminfo->no_id;
		wait_event.wait_timeout = 0;
		wait_event.ignore_nodes = 0;
		rc = slonik_wait_event(&wait_event);
		if (recreate_txn)
		{
			db_begin_xact(stmt, adminfo, false);
		}
		if (rc < 0)
			return rc;

	}
	rc = db_exec_evcommand(stmt, adminfo, query);
	if (!suppress_wait_for)
		last_event_node = adminfo->no_id;
	return rc;

}

/**
 * slonik_get_last_event_id(stmt, script, event_filter, events)
 *
 * query all nodes we have admin conninfo data for and
 * find the last event_id  of event types matching event_filter
 * generated from that node.
 *
 * store this in the SlonikAdmInfo structure so it can later
 * be used as part of a wait for.
 *
 */
static size_t
slonik_get_last_event_id(SlonikStmt * stmt,
						 SlonikScript * script,
						 const char *event_filter,
						 int64 **events)
{

	SlonDString query;
	PGresult   *result;
	char	   *event_id;
	SlonikAdmInfo *curAdmInfo = NULL;
	int			node_count = 0;
	int			node_idx;
	int			rc;

	dstring_init(&query);
	slon_mkquery(&query, "select max(ev_seqno) FROM \"_%s\".sl_event"
				 " , \"_%s\".sl_node "
				 " where ev_origin= "
				 "     (select last_value from \"_%s\".sl_local_node_id) "
				 " AND %s AND sl_node.no_id="
				 " ev_origin"
				 ,script->clustername, script->clustername,
				 script->clustername, event_filter);
	node_count = 0;
	for (curAdmInfo = script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
	{
		node_count++;
	}
	*events = malloc(sizeof(int64) * (node_count + 1));
	node_idx = 0;
	for (curAdmInfo = script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next, node_idx++)
	{
		SlonikAdmInfo *activeAdmInfo =
		get_active_adminfo(stmt, curAdmInfo->no_id);

		if (activeAdmInfo == NULL)
		{
			/**
			 * warning?
			 */
			(*events)[node_idx] = -1;
			continue;
		}
		rc = slonik_is_slony_installed(stmt, activeAdmInfo);
		if (rc == 1)
		{
			result = db_exec_select(stmt, activeAdmInfo, &query);
			if (result == NULL || PQntuples(result) != 1)
			{
				printf("error: unable to query event history on node %d\n",
					   curAdmInfo->no_id);
				if (result != NULL)
					PQclear(result);
				return -1;
			}
			event_id = PQgetvalue(result, 0, 0);
			db_rollback_xact(stmt, activeAdmInfo);
			if (event_id != NULL)
				(*events)[node_idx] = strtoll(event_id, NULL, 10);
			else
				(*events)[node_idx] = -1;
			PQclear(result);
		}
		else
		{
			(*events)[node_idx] = -1;
		}

	}


	dstring_terminate(&query);
	return node_count;
}

/**
 * waits until adminfo1 is caught up with config events from
 * all other nodes.
 *
 * adminfo1 - The node that we are waiting to be caught up
 * stmt - The statement that is currently being executed
 * ignore_node - allows 1 node to be ignored (don't wait for
 *				 adminfo1 to be caught up with that node)
 *				 -1 means don't ignore any nodes.
 *
 * Returns:
 *	0 - if all went fine
 * -1 - if a WAIT is attempted inside a slonik TRY block
 */
static int
slonik_wait_config_caughtup(SlonikAdmInfo * adminfo1,
							SlonikStmt * stmt,
							int ignore_node)
{
	SlonDString event_list;
	PGresult   *result = NULL;
	SlonikAdmInfo *curAdmInfo = NULL;
	int			first_event = 1;
	int			confirm_count = 0;
	SlonDString is_caughtup_query;
	SlonDString node_list;
	int			wait_count = 0;
	int			node_list_size = 0;
	int			sleep_count = 0;
	int64	   *behind_nodes = NULL;
	int			idx;
	int			cur_array_idx;
	int			recreate_txn = 0;

	/**
	 * an array that stores a node_id, last_event.
	 * or the last event seen for each admin conninfo
	 * node.
	 */
	int64	   *last_event_array = NULL;


	dstring_init(&event_list);
	dstring_init(&node_list);

	if (current_try_level != 0)
	{
		if (current_try_level == 1 && block_stmt_no == 1)
		{
			/**
			 * The first statement in the try block requires
			 * a wait for.	the code below will rollback the txn.
			 * we set a flag so we know to create a new one
			 * when we are done.
			 */
			recreate_txn = 1;
		}
		else
		{
			printf("%s:%d Error: WAIT operation forbidden inside a try block\n",
				   stmt->stmt_filename, stmt->stmt_lno);
			return -1;
		}
	}

	for (curAdmInfo = stmt->script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
	{
		node_list_size++;
	}
	last_event_array = malloc(node_list_size * sizeof(int64) * 2);
	memset(last_event_array, 0, sizeof(node_list_size * sizeof(int64) * 2));

	for (curAdmInfo = stmt->script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
	{
		char		seqno[NAMEDATALEN];

		if (curAdmInfo->last_event < 0 ||
			curAdmInfo->no_id == adminfo1->no_id ||
			curAdmInfo->no_id == ignore_node)
			continue;

		sprintf(seqno, INT64_FORMAT, curAdmInfo->last_event);
		slon_appendquery(&event_list,
						 "%s (node_list.no_id=%d)"
						 ,first_event ? " " : " OR "
						 ,curAdmInfo->no_id
						 ,seqno
			);
		slon_appendquery(&node_list, "%s (%d) ",
						 first_event ? " " : ",",
						 curAdmInfo->no_id);
		last_event_array[wait_count * 2] = curAdmInfo->no_id;
		last_event_array[wait_count * 2 + 1] = curAdmInfo->last_event;
		first_event = 0;
		wait_count++;
	}
	dstring_terminate(&node_list);


	dstring_init(&is_caughtup_query);
	/**
	 * I need a row for the case where a node is not in sl_confirm
	 * and the node is disabled or deleted.
	 */
	slon_mkquery(&is_caughtup_query,
				 "select node_list.no_id,max(con_seqno),no_active FROM "
				 " (VALUES %s) as node_list (no_id) LEFT JOIN "
				 "\"_%s\".sl_confirm ON(sl_confirm.con_origin=node_list.no_id"
				 " AND sl_confirm.con_received=%d)"
			 " LEFT JOIN \"_%s\".sl_node ON (node_list.no_id=sl_node.no_id) "
				 "GROUP BY node_list.no_id,no_active"
				 ,dstring_data(&node_list)
				 ,stmt->script->clustername
				 ,adminfo1->no_id
				 ,stmt->script->clustername);

	while (confirm_count != wait_count)
	{
		result = db_exec_select(stmt,
								adminfo1, &is_caughtup_query);
		if (result == NULL)
		{
			/**
			 * error
			 */
		}
		confirm_count = PQntuples(result);

		db_rollback_xact(stmt, adminfo1);

		/**
		 * find nodes that are missing.
		 *
		 */
		behind_nodes = malloc(node_list_size * sizeof(int64));
		memset(behind_nodes, 0, node_list_size * sizeof(int64));
		confirm_count = 0;
		for (idx = 0; idx < PQntuples(result); idx++)
		{
			char	   *n_id_c = PQgetvalue(result, idx, 0);
			int			n_id = atoi(n_id_c);
			char	   *seqno_c = PQgetvalue(result, idx, 1);
			int64		seqno = strtoll(seqno_c, NULL, 10);
			char	   *node_active = PQgetvalue(result, idx, 2);

			for (cur_array_idx = 0;
				 cur_array_idx < wait_count; cur_array_idx++)
			{
				if (last_event_array[cur_array_idx * 2] == n_id)
				{
					/*
					 * found.
					 */
					if (node_active == NULL || *node_active == 'f')
					{
						/**
						 * if node_active is null we assume the
						 * node has been deleted since it
						 * has no entry in sl_node
						 */
						behind_nodes[cur_array_idx] = -1;
						confirm_count++;
					}
					else if (last_event_array[cur_array_idx * 2 + 1] > seqno)
					{
						behind_nodes[cur_array_idx] = seqno;
					}
					else
					{
						behind_nodes[cur_array_idx] = -1;
						confirm_count++;
					}

				}

			}
		}						/* for .. PQntuples */
		if (confirm_count < wait_count)
		{
			sleep_count++;
			if (sleep_count % 10 == 0)
			{
				/**
				 * any elements in caught_up_nodes with a value 0
				 * means that the cooresponding node id in
				 * last_event_array is not showing up in the
				 * query result.
				 */
				SlonDString outstanding;

				dstring_init(&outstanding);
				first_event = 1;
				for (cur_array_idx = 0; cur_array_idx < wait_count;
					 cur_array_idx++)
				{
					if (behind_nodes[cur_array_idx] >= 0)
					{
						char		tmpbuf[96];

						sprintf(tmpbuf, "(" INT64_FORMAT "," INT64_FORMAT
								") only at (" INT64_FORMAT "," INT64_FORMAT
								")"
								,
								last_event_array[cur_array_idx * 2]
								,last_event_array[cur_array_idx * 2 + 1],
								last_event_array[cur_array_idx * 2],
								behind_nodes[cur_array_idx]);
						slon_appendquery(&outstanding, "%s %s"
										 ,first_event ? "" : ",", tmpbuf);
						first_event = 0;
					}

				}
				dstring_terminate(&outstanding);
				printf("waiting for events %s to be confirmed on node %d\n",
					   dstring_data(&outstanding), adminfo1->no_id);
				fflush(stdout);

			}					/* every 10 iterations */
			sleep(1);
		}
		free(behind_nodes);

	}							/* while */
	if (result != NULL)
		PQclear(result);
	dstring_terminate(&event_list);
	dstring_terminate(&is_caughtup_query);
	free(last_event_array);

	if (recreate_txn)
	{
		db_begin_xact(stmt, adminfo1, false);
	}

	return 0;

}

/**
 * Check all nodes in the cluster except those in skip_node_list
 * to find the maximum event id from node_id.
 * Normally skip_node_list would contain node_id as an element
 * (otherwise you could just get the answer directly from node_id).
 *
 * skip_node_list is a -1 terminated list of node_id values for the
 *			 nodes to ignore (not search).
 */
static int64
get_last_escaped_event_id(SlonikStmt * stmt,
						  int node_id,
						  int *skip_node_list)
{
	SlonDString query;
	PGresult   *result;
	char	   *event_id;
	SlonikAdmInfo *curAdmInfo = NULL;
	int64		max_event_id = 0;
	int64		cur_event_id;
	int			rc;

	dstring_init(&query);
	slon_mkquery(&query, "select max(ev_seqno) FROM \"_%s\".sl_event"
				 " where ev_origin=%d "
				 ,stmt->script->clustername, node_id);
	for (curAdmInfo = stmt->script->adminfo_list;
		 curAdmInfo != NULL; curAdmInfo = curAdmInfo->next)
	{
		int			node_list_idx;
		int			skip = 0;
		SlonikAdmInfo *activeAdmInfo=NULL;

		for (node_list_idx = 0; skip_node_list[node_list_idx] != -1; node_list_idx++)
		{
			if (curAdmInfo->no_id == skip_node_list[node_list_idx])
			{
				skip = 1;
				break;
			}
		}

		if (skip)
			continue;

		activeAdmInfo = get_active_adminfo(stmt, curAdmInfo->no_id);

		if (activeAdmInfo == NULL)
		{
			continue;
		}
		rc = slonik_is_slony_installed(stmt, activeAdmInfo);
		if (rc == 1)
		{
			result = db_exec_select(stmt, activeAdmInfo, &query);
			if (result != NULL || PQntuples(result) >= 1)
			{
				event_id = PQgetvalue(result, 0, 0);
				if (event_id != NULL)
					cur_event_id = strtoll(event_id, NULL, 10);
				else
					cur_event_id = -1;
				if (cur_event_id > max_event_id)
					max_event_id = cur_event_id;
			}
			PQclear(result);

		}
	}


	dstring_terminate(&query);
	return max_event_id;
}


int
slonik_resubscribe_node(SlonikStmt_resubscribe_node * stmt)
{


	SlonikAdmInfo *adminfo1;
	SlonikAdmInfo *adminfo2;
	SlonDString query;
	int			rc;

	adminfo1 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_provider);
	if (adminfo1 == NULL)
		return -1;


	dstring_init(&query);


	/**
	 * we don't actually want to execute the query until
	 * the provider node is caught up with all other nodes wrt config data.
	 *
	 * this is because we don't want to pick the origin based on
	 * stale data.
	 *
	 * @note an alternative might be to contact all adminconninfo
	 * nodes looking for the set origin and then submit the
	 * set origin to that.	This avoids the wait for and is probably
	 * what we should do.
	 */
	if (!auto_wait_disabled)
	{
		rc = slonik_wait_config_caughtup(adminfo1, &stmt->hdr, -1);
		if (rc < 0)
			return rc;
	}



	adminfo2 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_origin);
	if (db_begin_xact((SlonikStmt *) stmt, adminfo2, false) < 0)
		return -1;
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_event_lock, \"_%s\".sl_config_lock;"
				 "select \"_%s\".resubscribeNode(%d, %d, %d); ",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->no_origin, stmt->no_provider,
				 stmt->no_receiver);
	if (slonik_submitEvent((SlonikStmt *) stmt, adminfo2, &query,
						   stmt->hdr.script, auto_wait_disabled) < 0)
	{
		dstring_free(&query);
		return -1;
	}
	dstring_reset(&query);
	adminfo2 = get_active_adminfo((SlonikStmt *) stmt, stmt->no_receiver);
	if (adminfo2 == NULL)
	{
		printf("can not find conninfo for receiver node %d\n",
			   stmt->no_receiver);
		return -1;
	}
	slon_mkquery(&query,
				 "lock table \"_%s\".sl_config_lock;"
				 "select \"_%s\".reshapeSubscription(%d,%d,%d);",
				 stmt->hdr.script->clustername,
				 stmt->hdr.script->clustername,
				 stmt->no_origin,
				 stmt->no_provider,
				 stmt->no_receiver);
	if (db_exec_command((SlonikStmt *) stmt, adminfo2, &query) < 0)
	{
		printf("error reshaping subscriber\n");
	}

	dstring_free(&query);
	return 0;
}


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
