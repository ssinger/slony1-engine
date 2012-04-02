/*-------------------------------------------------------------------------
 * slony_logshipper.h
 *
 *	Definitions for slony_logshipper
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


/* ----------
 * SlonDString
 * ----------
 */
#define		SLON_DSTRING_SIZE_INIT	256
#define		SLON_DSTRING_SIZE_INC	2

typedef struct
{
	size_t		n_alloc;
	size_t		n_used;
	char	   *data;
}	SlonDString;

#define		dstring_init(__ds) \
do { \
	(__ds)->n_alloc = SLON_DSTRING_SIZE_INIT; \
	(__ds)->n_used = 0; \
	(__ds)->data = malloc(SLON_DSTRING_SIZE_INIT); \
	if ((__ds)->data == NULL) { \
		perror("dstring_init: malloc()"); \
		exit(-1); \
	} \
} while (0)
#define		dstring_reset(__ds) \
do { \
	(__ds)->n_used = 0; \
	(__ds)->data[0] = '\0'; \
} while (0)
#define		dstring_free(__ds) \
do { \
	free((__ds)->data); \
	(__ds)->n_used = 0; \
	(__ds)->data = NULL; \
} while (0)
#define		dstring_nappend(__ds,__s,__n) \
do { \
	if ((__ds)->n_used + (__n) >= (__ds)->n_alloc)	\
	{ \
		while ((__ds)->n_used + (__n) >= (__ds)->n_alloc) \
			(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			perror("dstring_nappend: realloc()"); \
			exit(-1); \
		} \
	} \
	memcpy(&((__ds)->data[(__ds)->n_used]), (__s), (__n)); \
	(__ds)->n_used += (__n); \
} while (0)
#define		dstring_append(___ds,___s) \
do { \
	register int ___n = strlen((___s)); \
	dstring_nappend((___ds),(___s),___n); \
} while (0)
#define		dstring_addchar(__ds,__c) \
do { \
	if ((__ds)->n_used + 1 >= (__ds)->n_alloc)	\
	{ \
		(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			perror("dstring_append: realloc()"); \
			exit(-1); \
		} \
	} \
	(__ds)->data[(__ds)->n_used++] = (__c); \
} while (0)
#define		dstring_terminate(__ds) \
do { \
	(__ds)->data[(__ds)->n_used] = '\0'; \
} while (0)
#define		dstring_data(__ds)	((__ds)->data)


/*
 * Parser data structures
 */
typedef struct AttElem_s
{
	char	   *attname;
	char	   *attvalue;
	struct AttElem_s *next;
}	AttElem;

typedef struct AttElemList_s
{
	AttElem    *list_head;
	AttElem    *list_tail;
}	AttElemList;

typedef struct InsertStmt_s
{
	char	   *namespace;
	char	   *tablename;
	AttElemList *attributes;
} InsertStmt;

typedef struct UpdateStmt_s
{
	char	   *namespace;
	char	   *tablename;
	AttElemList *changes;
	AttElemList *qualification;
} UpdateStmt;

typedef struct DeleteStmt_s
{
	char	   *namespace;
	char	   *tablename;
	int			only;
	AttElemList *qualification;
} DeleteStmt;

typedef struct TruncateStmt_s
{
	char	   *namespace;
	char	   *tablename;
} TruncateStmt;

typedef struct CopyStmt_s
{
	char	   *namespace;
	char	   *tablename;
	AttElemList *attributes;
	char	   *from;
} CopyStmt;

typedef struct RenameObject_s
{
	char	   *old_namespace;
	char	   *old_name;
	char	   *new_namespace;
	char	   *new_name;
	struct RenameObject_s *next;
}	RenameObject;

typedef struct ProcessingCommand_s
{
	char	   *command;
	struct ProcessingCommand_s *next;
}	ProcessingCommand;


typedef enum
{
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
}	log_level;


#ifndef MSGMAX
#define MSGMAX 1024
#endif


/*
 * Globals in slony_logshipper.c
 */
extern int	parse_errors;
extern char *current_file;
extern int	opt_quiet;
extern PGconn *dbconn;
extern bool logfile_switch_requested;
extern bool wait_for_resume;
extern bool shutdown_smart_requested;
extern bool shutdown_immed_requested;

extern char *archive_dir;
extern char *destination_dir;
extern char *destination_conninfo;
extern char *logfile_path;
extern int	max_archives;
extern char *cluster_name;
extern char *namespace;

extern RenameObject *rename_list;
extern ProcessingCommand *pre_processing_commands;
extern ProcessingCommand *post_processing_commands;
extern ProcessingCommand *error_commands;

/*
 * Functions in slony_logshipper.c
 */
extern int	process_check_at_counter(char *at_counter);
extern int	process_simple_sql(char *sql);
extern int	process_start_transaction(char *sql);
extern int	process_end_transaction(char *sql);
extern int	process_insert(InsertStmt *stmt);
extern int	process_update(UpdateStmt *stmt);
extern int	process_delete(DeleteStmt *stmt);
extern int	process_truncate(TruncateStmt *stmt);
extern int	process_copy(CopyStmt *stmt);
extern int	process_copydata(char *line);
extern int	process_copyend(void);
extern void config_add_rename(RenameObject * entry);
extern int lookup_rename(char *namespace, char *name,
			  char **use_namespace, char **use_name);
extern void errlog(log_level level, char *fmt,...);

/*
 * Functions in dbutil.c
 */
int			slon_mkquery(SlonDString * dsp, char *fmt,...);
int			slon_appendquery(SlonDString * dsp, char *fmt,...);


/*
 * Functions in ipcutil.c
 */
int			ipc_init(char *archive_dir);
int			ipc_finish(bool force);
int			ipc_poll(bool blocking);
int			ipc_send_path(char *logfname);
int			ipc_recv_path(char *buf);
int			ipc_send_term(char *archive_dir, bool immediate);
int			ipc_send_logswitch(char *archive_dir);
int			ipc_send_resume(char *archive_dir);
int			ipc_lock(void);
int			ipc_unlock(void);
int			ipc_wait_for_destroy(void);
int			ipc_cleanup(char *archive_dir);
void		ipc_set_shutdown_smart(void);
void		ipc_set_shutdown_immed(void);


/*
 * Parser related globals
 */
extern int	yylineno;
extern char *yytext;
extern FILE *yyin;
extern char yychunk[];

extern void scan_new_input_file(FILE *in);
extern void scan_push_string(char *str);
extern int	scan_yyinput(void);
extern void scan_copy_start(void);

extern void parse_error(const char *str);

extern void yyerror(const char *str);
extern int	yyparse(void);
extern int	yylex(void);


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
