%{
/*
 *
 *	The slony_logshipper command language grammar
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	
 *-------------------------------------------------------------------------
 */

#include "config.h"
#include "../slonik/types.h"
#include "libpq-fe.h"
#include "slony_logshipper.h"
#include "../parsestatements/scanner.h"

#include "scan.h"

extern int STMTS[MAXSTATEMENTS];


/*
 * Global data
 */
char   *current_file = "<stdin>";
#ifdef DEBUG
int yydebug=1;
#endif


%}

/*
 * Parser lval and types
 */
%union {
	int32		ival;
	char	   *str;
	AttElem	   *att_elem;
	AttElemList	*att_elem_list;
	SlonDString *dstring;
}

%type <ival>			num
%type <str>				ident
%type <str>				literal
%type <str>				arch_tracking_func
%type <str>				arch_finish_func
%type <str>				arch_seqsetval_func
%type <str>				arch_pgsetval_func
%type <att_elem>		arch_ident_eq_lit
%type <att_elem_list>	arch_upd_set
%type <att_elem_list>	arch_qualification
%type <ival>			arch_delete_only
%type <att_elem_list>	arch_delete_qual
%type <att_elem_list>	arch_attr_list
%type <att_elem_list>	arch_ins_vals
%type <dstring>			literal_parts
%type <str>				literal_part
%type <str>				literal_final
%type <dstring>			arch_copydata_parts
%type <str>				arch_copydata_part
%type <str>				arch_copydata_fin

/*
 * Keyword tokens
 */
%token	K_LOGFILE
%token	K_MAX
%token	K_ARCHIVES
%token	K_ANALYZE
%token	K_AND
%token	K_ARCHIVE
%token	K_ARCHIVE_COMMENT
%token  K_CASCADE
%token	K_CLUSTER
%token	K_COMMAND
%token	K_COMMIT
%token	K_CONF_COMMENT
%token	K_COPY
%token	K_DATABASE
%token	K_DELETE
%token	K_DESTINATION
%token	K_DIR
%token	K_ERROR
%token	K_EXEC_DDL
%token	K_FROM
%token	K_IGNORE
%token	K_INSERT
%token	K_INTO
%token	K_NAME
%token	K_NAMESPACE
%token	K_NULL
%token	K_ONLY
%token	K_POST
%token	K_PRE
%token	K_PROCESSING
%token	K_RENAME
%token	K_REPLICA
%token	K_SELECT
%token	K_SESSION_ROLE
%token	K_SET
%token	K_START
%token	K_START_ARCHIVE
%token	K_START_CONFIG
%token	K_TABLE
%token	K_TO
%token	K_TRANSACTION
%token  K_TRUNCATE
%token	K_UPDATE
%token	K_VACUUM
%token	K_VALUES
%token	K_WHERE

/*
 * Other scanner tokens
 */
%token	T_COPYDATA
%token	T_COPYDATA_PART
%token	T_COPYEND
%token	T_IDENT
%token	T_LITERAL
%token	T_LITERAL_PART
%token	T_NUMBER
%token	T_TRACKING_FUNCTION
%token	T_FINISH_FUNCTION
%token	T_SEQSETVAL_FUNCTION
%token	T_PGSETVAL_FUNCTION


%%

/*
 * One complete input is either a config file or a log archive
 */
valid_input			: configuration_file
					| archive_file
					;

configuration_file	: K_START_CONFIG ';'
                      conf_stmts
					;

conf_stmts			: conf_stmt
					| conf_stmts conf_stmt
					;

conf_stmt			: K_CONF_COMMENT
					| conf_archive_dir
					| conf_dest_dir
					| conf_dest_database
					| conf_logfile
					| conf_maxarchives
					| conf_clustername
					| conf_rename_object
					| conf_preprocess
					| conf_postprocess
					| conf_errorcommand
					;

conf_archive_dir	: K_ARCHIVE K_DIR '=' literal ';'
					{
						if (archive_dir != NULL)
							parse_error("duplicate archive dir specification");
						else
							archive_dir = strdup($4);
					}
					;

conf_dest_dir		: K_DESTINATION K_DIR '=' literal ';'
					{
						if (destination_dir != NULL)
							parse_error("duplicate destination dir specification");
						else
							destination_dir = strdup($4);
					}
					;

conf_dest_database	: K_DESTINATION K_DATABASE '=' literal ';'
					{
						if (destination_conninfo != NULL)
							parse_error("duplicate destination database specification");
						else
							destination_conninfo = strdup($4);
					}
					;

conf_logfile		: K_LOGFILE '=' literal ';'
					{
						if (logfile_path != NULL)
							parse_error("duplicate logfile path specification");
						else
							logfile_path = strdup($3);
					}
					;

conf_maxarchives	: K_MAX K_ARCHIVES '=' num ';'
					{
						max_archives = $4;
					}
					;

conf_clustername	: K_CLUSTER K_NAME '=' literal ';'
					{
						if (cluster_name != NULL)
							parse_error("duplicate cluster name specification");
						else
							cluster_name = strdup($4);
					}
					;

conf_rename_object	: K_IGNORE K_NAMESPACE ident ';'
					{
						RenameObject *new;

						new = (RenameObject *)malloc(sizeof(RenameObject));
						memset(new, 0, sizeof(RenameObject));

						new->old_namespace = $3;

						config_add_rename(new);
					}
					| K_IGNORE K_TABLE ident '.' ident ';'
					{
						RenameObject *new;

						new = (RenameObject *)malloc(sizeof(RenameObject));
						memset(new, 0, sizeof(RenameObject));

						new->old_namespace = $3;
						new->old_name      = $5;

						config_add_rename(new);
					}
					| K_RENAME K_NAMESPACE ident K_TO ident ';'
					{
						RenameObject *new;

						new = (RenameObject *)malloc(sizeof(RenameObject));
						memset(new, 0, sizeof(RenameObject));

						new->old_namespace = $3;
						new->new_namespace = $5;

						config_add_rename(new);
					}
					| K_RENAME K_TABLE ident '.' ident K_TO ident '.' ident ';'
					{
						RenameObject *new;

						new = (RenameObject *)malloc(sizeof(RenameObject));
						memset(new, 0, sizeof(RenameObject));

						new->old_namespace = $3;
						new->old_name      = $5;
						new->new_namespace = $7;
						new->new_name      = $9;

						config_add_rename(new);
					}
					;

conf_preprocess		: K_PRE K_PROCESSING K_COMMAND '=' literal ';'
					{
						ProcessingCommand	  **pptr;

						pptr = &pre_processing_commands;
						while (*pptr != NULL)
							pptr = &((*pptr)->next);
						
						*pptr = (ProcessingCommand *)malloc(sizeof(ProcessingCommand));
						(*pptr)->command = $5;
						(*pptr)->next = NULL;
					}
					;

conf_postprocess	: K_POST K_PROCESSING K_COMMAND '=' literal ';'
					{
						ProcessingCommand	  **pptr;

						pptr = &post_processing_commands;
						while (*pptr != NULL)
							pptr = &((*pptr)->next);
						
						*pptr = (ProcessingCommand *)malloc(sizeof(ProcessingCommand));
						(*pptr)->command = $5;
						(*pptr)->next = NULL;
					}
					;

conf_errorcommand	: K_ERROR K_COMMAND '=' literal ';'
					{
						ProcessingCommand	  **pptr;

						pptr = &error_commands;
						while (*pptr != NULL)
							pptr = &((*pptr)->next);
						
						*pptr = (ProcessingCommand *)malloc(sizeof(ProcessingCommand));
						(*pptr)->command = $4;
						(*pptr)->next = NULL;
					}
					;







archive_file		: K_START_ARCHIVE ';'
					  arch_hdr
					  arch_stmts
					;

arch_hdr			: arch_comments
					  arch_session_role
					  arch_start_trans
					  arch_tracking
					;

arch_session_role	: K_SET K_SESSION_ROLE K_TO K_REPLICA ';'
					{
						if (process_simple_sql(
							"set session_replication_role to replica;") < 0)
							YYABORT;
					}
					;

arch_start_trans	: K_START K_TRANSACTION ';'
					{
						if (process_start_transaction("start transaction;") < 0)
							YYABORT;
					}
					;

arch_tracking		: K_SELECT ident '.' arch_tracking_func '(' literal ',' literal ')' ';'
					{
						SlonDString	ds;
						int			rc;

						dstring_init(&ds);
						slon_mkquery(&ds, "select %s.%s('%s', '%s');",
								$2, $4, $6, $8);
						rc = process_check_at_counter($6);
						free($2);
						free($4);
						free($6);
						free($8);

						if (rc < 0)
						{
							dstring_free(&ds);
							YYABORT;
						}
						if (rc == 1)
						{
							dstring_free(&ds);
							YYACCEPT;
						}
						
						if (process_simple_sql(dstring_data(&ds)) < 0)
						{
							dstring_free(&ds);
							YYABORT;
						}
						else
							dstring_free(&ds);
					}

arch_tracking_func	: T_TRACKING_FUNCTION
					{
						char   *ret;
						size_t toklen=yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					;

arch_stmts			: arch_stmt
					| arch_stmts arch_stmt
					;

arch_stmt			: arch_comment
					| arch_commit
					| arch_insert
					| arch_update
					| arch_delete
					| arch_copy
					| arch_copydata
					| arch_copyend
					| arch_finishtable
					| arch_seqsetval
					| arch_pgsetval
					| arch_exec_ddl
					| arch_vacuum
					| arch_analyze
					| arch_truncate
					;

arch_commit			: K_COMMIT ';'
					{
						if (process_end_transaction("commit;") < 0)
							YYABORT;
					}
					;

arch_insert			: K_INSERT K_INTO ident '.' ident '(' arch_attr_list ')' K_VALUES '(' arch_ins_vals ')' ';'
					{
						InsertStmt	stmt;
						AttElem	   *nelem;
						AttElem	   *velem;
						AttElem	   *next;
						int			rc;

						stmt.namespace	= $3;
						stmt.tablename	= $5;
						stmt.attributes	= $7;

						for (nelem = $7->list_head, velem = $11->list_head;
								nelem != NULL && velem != NULL;
								nelem = nelem->next, velem = next)
						{
							nelem->attvalue = velem->attvalue;
							next = velem->next;
							free(velem);
						}
						free($11);

						rc = process_insert(&stmt);

						for (nelem = $7->list_head; nelem != NULL; nelem = next)
						{
							next = nelem->next;
							free(nelem->attname);
							if (nelem->attvalue != NULL)
								free(nelem->attvalue);
							free(nelem);
						}
						free(stmt.namespace);
						free(stmt.tablename);
						free(stmt.attributes);

						if (rc < 0)
							YYABORT;
					}
					;

arch_ins_vals		: literal
					{
						AttElemList	   *new;
						new = (AttElemList *)malloc(sizeof(AttElemList));

						new->list_head = (AttElem *)malloc(sizeof(AttElem));
						new->list_tail = new->list_head;

						new->list_head->attname = NULL;
						new->list_head->attvalue = $1;
						new->list_head->next = NULL;

						$$ = new;
					}
					| arch_ins_vals ',' literal
					{
						AttElem		   *new;
						new = (AttElem *)malloc(sizeof(AttElem));

						new->attname = NULL;
						new->attvalue = $3;
						new->next = NULL;

						$1->list_tail->next = new;
						$1->list_tail = new;

						$$ = $1;
					}
					;

arch_update			: K_UPDATE K_ONLY ident '.' ident arch_upd_set arch_qualification ';'
					{
						UpdateStmt		stmt;
						AttElem		   *elem;
						AttElem		   *next;
						int				rc;

						stmt.namespace		= $3;
						stmt.tablename		= $5;
						stmt.changes		= $6;
						stmt.qualification	= $7;

						rc = process_update(&stmt);

						for (elem = stmt.changes->list_head; elem != NULL; elem = next)
						{
							next = elem->next;
							free(elem->attname);
							if (elem->attvalue != NULL)
								free(elem->attvalue);
							free(elem);
						}
						free(stmt.changes);
						for (elem = stmt.qualification->list_head; elem != NULL; elem = next)
						{
							next = elem->next;
							free(elem->attname);
							if (elem->attvalue != NULL)
								free(elem->attvalue);
							free(elem);
						}
						free(stmt.namespace);
						free(stmt.tablename);
						free(stmt.qualification);

						if (rc < 0)
							YYABORT;
					}
					;

arch_upd_set		: K_SET arch_ident_eq_lit
					{
						AttElemList	   *new;
						new = (AttElemList *)malloc(sizeof(AttElemList));

						new->list_head = $2;
						new->list_tail = $2;

						$$ = new;
					}
					| arch_upd_set ',' arch_ident_eq_lit
					{ 
						$1->list_tail->next = $3;
						$1->list_tail = $3;

						$$ = $1;
					}

arch_delete			: K_DELETE K_FROM arch_delete_only ident '.' ident arch_delete_qual ';'
					{
						DeleteStmt		stmt;
						AttElem		   *elem;
						AttElem		   *next;
						int				rc;

						stmt.namespace		= $4;
						stmt.tablename		= $6;
						stmt.only			= $3;
						stmt.qualification	= $7;

						rc = process_delete(&stmt);

						if (stmt.qualification != NULL)
						{
							for (elem = stmt.qualification->list_head; elem != NULL; elem = next)
							{
								next = elem->next;
								free(elem->attname);
								if (elem->attvalue != NULL)
									free(elem->attvalue);
								free(elem);
							}
							free(stmt.qualification);
						}
						free(stmt.namespace);
						free(stmt.tablename);

						if (rc < 0)
							YYABORT;
					}
					;

arch_delete_only	: K_ONLY
					{
						$$ = 1;
					}
					|
					{
						$$ = 0;
					}
					;

arch_delete_qual	: arch_qualification
					{
						$$ = $1;
					}
					|
					{
						$$ = NULL;
					}
					;

arch_truncate		: K_TRUNCATE K_TABLE K_ONLY ident '.' ident K_CASCADE ';' 
					{
						
						TruncateStmt stmt;
						stmt.namespace=$4;
						stmt.tablename=$6;
						process_truncate(&stmt);

					};

arch_copy			: K_COPY ident '.' ident '(' arch_attr_list ')' K_FROM ident ';'
					{
						CopyStmt		stmt;
						AttElem		   *elem;
						AttElem		   *next;
						int				rc;

						stmt.namespace		= $2;
						stmt.tablename		= $4;
						stmt.attributes		= $6;
						stmt.from			= $9;

						rc = process_copy(&stmt);

						for (elem = stmt.attributes->list_head; elem != NULL; elem = next)
						{
							next = elem->next;
							free(elem->attname);
							if (elem->attvalue != NULL)
								free(elem->attvalue);
							free(elem);
						}
						free(stmt.attributes);
						free(stmt.namespace);
						free(stmt.tablename);

						if (rc < 0)
							YYABORT;
					}
					;

arch_copydata		: arch_copydata_fin
					{
						int rc;

						rc = process_copydata($1);
						free($1);

						if (rc < 0)
							YYABORT;
					}
					| arch_copydata_parts arch_copydata_fin
					{
						int rc;

						dstring_append($1, $2);
						dstring_terminate($1);
						free($2);
						rc = process_copydata(dstring_data($1));
						dstring_free($1);

						if (rc < 0)
							YYABORT;
					}
					;

arch_copydata_parts	: arch_copydata_part
					{
						SlonDString	*new;

						new = (SlonDString *)malloc(sizeof(SlonDString));
						dstring_init(new);
						dstring_append(new, $1);
						free($1);

						$$ = new;
					}
					| arch_copydata_parts arch_copydata_part
					{
						dstring_append($1, $2);
						free($2);

						$$ = $1;
					}
					;

arch_copydata_fin	: T_COPYDATA
					{
						$$ = strdup(yychunk);
					}
					;

arch_copydata_part	: T_COPYDATA_PART
					{
						$$ = strdup(yychunk);
					}
					;

arch_copyend		: T_COPYEND
					{
						if (process_copyend() < 0)
							YYABORT;
					}
					;

arch_attr_list		: ident
					{
						AttElemList	   *new;
						new = (AttElemList *)malloc(sizeof(AttElemList));

						new->list_head = (AttElem *)malloc(sizeof(AttElem));
						new->list_tail = new->list_head;

						new->list_head->attname = $1;
						new->list_head->attvalue = NULL;
						new->list_head->next = NULL;

						$$ = new;
					}
					| arch_attr_list ',' ident
					{
						AttElem		   *new;
						new = (AttElem *)malloc(sizeof(AttElem));

						new->attname = $3;
						new->attvalue = NULL;
						new->next = NULL;

						$1->list_tail->next = new;
						$1->list_tail = new;

						$$ = $1;
					}
					;

arch_qualification	: K_WHERE arch_ident_eq_lit
					{
						AttElemList	   *new;
						new = (AttElemList *)malloc(sizeof(AttElemList));

						new->list_head = $2;
						new->list_tail = $2;

						$$ = new;
					}
					| arch_qualification K_AND arch_ident_eq_lit
					{
						$1->list_tail->next = $3;
						$1->list_tail = $3;

						$$ = $1;
					}
					;

arch_ident_eq_lit	: ident '=' literal
					{
						AttElem	   *new = (AttElem *)malloc(sizeof(AttElem));

						new->attname = $1;
						new->attvalue = $3;
						new->next = NULL;

						$$ = new;
					}
					

arch_vacuum			: K_VACUUM K_ANALYZE ident '.' ident ';'
					{
						SlonDString	ds;
						char	   *namespace;
						char	   *tablename;

						if (lookup_rename($3, $5, &namespace, &tablename) == 1)
						{
							dstring_init(&ds);
							slon_mkquery(&ds, "vacuum analyze %s.%s;",
									$3, $5);
							free($3);
							free($5);
							if (process_simple_sql(dstring_data(&ds)) < 0)
							{
								dstring_free(&ds);
								YYABORT;
							}
							else
								dstring_free(&ds);
						}
						else
						{
							free($3);
							free($5);
						}
					}
					;

arch_analyze		: K_ANALYZE ident '.' ident ';'
					{
						SlonDString	ds;
						char	   *namespace;
						char	   *tablename;

						if (lookup_rename($2, $4, &namespace, &tablename) == 1)
						{
							dstring_init(&ds);
							slon_mkquery(&ds, "analyze %s.%s;",
									namespace, tablename);
							free($2);
							free($4);
							if (process_simple_sql(dstring_data(&ds)) < 0)
							{
								dstring_free(&ds);
								YYABORT;
							}
							else
								dstring_free(&ds);
						}
						else
						{
							free($2);
							free($4);
						}
					}
					;

arch_finishtable	: K_SELECT ident '.' arch_finish_func '(' num ')' ';'
					{
						SlonDString	ds;
						dstring_init(&ds);
						slon_mkquery(&ds, "select %s.%s(%d);",
								$2, $4, $6);
						free($2);
						free($4);
						if (process_simple_sql(dstring_data(&ds)) < 0)
						{
							dstring_free(&ds);
							YYABORT;
						}
						else
							dstring_free(&ds);
					}

arch_finish_func	: T_FINISH_FUNCTION
					{
						char   *ret;
						size_t toklen = yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					;

arch_seqsetval		: K_SELECT ident '.' arch_seqsetval_func '(' literal ',' literal ',' literal ')' ';'
					{
						SlonDString	ds;
						dstring_init(&ds);
						slon_mkquery(&ds, "select %s.%s('%s', '%s', '%s');",
									 $2, $4, $6, $8,$10);
						free($2);
						free($4);
						free($8);
						if (process_simple_sql(dstring_data(&ds)) < 0)
						{
							dstring_free(&ds);
							YYABORT;
						}
						else
							dstring_free(&ds);
					}

arch_seqsetval_func	: T_SEQSETVAL_FUNCTION
					{
						char   *ret;
						size_t toklen= yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					;

arch_pgsetval		: K_SELECT ident '.' arch_pgsetval_func '('  literal ',' literal ')' ';'
					{
						SlonDString	ds;
						dstring_init(&ds);
						slon_mkquery(&ds, "select %s.%s('%s', '%s');",
									 $2, $4, $6, $8);
						free($2);
						free($4);
						free($6);
						free($8);
						if (process_simple_sql(dstring_data(&ds)) < 0)
						{
							dstring_free(&ds);
							YYABORT;
						}
						else
							dstring_free(&ds);
					}

arch_pgsetval_func	: T_PGSETVAL_FUNCTION
					{
						char   *ret;
						size_t toklen = yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					;

arch_exec_ddl		: K_EXEC_DDL
					{
						SlonDString	ds;
						int			c;
						int			nstmts;
						int			i;

						if (process_simple_sql("-- DDL_SCRIPT") < 0)
							YYABORT;

						dstring_init(&ds);
						while ((c = scan_yyinput()) != EOF)
						{
							dstring_addchar(&ds, c);
						}
						dstring_terminate(&ds);

						if ((nstmts = scan_for_statements(dstring_data(&ds))) < 0)
						{
							errlog(LOG_ERROR, "scan_for_statements failed\n");
							dstring_free(&ds);
							YYABORT;
						}

						for (i = 0; i < nstmts; i++)
						{
							int start = (i == 0) ? 0 : STMTS[i - 1] + 1;
							int end = STMTS[i];

							dstring_data(&ds)[end] = '\0';

							if (process_simple_sql(dstring_data(&ds) + start) < 0)
							{
								dstring_free(&ds);
								YYABORT;
							}
						}
						dstring_free(&ds);

						/*
						 * Just in case
						 */
						process_end_transaction("rollback;");

						YYACCEPT;
					}
					;

arch_comments		: arch_comment
					| arch_comments arch_comment
					;

arch_comment		: K_ARCHIVE_COMMENT
					{
						char   *str;
						size_t toklen = yyget_leng();
						str = malloc(toklen + 1);
						memcpy(str, yytext, toklen);
						str[toklen] = '\0';

						if (process_simple_sql(str) < 0)
						{
							free(str);
							YYABORT;
						}
						else
							free(str);
					}
					;

num					: T_NUMBER
					{
						$$ = strtol(yytext, NULL, 10);
					}
					;

ident				: T_IDENT
					{
						char   *ret;
						size_t toklen = yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					| ident_keywords
					{
						char   *ret;
						size_t toklen = yyget_leng();
						ret = malloc(toklen + 1);
						memcpy(ret, yytext, toklen);
						ret[toklen] = '\0';

						$$ = ret;
					}
					;

ident_keywords		: K_START_CONFIG | K_START_ARCHIVE
					| K_ARCHIVE
					| K_ARCHIVES
					| K_CLUSTER
					| K_COMMIT
					| K_COPY
					| K_DATABASE
					| K_DELETE
					| K_DESTINATION
					| K_DIR
					| K_ERROR
					| K_IGNORE
					| K_INSERT
					| K_LOGFILE
					| K_MAX
					| K_NAME
					| K_NAMESPACE
					| K_POST
					| K_PRE
					| K_PROCESSING
					| K_RENAME
					| K_SET
					| K_START
					| K_TRANSACTION
					| K_TRUNCATE
					| K_UPDATE
					| K_VACUUM
					| K_VALUES
					| T_TRACKING_FUNCTION
					| T_FINISH_FUNCTION
					| T_SEQSETVAL_FUNCTION
					| T_PGSETVAL_FUNCTION
					;

literal				: literal_final
					{
						$$ = $1;
					}
					| literal_parts literal_final
					{
						char   *ret;

						dstring_append($1, $2);
						dstring_terminate($1);
						free($2);

						ret = strdup(dstring_data($1));
						dstring_free($1);
						
						$$ = ret;
					}
					| K_NULL
					{
						$$ = NULL;
					}
					;

literal_parts		: literal_part
					{
						SlonDString	   *dsp = (SlonDString *)malloc(sizeof(SlonDString));

						dstring_init(dsp);
						dstring_append(dsp, $1);
						free($1);

						$$ = dsp;
					}
					| literal_parts literal_part
					{
						dstring_append($1, $2);
						free($2);

						$$ = $1;
					}
					;

literal_part		: T_LITERAL_PART
					{
						$$ = strdup(yychunk);
					}

literal_final		: T_LITERAL
					{
						$$ = strdup(yychunk);
					}

%%


void
yyerror(const char *msg)
{
	errlog(LOG_ERROR, "%s:%d: ERROR %s at or near %s\n", current_file,
				yylineno, msg, yytext);
	parse_errors++;
}
void

parse_error(const char *msg)
{
	errlog(LOG_ERROR, "%s:%d: ERROR %s\n", current_file,
				yylineno, msg);
	parse_errors++;
}




/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
