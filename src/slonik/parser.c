#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28.2.1 2001/07/19 05:46:39 peter Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
#if defined(__cplusplus) || __STDC__
static int yygrowstack(void);
#else
static int yygrowstack();
#endif
#define YYPREFIX "yy"
#line 2 "parser.y"
/*-------------------------------------------------------------------------
 * parser.y
 *
 *	The slonik command language grammar
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: parser.c,v 1.1 2004-03-23 18:45:11 wieck Exp $
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "libpq-fe.h"
#include "slonik.h"


/*
 * Common option types
 */
typedef enum {
	O_CLIENT,
	O_COMMENT,
	O_CONNINFO,
	O_CONNRETRY,
	O_EVENT_NODE,
	O_FORWARD,
	O_FQNAME,
	O_ID,
	O_NEW_ORIGIN,
	O_NODE_ID,
	O_OLD_ORIGIN,
	O_ORIGIN,
	O_PROVIDER,
	O_RECEIVER,
	O_SERVER,
	O_SER_KEY,
	O_SET_ID,
	O_USE_KEY,

	END_OF_OPTIONS = -1
} option_code;


/*
 * Common given option list
 */
typedef struct option_list {
	option_code	opt_code;
	int			lineno;
	int32		ival;
	char	   *str;

	struct option_list *next;
} option_list;


/*
 * Common per statement possible option strcture
 */
typedef struct statement_option {
	option_code	opt_code;
	int			lineno;
	int			ival;
	char	   *str;
} statement_option;
#define	STMT_OPTION_INT(_code,_dfl)		{_code, -1, _dfl, NULL}
#define	STMT_OPTION_STR(_code,_dfl)		{_code, -1, -1, _dfl}
#define	STMT_OPTION_YN(_code,_dfl)		{_code, -1, -1, _dfl}
#define STMT_OPTION_END					{END_OF_OPTIONS, -1, -1, NULL}


/*
 * Global data
 */
char   *current_file = "<stdin>";


/*
 * Local functions
 */
static int	assign_options(statement_option *so, option_list *ol);



#line 92 "parser.y"
typedef union {
	int32			ival;
	char		   *str;
	option_list	   *opt_list;
	SlonikAdmInfo  *adm_info;
	SlonikStmt	   *statement;
} YYSTYPE;
#line 115 "y.tab.c"
#define YYERRCODE 256
#define K_ADD 257
#define K_ADMIN 258
#define K_CLIENT 259
#define K_CLUSTER 260
#define K_CLUSTERNAME 261
#define K_COMMENT 262
#define K_CONNINFO 263
#define K_CONNRETRY 264
#define K_CREATE 265
#define K_DROP 266
#define K_ECHO 267
#define K_ERROR 268
#define K_EVENT 269
#define K_EXIT 270
#define K_FALSE 271
#define K_FORWARD 272
#define K_FULL 273
#define K_ID 274
#define K_INIT 275
#define K_KEY 276
#define K_LISTEN 277
#define K_LOCK 278
#define K_MOVE 279
#define K_NAME 280
#define K_NEW 281
#define K_NO 282
#define K_NODE 283
#define K_OFF 284
#define K_OLD 285
#define K_ON 286
#define K_ORIGIN 287
#define K_PATH 288
#define K_PROVIDER 289
#define K_QUALIFIED 290
#define K_RECEIVER 291
#define K_RESTART 292
#define K_SERIAL 293
#define K_SERVER 294
#define K_SET 295
#define K_STORE 296
#define K_SUBSCRIBE 297
#define K_SUCCESS 298
#define K_TABLE 299
#define K_TRUE 300
#define K_TRY 301
#define K_UNLOCK 302
#define K_UNSUBSCRIBE 303
#define K_YES 304
#define T_IDENT 305
#define T_LITERAL 306
#define T_NUMBER 307
const short yylhs[] = {                                        -1,
    0,    6,    7,    7,    8,    9,    9,   10,   10,   11,
   11,   14,   14,   15,   12,   12,   13,   13,   13,   13,
   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,
   13,   13,   13,   13,   41,   13,   16,   17,   18,    3,
    3,   19,   20,   21,   22,   23,   24,   25,   26,   27,
   29,   28,   30,   31,   32,   33,   34,   35,   35,   37,
   37,   36,   36,   36,   36,   36,   36,   36,   36,   36,
   36,   36,   36,   36,   36,   36,   36,   36,   36,   38,
   39,   40,   40,   42,   42,   42,   43,   43,   43,    1,
    5,    4,    2,
};
const short yylen[] = {                                         2,
    3,    6,    1,    2,    8,    1,    2,    1,    1,    6,
    7,    0,    5,    5,    1,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    0,    3,    4,    4,    5,    1,
    2,    2,    4,    4,    4,    4,    4,    4,    4,    4,
    5,    5,    4,    4,    4,    4,    4,    1,    4,    1,
    3,    3,    4,    3,    3,    3,    4,    4,    3,    3,
    3,    3,    3,    4,    4,    5,    3,    3,    3,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    0,
};
const short yydefred[] = {                                     93,
    0,    0,   93,    0,    0,   93,    0,    0,    0,    0,
    1,    0,    8,    9,   17,   18,   19,    0,   20,   21,
   22,   23,   24,   25,   26,   27,   29,   28,   30,   31,
   32,   33,   34,    4,    0,   90,    0,   42,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    7,   35,   91,    0,    0,    0,    0,
    0,    0,   92,    0,   40,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   93,    0,    0,
   36,    2,    0,   58,    0,   50,   49,   45,   47,   37,
   41,   38,   43,   55,   57,    0,    0,   48,   44,   46,
   53,    0,    0,    0,    0,   56,   54,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   39,   52,   51,
    0,   16,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   10,    0,    5,   80,   65,   81,   72,
   73,   71,    0,   89,   87,   88,   85,   86,   84,   79,
   82,   83,    0,   62,   78,   77,    0,    0,    0,   66,
   70,   69,   64,    0,   61,   59,    0,    0,    0,   11,
   63,    0,   68,   75,   67,   74,   93,   93,   76,    0,
    0,   13,   14,
};
const short yydgoto[] = {                                       1,
  157,   10,   67,  159,   57,    3,    6,    7,   11,   12,
   13,  104,   14,  154,  155,   15,   16,   17,   18,   19,
   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   86,  126,  127,  158,  160,  170,
   81,  171,  172,
};
const short yysindex[] = {                                      0,
    0, -243,    0, -259, -261,    0,    0,  -32, -282, -205,
    0,    0,    0,    0,    0,    0,    0,  -22,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -264,    0, -216,    0, -252, -253,
 -262,  -45, -214, -248, -242, -226, -199, -250, -236, -194,
  -57, -228, -224,    0,    0,    0,   10, -188,  -25,  -25,
  -25,  -25,    0,   17,    0,  -45,   18,  -25,  -25,  -25,
 -282, -221,  -25,  -25,  -25,  -25, -197,    0,  -25,  -25,
    0,    0,   19,    0,   61,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   22,  -25,    0,    0,    0,
    0,  -25,  -74,  -41,    0,    0,    0, -262,   24,   25,
   27,   28, -200,   32, -189,   41,   42, -179, -164, -175,
   53,   55,   56,   57, -155,   76,   81,    0,    0,    0,
 -163,    0,   65, -282, -262, -262, -282,   64, -171, -153,
 -282, -280,   67,   69,   70, -282, -282, -282, -282,   71,
   61,   79, -258,    0, -147,    0,    0,    0,    0,    0,
    0,    0, -282,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   85,    0,    0,    0, -282, -282, -282,    0,
    0,    0,    0, -282,    0,    0,   26,   29, -121,    0,
    0, -262,    0,    0,    0,    0,    0,    0,    0,   15,
   30,    0,    0,
};
const short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0, -247,    0,    0,    0,
    0,    1,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -122,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  109,    0,    0,    0,    0,
   16,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   16,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short yygindex[] = {                                      0,
   -3,    4,   88,  -36,    0,    0,  144,    0,  -10,    0,
    0,   54,  -66,    3,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  172,    0,    9,  -42, -128,    0,
    0,    0,    0,
};
#define YYTABLESIZE 356
const short yytable[] = {                                      66,
    6,   54,   15,    2,   64,   37,    5,  161,    3,  187,
    5,  105,  175,  176,   85,   12,    4,    3,    3,    3,
    8,    9,    3,   60,   36,   63,   73,    3,   35,   61,
    3,    3,   74,   84,   62,   93,   55,   75,  105,  188,
   56,   58,   59,   63,    3,   68,   69,    3,    3,    3,
   38,    3,   70,    3,    3,    3,   71,   72,   76,   39,
   40,   41,   77,  199,   42,   78,   79,   96,   82,   43,
   80,  133,   44,   45,   83,   90,   92,   97,  102,  108,
  128,  103,  138,  131,  134,  135,   46,  136,  137,   47,
   48,   49,  139,   50,  162,   51,   52,   53,  174,  164,
  140,  141,  142,  180,  181,  182,  183,  143,  103,  144,
  165,  145,  166,  146,  167,  147,  148,  149,  150,  151,
  191,  152,  153,  156,  163,    6,  173,  177,  168,  178,
  179,  184,  169,   93,  193,  194,  195,  186,  189,  202,
   12,  196,   93,   93,   93,  192,  187,   93,  197,   60,
   34,  198,   93,   91,  203,   93,   93,  190,  132,  185,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   93,
    0,    0,   93,   93,   93,    0,   93,    0,    0,   93,
   93,   38,    0,    0,    0,    0,  200,  201,    0,    0,
   39,   40,   41,    0,    0,   42,    0,    0,    0,    0,
   43,    0,    0,   44,   45,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   46,    0,    0,
   47,   48,   49,    0,   50,    0,    0,   52,   53,    0,
    0,   87,   88,   89,    0,    0,    0,    0,    0,   93,
   94,   95,    0,    0,   98,   99,  100,  101,    0,    0,
  106,  107,    0,    0,    0,    0,   93,    0,    0,    0,
    0,   65,    0,    0,    0,   93,   93,   93,  129,    0,
   93,   12,    0,  130,    0,   93,    0,    0,   93,   93,
   12,   12,   12,    0,    0,   12,    0,    0,    0,    0,
   12,    0,   93,   12,   12,   93,   93,   93,    0,   93,
    0,   93,   93,   93,    0,    0,    0,   12,    0,    0,
   12,   12,   12,    0,   12,    0,   12,   12,   12,  109,
    0,    0,  110,  111,  112,    0,    0,    0,    0,  113,
    0,    0,  114,  115,  116,    0,  117,    0,    0,    0,
    0,  118,    0,  119,    0,  120,    0,  121,    0,  122,
    0,  123,    0,    0,  124,  125,
};
const short yycheck[] = {                                      45,
    0,   12,  125,    0,   41,    9,    3,  136,  256,  268,
    7,   78,  293,  142,   40,    0,  260,  265,  266,  267,
  280,  283,  270,  277,  307,  306,  277,  275,   61,  283,
  278,  279,  283,   59,  288,  283,   59,  288,  105,  298,
  305,  258,  295,  306,  292,  260,  295,  295,  296,  297,
  256,  299,  295,  301,  302,  303,  283,  257,  295,  265,
  266,  267,  257,  192,  270,  123,  295,   71,   59,  275,
  295,  108,  278,  279,  263,   59,   59,  299,  276,   61,
   59,   78,  283,  125,   61,   61,  292,   61,   61,  295,
  296,  297,   61,  299,  137,  301,  302,  303,  141,  271,
  290,   61,   61,  146,  147,  148,  149,  287,  105,  274,
  282,  287,  284,   61,  286,   61,   61,   61,  274,   44,
  163,   41,  286,   59,   61,  125,  280,   61,  300,   61,
   61,   61,  304,  256,  177,  178,  179,   59,  286,  125,
  125,  184,  265,  266,  267,   61,  268,  270,  123,   41,
    7,  123,  275,   66,  125,  278,  279,  155,  105,  151,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  292,
   -1,   -1,  295,  296,  297,   -1,  299,   -1,   -1,  302,
  303,  256,   -1,   -1,   -1,   -1,  197,  198,   -1,   -1,
  265,  266,  267,   -1,   -1,  270,   -1,   -1,   -1,   -1,
  275,   -1,   -1,  278,  279,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  292,   -1,   -1,
  295,  296,  297,   -1,  299,   -1,   -1,  302,  303,   -1,
   -1,   60,   61,   62,   -1,   -1,   -1,   -1,   -1,   68,
   69,   70,   -1,   -1,   73,   74,   75,   76,   -1,   -1,
   79,   80,   -1,   -1,   -1,   -1,  256,   -1,   -1,   -1,
   -1,  307,   -1,   -1,   -1,  265,  266,  267,   97,   -1,
  270,  256,   -1,  102,   -1,  275,   -1,   -1,  278,  279,
  265,  266,  267,   -1,   -1,  270,   -1,   -1,   -1,   -1,
  275,   -1,  292,  278,  279,  295,  296,  297,   -1,  299,
   -1,  301,  302,  303,   -1,   -1,   -1,  292,   -1,   -1,
  295,  296,  297,   -1,  299,   -1,  301,  302,  303,  259,
   -1,   -1,  262,  263,  264,   -1,   -1,   -1,   -1,  269,
   -1,   -1,  272,  273,  274,   -1,  276,   -1,   -1,   -1,
   -1,  281,   -1,  283,   -1,  285,   -1,  287,   -1,  289,
   -1,  291,   -1,   -1,  294,  295,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 307
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,"'='",
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"K_ADD","K_ADMIN","K_CLIENT","K_CLUSTER","K_CLUSTERNAME","K_COMMENT",
"K_CONNINFO","K_CONNRETRY","K_CREATE","K_DROP","K_ECHO","K_ERROR","K_EVENT",
"K_EXIT","K_FALSE","K_FORWARD","K_FULL","K_ID","K_INIT","K_KEY","K_LISTEN",
"K_LOCK","K_MOVE","K_NAME","K_NEW","K_NO","K_NODE","K_OFF","K_OLD","K_ON",
"K_ORIGIN","K_PATH","K_PROVIDER","K_QUALIFIED","K_RECEIVER","K_RESTART",
"K_SERIAL","K_SERVER","K_SET","K_STORE","K_SUBSCRIBE","K_SUCCESS","K_TABLE",
"K_TRUE","K_TRY","K_UNLOCK","K_UNSUBSCRIBE","K_YES","T_IDENT","T_LITERAL",
"T_NUMBER",
};
const char * const yyrule[] = {
"$accept : script",
"script : hdr_clustername hdr_admconninfos stmts",
"hdr_clustername : lno K_CLUSTER K_NAME '=' ident ';'",
"hdr_admconninfos : hdr_admconninfo",
"hdr_admconninfos : hdr_admconninfo hdr_admconninfos",
"hdr_admconninfo : lno K_NODE id K_ADMIN K_CONNINFO '=' literal ';'",
"stmts : stmt",
"stmts : stmt stmts",
"stmt : stmt_try",
"stmt : try_stmt",
"stmt_try : lno K_TRY '{' try_stmts '}' try_on_error",
"stmt_try : lno K_TRY '{' try_stmts '}' try_on_success try_on_error",
"try_on_error :",
"try_on_error : K_ON K_ERROR '{' stmts '}'",
"try_on_success : K_ON K_SUCCESS '{' stmts '}'",
"try_stmts : try_stmt",
"try_stmts : try_stmt try_stmts",
"try_stmt : stmt_echo",
"try_stmt : stmt_exit",
"try_stmt : stmt_restart_node",
"try_stmt : stmt_init_cluster",
"try_stmt : stmt_store_node",
"try_stmt : stmt_drop_node",
"try_stmt : stmt_store_path",
"try_stmt : stmt_drop_path",
"try_stmt : stmt_store_listen",
"try_stmt : stmt_drop_listen",
"try_stmt : stmt_create_set",
"try_stmt : stmt_table_add_key",
"try_stmt : stmt_set_add_table",
"try_stmt : stmt_subscribe_set",
"try_stmt : stmt_unsubscribe_set",
"try_stmt : stmt_lock_set",
"try_stmt : stmt_unlock_set",
"try_stmt : stmt_move_set",
"$$1 :",
"try_stmt : stmt_error ';' $$1",
"stmt_echo : lno K_ECHO literal ';'",
"stmt_exit : lno K_EXIT exit_code ';'",
"stmt_restart_node : lno K_RESTART K_NODE id ';'",
"exit_code : T_NUMBER",
"exit_code : '-' exit_code",
"stmt_error : lno error",
"stmt_init_cluster : lno K_INIT K_CLUSTER option_list",
"stmt_store_node : lno K_STORE K_NODE option_list",
"stmt_drop_node : lno K_DROP K_NODE option_list",
"stmt_store_path : lno K_STORE K_PATH option_list",
"stmt_drop_path : lno K_DROP K_PATH option_list",
"stmt_store_listen : lno K_STORE K_LISTEN option_list",
"stmt_drop_listen : lno K_DROP K_LISTEN option_list",
"stmt_create_set : lno K_CREATE K_SET option_list",
"stmt_table_add_key : lno K_TABLE K_ADD K_KEY option_list",
"stmt_set_add_table : lno K_SET K_ADD K_TABLE option_list",
"stmt_subscribe_set : lno K_SUBSCRIBE K_SET option_list",
"stmt_unsubscribe_set : lno K_UNSUBSCRIBE K_SET option_list",
"stmt_lock_set : lno K_LOCK K_SET option_list",
"stmt_unlock_set : lno K_UNLOCK K_SET option_list",
"stmt_move_set : lno K_MOVE K_SET option_list",
"option_list : ';'",
"option_list : '(' option_list_items ')' ';'",
"option_list_items : option_list_item",
"option_list_items : option_list_item ',' option_list_items",
"option_list_item : K_ID '=' option_item_id",
"option_list_item : K_EVENT K_NODE '=' option_item_id",
"option_list_item : K_SERVER '=' option_item_id",
"option_list_item : K_CLIENT '=' option_item_id",
"option_list_item : K_ORIGIN '=' option_item_id",
"option_list_item : K_OLD K_ORIGIN '=' option_item_id",
"option_list_item : K_NEW K_ORIGIN '=' option_item_id",
"option_list_item : K_RECEIVER '=' option_item_id",
"option_list_item : K_PROVIDER '=' option_item_id",
"option_list_item : K_CONNRETRY '=' option_item_id",
"option_list_item : K_COMMENT '=' option_item_literal",
"option_list_item : K_CONNINFO '=' option_item_literal",
"option_list_item : K_SET K_ID '=' option_item_id",
"option_list_item : K_NODE K_ID '=' option_item_id",
"option_list_item : K_FULL K_QUALIFIED K_NAME '=' option_item_literal",
"option_list_item : K_KEY '=' option_item_literal",
"option_list_item : K_KEY '=' K_SERIAL",
"option_list_item : K_FORWARD '=' option_item_yn",
"option_item_id : id",
"option_item_literal : literal",
"option_item_yn : option_item_yn_yes",
"option_item_yn : option_item_yn_no",
"option_item_yn_yes : K_YES",
"option_item_yn_yes : K_ON",
"option_item_yn_yes : K_TRUE",
"option_item_yn_no : K_NO",
"option_item_yn_no : K_OFF",
"option_item_yn_no : K_FALSE",
"id : T_NUMBER",
"ident : T_IDENT",
"literal : T_LITERAL",
"lno :",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 1059 "parser.y"


/* ----------
 * option_str
 *
 *	Returns a string representation of a common option type
 * ----------
 */
static char *
option_str(option_code opt_code)
{
	switch (opt_code)
	{
		case O_CLIENT:			return "client";
		case O_COMMENT:			return "comment";
		case O_CONNINFO:		return "conninfo";
		case O_CONNRETRY:		return "connretry";
		case O_EVENT_NODE:		return "event node";
		case O_FQNAME:			return "full qualified name";
		case O_ID:				return "id";
		case O_NODE_ID:			return "node id";
		case O_ORIGIN:			return "origin";
		case O_PROVIDER:		return "provider";
		case O_RECEIVER:		return "receiver";
		case O_SERVER:			return "server";
		case O_SER_KEY:			return "key";
		case O_SET_ID:			return "set id";
		case O_USE_KEY:			return "key";
		case O_FORWARD:			return "forward";
		case O_NEW_ORIGIN:		return "new origin";
		case O_OLD_ORIGIN:		return "old origin";
		case END_OF_OPTIONS:	return "???";
	}
	return "???";
}

/* ----------
 * assign_options
 *
 *	Try to map the actually given options to the statements specific
 *	possible options.
 * ----------
 */
static int
assign_options(statement_option *so, option_list *ol)
{
	statement_option	   *s_opt;
	option_list			   *u_opt;
	int						errors = 0;

	for (u_opt = ol; u_opt; u_opt = u_opt->next)
	{
		for (s_opt = so; s_opt->opt_code >= 0; s_opt++)
		{
			if (s_opt->opt_code == u_opt->opt_code)
				break;
		}

		if (s_opt->opt_code < 0)
		{
			fprintf(stderr, "%s:%d: option %s not allowed here\n",
					current_file, u_opt->lineno,
					option_str(u_opt->opt_code));
			errors++;
			continue;
		}

		if (s_opt->lineno >= 0)
		{
			fprintf(stderr, "%s:%d: option %s already defined on line %d\n",
					current_file, u_opt->lineno,
					option_str(u_opt->opt_code), s_opt->lineno);
			errors++;
			continue;
		}

		s_opt->lineno	= u_opt->lineno;
		s_opt->ival		= u_opt->ival;
		s_opt->str		= u_opt->str;
	}

	return errors;
}


void
yyerror(const char *msg)
{
	fprintf(stderr, "%s:%d: ERROR: %s at or near %s\n", current_file,
				yylineno, msg, yytext);
	parser_errors++;
}


/*
 * Include the output of fles for the scanner here.
 */
#include "scan.c"


#line 600 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 210 "parser.y"
{
						parser_script = (SlonikScript *)
								malloc(sizeof(SlonikScript));
						memset(parser_script, 0, sizeof(SlonikScript));

						parser_script->clustername		= yyvsp[-2].str;
						parser_script->filename			= current_file;
						parser_script->adminfo_list		= yyvsp[-1].adm_info;
						parser_script->script_stmts		= yyvsp[0].statement;
					}
break;
case 2:
#line 223 "parser.y"
{
						yyval.str = yyvsp[-1].str;
					}
break;
case 3:
#line 229 "parser.y"
{ yyval.adm_info = yyvsp[0].adm_info; }
break;
case 4:
#line 231 "parser.y"
{ yyvsp[-1].adm_info->next = yyvsp[0].adm_info; yyval.adm_info = yyvsp[-1].adm_info; }
break;
case 5:
#line 235 "parser.y"
{
						SlonikAdmInfo	   *new;

						new = (SlonikAdmInfo *)
								malloc(sizeof(SlonikAdmInfo));
						memset(new, 0, sizeof(SlonikAdmInfo));

						new->no_id			= yyvsp[-5].ival;
						new->stmt_filename	= current_file;
						new->stmt_lno		= yyvsp[-7].ival;
						new->conninfo		= yyvsp[-1].str;

						yyval.adm_info = new;
					}
break;
case 6:
#line 252 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 7:
#line 254 "parser.y"
{ yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
break;
case 8:
#line 258 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 9:
#line 260 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 10:
#line 264 "parser.y"
{
						SlonikStmt_try *new;

						new = (SlonikStmt_try *)
								malloc(sizeof(SlonikStmt_try));
						memset(new, 0, sizeof(SlonikStmt_try));
						new->hdr.stmt_type		= STMT_TRY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-5].ival;

						new->try_block = yyvsp[-2].statement;
						new->error_block = yyvsp[0].statement;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 11:
#line 281 "parser.y"
{
						SlonikStmt_try *new;

						new = (SlonikStmt_try *)
								malloc(sizeof(SlonikStmt_try));
						memset(new, 0, sizeof(SlonikStmt_try));
						new->hdr.stmt_type		= STMT_TRY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-6].ival;

						new->try_block = yyvsp[-3].statement;
						new->success_block = yyvsp[-1].statement;
						new->error_block = yyvsp[0].statement;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 12:
#line 300 "parser.y"
{ yyval.statement = NULL; }
break;
case 13:
#line 302 "parser.y"
{ yyval.statement = yyvsp[-1].statement; }
break;
case 14:
#line 305 "parser.y"
{ yyval.statement = yyvsp[-1].statement; }
break;
case 15:
#line 309 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 16:
#line 311 "parser.y"
{ yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
break;
case 17:
#line 315 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 18:
#line 317 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 19:
#line 319 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 20:
#line 321 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 21:
#line 323 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 22:
#line 325 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 23:
#line 327 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 24:
#line 329 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 25:
#line 331 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 26:
#line 333 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 27:
#line 335 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 28:
#line 337 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 29:
#line 339 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 30:
#line 341 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 31:
#line 343 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 32:
#line 345 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 33:
#line 347 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 34:
#line 349 "parser.y"
{ yyval.statement = yyvsp[0].statement; }
break;
case 35:
#line 350 "parser.y"
{ yyerrok; }
break;
case 36:
#line 351 "parser.y"
{ yyval.statement = yyvsp[-2].statement; }
break;
case 37:
#line 355 "parser.y"
{
						SlonikStmt_echo *new;

						new = (SlonikStmt_echo *)
								malloc(sizeof(SlonikStmt_echo));
						memset(new, 0, sizeof(SlonikStmt_echo));
						new->hdr.stmt_type		= STMT_ECHO;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						new->str = yyvsp[-1].str;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 38:
#line 372 "parser.y"
{
						SlonikStmt_exit *new;

						new = (SlonikStmt_exit *)
								malloc(sizeof(SlonikStmt_exit));
						memset(new, 0, sizeof(SlonikStmt_exit));
						new->hdr.stmt_type		= STMT_EXIT;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						new->exitcode = yyvsp[-1].ival;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 39:
#line 389 "parser.y"
{
						SlonikStmt_restart_node *new;

						new = (SlonikStmt_restart_node *)
								malloc(sizeof(SlonikStmt_restart_node));
						memset(new, 0, sizeof(SlonikStmt_restart_node));
						new->hdr.stmt_type		= STMT_RESTART_NODE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						new->no_id = yyvsp[-1].ival;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 40:
#line 406 "parser.y"
{ yyval.ival = strtol(yytext, NULL, 10); }
break;
case 41:
#line 408 "parser.y"
{ yyval.ival = -yyvsp[0].ival; }
break;
case 42:
#line 412 "parser.y"
{
						SlonikStmt *new;

						new = (SlonikStmt *)
								malloc(sizeof(SlonikStmt));
						memset(new, 0, sizeof(SlonikStmt));
						new->stmt_type		= STMT_ERROR;
						new->stmt_filename	= current_file;
						new->stmt_lno		= yyvsp[-1].ival;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 43:
#line 427 "parser.y"
{
						SlonikStmt_init_cluster *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, 1 ),
							STMT_OPTION_STR( O_COMMENT, "Primary Node 1" ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_init_cluster *)
								malloc(sizeof(SlonikStmt_init_cluster));
						memset(new, 0, sizeof(SlonikStmt_init_cluster));
						new->hdr.stmt_type		= STMT_INIT_CLUSTER;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
							new->no_comment		= opt[1].str;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 44:
#line 455 "parser.y"
{
						SlonikStmt_store_node *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_STR( O_COMMENT, NULL ),
							STMT_OPTION_INT( O_EVENT_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_store_node *)
								malloc(sizeof(SlonikStmt_store_node));
						memset(new, 0, sizeof(SlonikStmt_store_node));
						new->hdr.stmt_type		= STMT_STORE_NODE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
							new->no_comment		= opt[1].str;
							new->ev_origin		= opt[2].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 45:
#line 483 "parser.y"
{
						SlonikStmt_drop_node *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_EVENT_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_drop_node *)
								malloc(sizeof(SlonikStmt_drop_node));
						memset(new, 0, sizeof(SlonikStmt_drop_node));
						new->hdr.stmt_type		= STMT_DROP_NODE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
							new->ev_origin		= opt[1].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 46:
#line 509 "parser.y"
{
						SlonikStmt_store_path *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_SERVER, -1 ),
							STMT_OPTION_INT( O_CLIENT, -1 ),
							STMT_OPTION_STR( O_CONNINFO, NULL ),
							STMT_OPTION_INT( O_CONNRETRY, 10 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_store_path *)
								malloc(sizeof(SlonikStmt_store_path));
						memset(new, 0, sizeof(SlonikStmt_store_path));
						new->hdr.stmt_type		= STMT_STORE_PATH;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->pa_server		= opt[0].ival;
							new->pa_client		= opt[1].ival;
							new->pa_conninfo	= opt[2].str;
							new->pa_connretry	= opt[3].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 47:
#line 539 "parser.y"
{
						SlonikStmt_drop_path *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_SERVER, -1 ),
							STMT_OPTION_INT( O_CLIENT, -1 ),
							STMT_OPTION_INT( O_EVENT_NODE, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_drop_path *)
								malloc(sizeof(SlonikStmt_drop_path));
						memset(new, 0, sizeof(SlonikStmt_drop_path));
						new->hdr.stmt_type		= STMT_DROP_PATH;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->pa_server		= opt[0].ival;
							new->pa_client		= opt[1].ival;
							new->ev_origin		= opt[2].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 48:
#line 567 "parser.y"
{
						SlonikStmt_store_listen *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_RECEIVER, -1 ),
							STMT_OPTION_INT( O_PROVIDER, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_store_listen *)
								malloc(sizeof(SlonikStmt_store_listen));
						memset(new, 0, sizeof(SlonikStmt_store_listen));
						new->hdr.stmt_type		= STMT_STORE_LISTEN;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->li_origin		= opt[0].ival;
							new->li_receiver	= opt[1].ival;
							new->li_provider	= opt[2].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 49:
#line 595 "parser.y"
{
						SlonikStmt_drop_listen *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_RECEIVER, -1 ),
							STMT_OPTION_INT( O_PROVIDER, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_drop_listen *)
								malloc(sizeof(SlonikStmt_drop_listen));
						memset(new, 0, sizeof(SlonikStmt_drop_listen));
						new->hdr.stmt_type		= STMT_DROP_LISTEN;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->li_origin		= opt[0].ival;
							new->li_receiver	= opt[1].ival;
							new->li_provider	= opt[2].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 50:
#line 623 "parser.y"
{
						SlonikStmt_create_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_STR( O_COMMENT, NULL ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_create_set *)
								malloc(sizeof(SlonikStmt_create_set));
						memset(new, 0, sizeof(SlonikStmt_create_set));
						new->hdr.stmt_type		= STMT_CREATE_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
							new->set_comment	= opt[2].str;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 51:
#line 651 "parser.y"
{
						SlonikStmt_table_add_key *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_NODE_ID, -1 ),
							STMT_OPTION_STR( O_FQNAME, NULL ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_table_add_key *)
								malloc(sizeof(SlonikStmt_table_add_key));
						memset(new, 0, sizeof(SlonikStmt_table_add_key));
						new->hdr.stmt_type		= STMT_TABLE_ADD_KEY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
							new->tab_fqname		= opt[1].str;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 52:
#line 677 "parser.y"
{
						SlonikStmt_set_add_table *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_SET_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_STR( O_FQNAME, NULL ),
							STMT_OPTION_STR( O_USE_KEY, NULL ),
							STMT_OPTION_INT( O_SER_KEY, 0 ),
							STMT_OPTION_STR( O_COMMENT, NULL ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_set_add_table *)
								malloc(sizeof(SlonikStmt_set_add_table));
						memset(new, 0, sizeof(SlonikStmt_set_add_table));
						new->hdr.stmt_type		= STMT_SET_ADD_TABLE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
							new->tab_id			= opt[2].ival;
							new->tab_fqname		= opt[3].str;
							new->use_key		= opt[4].str;
							new->use_serial		= opt[5].ival;
							new->tab_comment	= opt[6].str;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 53:
#line 713 "parser.y"
{
						SlonikStmt_subscribe_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_PROVIDER, -1 ),
							STMT_OPTION_INT( O_RECEIVER, -1 ),
							STMT_OPTION_YN( O_FORWARD, 0 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_subscribe_set *)
								malloc(sizeof(SlonikStmt_subscribe_set));
						memset(new, 0, sizeof(SlonikStmt_subscribe_set));
						new->hdr.stmt_type		= STMT_SUBSCRIBE_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->sub_setid		= opt[0].ival;
							new->sub_provider	= opt[1].ival;
							new->sub_receiver	= opt[2].ival;
							new->sub_forward	= opt[3].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 54:
#line 743 "parser.y"
{
						SlonikStmt_unsubscribe_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_RECEIVER, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_unsubscribe_set *)
								malloc(sizeof(SlonikStmt_unsubscribe_set));
						memset(new, 0, sizeof(SlonikStmt_unsubscribe_set));
						new->hdr.stmt_type		= STMT_UNSUBSCRIBE_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->sub_setid		= opt[0].ival;
							new->sub_receiver	= opt[1].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 55:
#line 769 "parser.y"
{
						SlonikStmt_lock_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_lock_set *)
								malloc(sizeof(SlonikStmt_lock_set));
						memset(new, 0, sizeof(SlonikStmt_lock_set));
						new->hdr.stmt_type		= STMT_LOCK_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 56:
#line 795 "parser.y"
{
						SlonikStmt_unlock_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_unlock_set *)
								malloc(sizeof(SlonikStmt_unlock_set));
						memset(new, 0, sizeof(SlonikStmt_unlock_set));
						new->hdr.stmt_type		= STMT_UNLOCK_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 57:
#line 821 "parser.y"
{
						SlonikStmt_move_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_OLD_ORIGIN, -1 ),
							STMT_OPTION_INT( O_NEW_ORIGIN, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_move_set *)
								malloc(sizeof(SlonikStmt_move_set));
						memset(new, 0, sizeof(SlonikStmt_move_set));
						new->hdr.stmt_type		= STMT_MOVE_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->old_origin		= opt[1].ival;
							new->new_origin		= opt[2].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
break;
case 58:
#line 849 "parser.y"
{ yyval.opt_list = NULL; }
break;
case 59:
#line 851 "parser.y"
{ yyval.opt_list = yyvsp[-2].opt_list; }
break;
case 60:
#line 855 "parser.y"
{ yyval.opt_list = yyvsp[0].opt_list; }
break;
case 61:
#line 857 "parser.y"
{ yyvsp[-2].opt_list->next = yyvsp[0].opt_list; yyval.opt_list = yyvsp[-2].opt_list; }
break;
case 62:
#line 861 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 63:
#line 866 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_EVENT_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 64:
#line 871 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_SERVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 65:
#line 876 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_CLIENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 66:
#line 881 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 67:
#line 886 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_OLD_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 68:
#line 891 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_NEW_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 69:
#line 896 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_RECEIVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 70:
#line 901 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_PROVIDER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 71:
#line 906 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_CONNRETRY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 72:
#line 911 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_COMMENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 73:
#line 916 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_CONNINFO;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 74:
#line 921 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_SET_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 75:
#line 926 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_NODE_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 76:
#line 931 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_FQNAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 77:
#line 936 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_USE_KEY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 78:
#line 941 "parser.y"
{
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->opt_code	= O_SER_KEY;
						new->ival	= 1;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
break;
case 79:
#line 954 "parser.y"
{
						yyvsp[0].opt_list->opt_code	= O_FORWARD;
						yyval.opt_list = yyvsp[0].opt_list;
					}
break;
case 80:
#line 961 "parser.y"
{
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->ival	= yyvsp[0].ival;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
break;
case 81:
#line 975 "parser.y"
{
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->ival	= -1;
						new->str	= yyvsp[0].str;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
break;
case 82:
#line 989 "parser.y"
{
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->ival	= 1;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
break;
case 83:
#line 1001 "parser.y"
{
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->ival	= 0;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
break;
case 90:
#line 1025 "parser.y"
{
						yyval.ival = strtol(yytext, NULL, 10);
					}
break;
case 91:
#line 1031 "parser.y"
{
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
break;
case 92:
#line 1043 "parser.y"
{
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
break;
case 93:
#line 1055 "parser.y"
{ yyval.ival = yylineno; }
break;
#line 1728 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
