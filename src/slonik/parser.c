/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     K_ADD = 258,
     K_ADMIN = 259,
     K_BACKUP = 260,
     K_CLIENT = 261,
     K_CLUSTER = 262,
     K_CLUSTERNAME = 263,
     K_COMMENT = 264,
     K_CONNINFO = 265,
     K_CONNRETRY = 266,
     K_CREATE = 267,
     K_DROP = 268,
     K_ECHO = 269,
     K_ERROR = 270,
     K_EVENT = 271,
     K_EXIT = 272,
     K_FAILOVER = 273,
     K_FALSE = 274,
     K_FORWARD = 275,
     K_FULL = 276,
     K_ID = 277,
     K_INIT = 278,
     K_KEY = 279,
     K_LISTEN = 280,
     K_LOCK = 281,
     K_MOVE = 282,
     K_NAME = 283,
     K_NEW = 284,
     K_NO = 285,
     K_NODE = 286,
     K_OFF = 287,
     K_OLD = 288,
     K_ON = 289,
     K_ORIGIN = 290,
     K_PATH = 291,
     K_PROVIDER = 292,
     K_QUALIFIED = 293,
     K_RECEIVER = 294,
     K_RESTART = 295,
     K_SEQUENCE = 296,
     K_SERIAL = 297,
     K_SERVER = 298,
     K_SET = 299,
     K_STORE = 300,
     K_SUBSCRIBE = 301,
     K_SUCCESS = 302,
     K_TABLE = 303,
     K_TRUE = 304,
     K_TRY = 305,
     K_UNINSTALL = 306,
     K_UNLOCK = 307,
     K_UNSUBSCRIBE = 308,
     K_YES = 309,
     T_IDENT = 310,
     T_LITERAL = 311,
     T_NUMBER = 312
   };
#endif
#define K_ADD 258
#define K_ADMIN 259
#define K_BACKUP 260
#define K_CLIENT 261
#define K_CLUSTER 262
#define K_CLUSTERNAME 263
#define K_COMMENT 264
#define K_CONNINFO 265
#define K_CONNRETRY 266
#define K_CREATE 267
#define K_DROP 268
#define K_ECHO 269
#define K_ERROR 270
#define K_EVENT 271
#define K_EXIT 272
#define K_FAILOVER 273
#define K_FALSE 274
#define K_FORWARD 275
#define K_FULL 276
#define K_ID 277
#define K_INIT 278
#define K_KEY 279
#define K_LISTEN 280
#define K_LOCK 281
#define K_MOVE 282
#define K_NAME 283
#define K_NEW 284
#define K_NO 285
#define K_NODE 286
#define K_OFF 287
#define K_OLD 288
#define K_ON 289
#define K_ORIGIN 290
#define K_PATH 291
#define K_PROVIDER 292
#define K_QUALIFIED 293
#define K_RECEIVER 294
#define K_RESTART 295
#define K_SEQUENCE 296
#define K_SERIAL 297
#define K_SERVER 298
#define K_SET 299
#define K_STORE 300
#define K_SUBSCRIBE 301
#define K_SUCCESS 302
#define K_TABLE 303
#define K_TRUE 304
#define K_TRY 305
#define K_UNINSTALL 306
#define K_UNLOCK 307
#define K_UNSUBSCRIBE 308
#define K_YES 309
#define T_IDENT 310
#define T_LITERAL 311
#define T_NUMBER 312




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

/*-------------------------------------------------------------------------
 * parser.y
 *
 *	The slonik command language grammar
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: parser.c,v 1.5 2004-04-13 20:00:20 wieck Exp $
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "libpq-fe.h"
#include "slonik.h"


/*
 * Common option types
 */
typedef enum {
	O_BACKUP_NODE,
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
extern int yyleng;


/*
 * Local functions
 */
static int	assign_options(statement_option *so, option_list *ol);





/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 94 "parser.y"
typedef union YYSTYPE {
	int32			ival;
	char		   *str;
	option_list	   *opt_list;
	SlonikAdmInfo  *adm_info;
	SlonikStmt	   *statement;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 287 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 299 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   206

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  48
/* YYNRULES -- Number of rules. */
#define YYNRULES  103
/* YYNRULES -- Number of states. */
#define YYNSTATES  221

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   312

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      63,    64,     2,     2,    65,    62,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    59,
       2,    58,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    60,     2,    61,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     7,    14,    16,    19,    28,    30,    33,
      35,    37,    44,    52,    60,    67,    73,    79,    85,    87,
      90,    92,    94,    96,    98,   100,   102,   104,   106,   108,
     110,   112,   114,   116,   118,   120,   122,   124,   126,   128,
     130,   132,   133,   137,   142,   147,   153,   155,   158,   161,
     166,   171,   176,   180,   185,   190,   195,   200,   205,   210,
     216,   222,   228,   233,   238,   243,   248,   253,   255,   260,
     262,   266,   270,   275,   280,   284,   288,   292,   297,   302,
     306,   310,   314,   318,   322,   327,   332,   338,   342,   346,
     350,   352,   354,   356,   358,   360,   362,   364,   366,   368,
     370,   372,   374,   376
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      67,     0,    -1,    68,    69,    71,    -1,   113,     7,    28,
      58,   111,    59,    -1,    70,    -1,    70,    69,    -1,   113,
      31,   110,     4,    10,    58,   112,    59,    -1,    72,    -1,
      72,    71,    -1,    73,    -1,    77,    -1,   113,    50,    60,
      76,    61,    74,    -1,   113,    50,    60,    76,    61,    75,
      74,    -1,   113,    50,    60,    76,    61,    74,    75,    -1,
     113,    50,    60,    76,    61,    75,    -1,   113,    50,    60,
      76,    61,    -1,    34,    15,    60,    71,    61,    -1,    34,
      47,    60,    71,    61,    -1,    77,    -1,    77,    76,    -1,
      79,    -1,    80,    -1,    81,    -1,    84,    -1,    85,    -1,
      86,    -1,    87,    -1,    88,    -1,    89,    -1,    90,    -1,
      91,    -1,    92,    -1,    93,    -1,    94,    -1,    95,    -1,
      96,    -1,    97,    -1,    98,    -1,    99,    -1,   100,    -1,
     101,    -1,    -1,    83,    59,    78,    -1,   113,    14,   112,
      59,    -1,   113,    17,    82,    59,    -1,   113,    40,    31,
     110,    59,    -1,    57,    -1,    62,    82,    -1,   113,     1,
      -1,   113,    23,     7,   102,    -1,   113,    45,    31,   102,
      -1,   113,    13,    31,   102,    -1,   113,    18,   102,    -1,
     113,    51,    31,   102,    -1,   113,    45,    36,   102,    -1,
     113,    13,    36,   102,    -1,   113,    45,    25,   102,    -1,
     113,    13,    25,   102,    -1,   113,    12,    44,   102,    -1,
     113,    48,     3,    24,   102,    -1,   113,    44,     3,    48,
     102,    -1,   113,    44,     3,    41,   102,    -1,   113,    46,
      44,   102,    -1,   113,    53,    44,   102,    -1,   113,    26,
      44,   102,    -1,   113,    52,    44,   102,    -1,   113,    27,
      44,   102,    -1,    59,    -1,    63,   103,    64,    59,    -1,
     104,    -1,   104,    65,   103,    -1,    22,    58,   105,    -1,
       5,    31,    58,   105,    -1,    16,    31,    58,   105,    -1,
      43,    58,   105,    -1,     6,    58,   105,    -1,    35,    58,
     105,    -1,    33,    35,    58,   105,    -1,    29,    35,    58,
     105,    -1,    39,    58,   105,    -1,    37,    58,   105,    -1,
      11,    58,   105,    -1,     9,    58,   106,    -1,    10,    58,
     106,    -1,    44,    22,    58,   105,    -1,    31,    22,    58,
     105,    -1,    21,    38,    28,    58,   106,    -1,    24,    58,
     106,    -1,    24,    58,    42,    -1,    20,    58,   107,    -1,
     110,    -1,   112,    -1,   108,    -1,   109,    -1,    54,    -1,
      34,    -1,    49,    -1,    30,    -1,    32,    -1,    19,    -1,
      57,    -1,    55,    -1,    56,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   216,   216,   231,   237,   239,   243,   260,   262,   266,
     268,   272,   289,   307,   325,   342,   359,   362,   366,   368,
     372,   374,   376,   378,   380,   382,   384,   386,   388,   390,
     392,   394,   396,   398,   400,   402,   404,   406,   408,   410,
     412,   414,   414,   418,   435,   452,   469,   471,   475,   490,
     518,   546,   572,   598,   622,   652,   680,   708,   736,   764,
     790,   826,   858,   888,   914,   940,   966,   994,   996,  1000,
    1002,  1006,  1011,  1016,  1021,  1026,  1031,  1036,  1041,  1046,
    1051,  1056,  1061,  1066,  1071,  1076,  1081,  1086,  1091,  1104,
    1111,  1125,  1139,  1151,  1165,  1166,  1167,  1170,  1171,  1172,
    1175,  1181,  1193,  1206
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "K_ADD", "K_ADMIN", "K_BACKUP", "K_CLIENT", 
  "K_CLUSTER", "K_CLUSTERNAME", "K_COMMENT", "K_CONNINFO", "K_CONNRETRY", 
  "K_CREATE", "K_DROP", "K_ECHO", "K_ERROR", "K_EVENT", "K_EXIT", 
  "K_FAILOVER", "K_FALSE", "K_FORWARD", "K_FULL", "K_ID", "K_INIT", 
  "K_KEY", "K_LISTEN", "K_LOCK", "K_MOVE", "K_NAME", "K_NEW", "K_NO", 
  "K_NODE", "K_OFF", "K_OLD", "K_ON", "K_ORIGIN", "K_PATH", "K_PROVIDER", 
  "K_QUALIFIED", "K_RECEIVER", "K_RESTART", "K_SEQUENCE", "K_SERIAL", 
  "K_SERVER", "K_SET", "K_STORE", "K_SUBSCRIBE", "K_SUCCESS", "K_TABLE", 
  "K_TRUE", "K_TRY", "K_UNINSTALL", "K_UNLOCK", "K_UNSUBSCRIBE", "K_YES", 
  "T_IDENT", "T_LITERAL", "T_NUMBER", "'='", "';'", "'{'", "'}'", "'-'", 
  "'('", "')'", "','", "$accept", "script", "hdr_clustername", 
  "hdr_admconninfos", "hdr_admconninfo", "stmts", "stmt", "stmt_try", 
  "try_on_error", "try_on_success", "try_stmts", "try_stmt", "@1", 
  "stmt_echo", "stmt_exit", "stmt_restart_node", "exit_code", 
  "stmt_error", "stmt_init_cluster", "stmt_store_node", "stmt_drop_node", 
  "stmt_failed_node", "stmt_uninstall_node", "stmt_store_path", 
  "stmt_drop_path", "stmt_store_listen", "stmt_drop_listen", 
  "stmt_create_set", "stmt_table_add_key", "stmt_set_add_table", 
  "stmt_set_add_sequence", "stmt_subscribe_set", "stmt_unsubscribe_set", 
  "stmt_lock_set", "stmt_unlock_set", "stmt_move_set", "option_list", 
  "option_list_items", "option_list_item", "option_item_id", 
  "option_item_literal", "option_item_yn", "option_item_yn_yes", 
  "option_item_yn_no", "id", "ident", "literal", "lno", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,    61,    59,
     123,   125,    45,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    66,    67,    68,    69,    69,    70,    71,    71,    72,
      72,    73,    73,    73,    73,    73,    74,    75,    76,    76,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    78,    77,    79,    80,    81,    82,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   102,   103,
     103,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     105,   106,   107,   107,   108,   108,   108,   109,   109,   109,
     110,   111,   112,   113
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     6,     1,     2,     8,     1,     2,     1,
       1,     6,     7,     7,     6,     5,     5,     5,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     3,     4,     4,     5,     1,     2,     2,     4,
       4,     4,     3,     4,     4,     4,     4,     4,     4,     5,
       5,     5,     4,     4,     4,     4,     4,     1,     4,     1,
       3,     3,     4,     4,     3,     3,     3,     4,     4,     3,
       3,     3,     3,     3,     4,     4,     5,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     103,     0,   103,     0,     1,   103,     4,     0,     0,     2,
     103,     9,    10,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     5,     0,     0,     8,
      41,    48,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   100,
       0,     0,    42,     0,     0,     0,     0,   102,     0,    46,
       0,     0,    67,     0,    52,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   103,     0,     0,     0,     0,
     101,     0,    58,    57,    51,    55,    43,    47,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    69,    49,
      64,    66,     0,     0,     0,    56,    50,    54,    62,     0,
       0,   103,     0,    53,    65,    63,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,    61,
      60,    59,    15,    19,     0,     0,    75,    90,    82,    91,
      83,    81,     0,    99,    97,    98,    95,    96,    94,    89,
      92,    93,     0,    71,    88,    87,     0,     0,     0,    76,
      80,    79,    74,     0,    68,    70,     0,    11,    14,     0,
      72,    73,     0,    78,    85,    77,    84,     0,     0,     0,
      13,     0,    12,     6,    86,   103,   103,     0,     0,    16,
      17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     5,     6,     9,    10,    11,   197,   198,
     130,    12,    62,    13,    14,    15,    71,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    74,   117,   118,   166,
     168,   179,   180,   181,   167,    91,   169,    35
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -136
static const short yypact[] =
{
    -136,    17,  -136,    16,  -136,  -136,     5,     8,    15,  -136,
       4,  -136,  -136,  -136,  -136,  -136,   -15,  -136,  -136,  -136,
    -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,
    -136,  -136,  -136,  -136,  -136,    37,  -136,    -9,    -5,  -136,
    -136,  -136,    12,    -7,    -4,   -43,   -22,    50,    14,    18,
      30,    63,     9,    26,    68,    13,    41,    31,    32,  -136,
      74,    24,  -136,   -22,   -22,   -22,   -22,  -136,    21,  -136,
     -43,    33,  -136,   135,  -136,   -22,   -22,   -22,    -9,    -6,
     -22,   -22,   -22,   -22,    67,  -136,   -22,   -22,   -22,    84,
    -136,    34,  -136,  -136,  -136,  -136,  -136,  -136,  -136,    71,
      40,    46,    47,    53,    77,    58,    61,    59,    60,    85,
      97,    86,    66,    72,    79,    80,   103,    75,    78,  -136,
    -136,  -136,    93,   -22,   -22,  -136,  -136,  -136,  -136,   -22,
      81,    92,    83,  -136,  -136,  -136,   100,  -136,   102,    -9,
      -4,    -4,    -9,   104,   131,   139,    -9,   -26,   111,   113,
     115,    -9,    -9,    -9,    -9,   117,   118,   135,  -136,  -136,
    -136,  -136,   142,  -136,    -4,    -9,  -136,  -136,  -136,  -136,
    -136,  -136,    -9,  -136,  -136,  -136,  -136,  -136,  -136,  -136,
    -136,  -136,   123,  -136,  -136,  -136,    -9,    -9,    -9,  -136,
    -136,  -136,  -136,    -9,  -136,  -136,     0,   148,   149,   125,
    -136,  -136,    -4,  -136,  -136,  -136,  -136,   126,   127,   141,
    -136,   174,  -136,  -136,  -136,  -136,  -136,   129,   130,  -136,
    -136
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -136,  -136,  -136,   186,  -136,   -10,  -136,  -136,    -3,    -1,
      62,   -72,  -136,  -136,  -136,  -136,   124,  -136,  -136,  -136,
    -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,  -136,
    -136,  -136,  -136,  -136,  -136,  -136,   -55,    42,  -136,   -39,
    -135,  -136,  -136,  -136,   -32,  -136,   -42,     1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -104
static const short yytable[] =
{
      39,     3,    68,     7,    -7,    60,   170,     7,    92,    93,
      94,    95,   185,   131,    69,   207,   184,     4,    64,    70,
     119,   120,   121,     8,    65,   125,   126,   127,   128,    66,
      67,   133,   134,   135,    80,   123,  -103,    72,    41,    37,
      81,    73,   124,    38,    40,    82,   122,   208,    59,    42,
      43,    44,    67,    61,    45,    46,    63,    75,    76,   131,
      47,    78,    77,    48,    49,    -7,    79,   214,   159,   160,
      83,    84,    86,    85,   161,    87,    88,    50,    89,    90,
      96,    51,    52,    53,    41,    54,   132,    55,    56,    57,
      58,   129,    98,   137,   136,    42,    43,    44,   139,   145,
      45,    46,   138,   171,   140,   141,    47,   183,   143,    48,
      49,   142,   189,   190,   191,   192,   144,   146,   147,   149,
     148,   150,   199,    50,   151,   155,   200,    51,    52,    53,
     152,    54,   132,   201,    56,    57,    58,   153,   154,   156,
      99,   100,   162,   157,   101,   102,   103,   203,   204,   205,
     173,   104,   158,   -18,   206,   105,   106,   107,   164,   108,
     165,   174,   172,   175,   109,   176,   110,   182,   111,   186,
     112,   187,   113,   188,   114,   193,   196,   194,   115,   116,
     177,   202,   209,   211,   213,   178,   215,   216,   208,   207,
     219,   220,    36,   163,    97,   212,   210,     0,     0,   195,
       0,     0,     0,     0,     0,   217,   218
};

static const short yycheck[] =
{
      10,     0,    44,     2,     0,    37,   141,     6,    63,    64,
      65,    66,   147,    85,    57,    15,    42,     0,    25,    62,
      75,    76,    77,     7,    31,    80,    81,    82,    83,    36,
      56,    86,    87,    88,    25,    41,    31,    59,     1,    31,
      31,    63,    48,    28,    59,    36,    78,    47,    57,    12,
      13,    14,    56,    58,    17,    18,    44,     7,    44,   131,
      23,    31,    44,    26,    27,    61,     3,   202,   123,   124,
      44,     3,    31,    60,   129,    44,    44,    40,     4,    55,
      59,    44,    45,    46,     1,    48,    85,    50,    51,    52,
      53,    24,    59,    59,    10,    12,    13,    14,    58,    38,
      17,    18,    31,   142,    58,    58,    23,   146,    31,    26,
      27,    58,   151,   152,   153,   154,    58,    58,    58,    22,
      35,    35,   164,    40,    58,    22,   165,    44,    45,    46,
      58,    48,   131,   172,    51,    52,    53,    58,    58,    64,
       5,     6,    61,    65,     9,    10,    11,   186,   187,   188,
      19,    16,    59,    61,   193,    20,    21,    22,    58,    24,
      58,    30,    58,    32,    29,    34,    31,    28,    33,    58,
      35,    58,    37,    58,    39,    58,    34,    59,    43,    44,
      49,    58,    34,    34,    59,    54,    60,    60,    47,    15,
      61,    61,     6,   131,    70,   198,   197,    -1,    -1,   157,
      -1,    -1,    -1,    -1,    -1,   215,   216
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    67,    68,   113,     0,    69,    70,   113,     7,    71,
      72,    73,    77,    79,    80,    81,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   113,    69,    31,    28,    71,
      59,     1,    12,    13,    14,    17,    18,    23,    26,    27,
      40,    44,    45,    46,    48,    50,    51,    52,    53,    57,
     110,    58,    78,    44,    25,    31,    36,    56,   112,    57,
      62,    82,    59,    63,   102,     7,    44,    44,    31,     3,
      25,    31,    36,    44,     3,    60,    31,    44,    44,     4,
      55,   111,   102,   102,   102,   102,    59,    82,    59,     5,
       6,     9,    10,    11,    16,    20,    21,    22,    24,    29,
      31,    33,    35,    37,    39,    43,    44,   103,   104,   102,
     102,   102,   110,    41,    48,   102,   102,   102,   102,    24,
      76,    77,   113,   102,   102,   102,    10,    59,    31,    58,
      58,    58,    58,    31,    58,    38,    58,    58,    35,    22,
      35,    58,    58,    58,    58,    22,    64,    65,    59,   102,
     102,   102,    61,    76,    58,    58,   105,   110,   106,   112,
     106,   105,    58,    19,    30,    32,    34,    49,    54,   107,
     108,   109,    28,   105,    42,   106,    58,    58,    58,   105,
     105,   105,   105,    58,    59,   103,    34,    74,    75,   112,
     105,   105,    58,   105,   105,   105,   105,    15,    47,    34,
      75,    34,    74,    59,   106,    60,    60,    71,    71,    61,
      61
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 219 "parser.y"
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

  case 3:
#line 232 "parser.y"
    {
						yyval.str = yyvsp[-1].str;
					}
    break;

  case 4:
#line 238 "parser.y"
    { yyval.adm_info = yyvsp[0].adm_info; }
    break;

  case 5:
#line 240 "parser.y"
    { yyvsp[-1].adm_info->next = yyvsp[0].adm_info; yyval.adm_info = yyvsp[-1].adm_info; }
    break;

  case 6:
#line 244 "parser.y"
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

  case 7:
#line 261 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 8:
#line 263 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 9:
#line 267 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 10:
#line 269 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 11:
#line 274 "parser.y"
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

  case 12:
#line 291 "parser.y"
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

  case 13:
#line 309 "parser.y"
    {
						SlonikStmt_try *new;

						new = (SlonikStmt_try *)
								malloc(sizeof(SlonikStmt_try));
						memset(new, 0, sizeof(SlonikStmt_try));
						new->hdr.stmt_type		= STMT_TRY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-6].ival;

						new->try_block = yyvsp[-3].statement;
						new->success_block = yyvsp[0].statement;
						new->error_block = yyvsp[-1].statement;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 14:
#line 327 "parser.y"
    {
						SlonikStmt_try *new;

						new = (SlonikStmt_try *)
								malloc(sizeof(SlonikStmt_try));
						memset(new, 0, sizeof(SlonikStmt_try));
						new->hdr.stmt_type		= STMT_TRY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-5].ival;

						new->try_block = yyvsp[-2].statement;
						new->success_block = yyvsp[0].statement;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 15:
#line 343 "parser.y"
    {
						SlonikStmt_try *new;

						new = (SlonikStmt_try *)
								malloc(sizeof(SlonikStmt_try));
						memset(new, 0, sizeof(SlonikStmt_try));
						new->hdr.stmt_type		= STMT_TRY;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						new->try_block = yyvsp[-1].statement;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 16:
#line 360 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 17:
#line 363 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 18:
#line 367 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 19:
#line 369 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 20:
#line 373 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 21:
#line 375 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 22:
#line 377 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 23:
#line 379 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 24:
#line 381 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 25:
#line 383 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 26:
#line 385 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 27:
#line 387 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 28:
#line 389 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 29:
#line 391 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 30:
#line 393 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 31:
#line 395 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 32:
#line 397 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 33:
#line 399 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 34:
#line 401 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 35:
#line 403 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 36:
#line 405 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 37:
#line 407 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 38:
#line 409 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 39:
#line 411 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 40:
#line 413 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 41:
#line 414 "parser.y"
    { yyerrok; }
    break;

  case 42:
#line 415 "parser.y"
    { yyval.statement = yyvsp[-2].statement; }
    break;

  case 43:
#line 419 "parser.y"
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

  case 44:
#line 436 "parser.y"
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

  case 45:
#line 453 "parser.y"
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

  case 46:
#line 470 "parser.y"
    { yyval.ival = strtol(yytext, NULL, 10); }
    break;

  case 47:
#line 472 "parser.y"
    { yyval.ival = -yyvsp[0].ival; }
    break;

  case 48:
#line 476 "parser.y"
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

  case 49:
#line 491 "parser.y"
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

  case 50:
#line 519 "parser.y"
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

  case 51:
#line 547 "parser.y"
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

  case 52:
#line 573 "parser.y"
    {
						SlonikStmt_failed_node *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_BACKUP_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_failed_node *)
								malloc(sizeof(SlonikStmt_failed_node));
						memset(new, 0, sizeof(SlonikStmt_failed_node));
						new->hdr.stmt_type		= STMT_FAILED_NODE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-2].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
							new->backup_node	= opt[1].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 53:
#line 599 "parser.y"
    {
						SlonikStmt_uninstall_node *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_uninstall_node *)
								malloc(sizeof(SlonikStmt_uninstall_node));
						memset(new, 0, sizeof(SlonikStmt_uninstall_node));
						new->hdr.stmt_type		= STMT_UNINSTALL_NODE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
						}

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 54:
#line 623 "parser.y"
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

  case 55:
#line 653 "parser.y"
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

  case 56:
#line 681 "parser.y"
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

  case 57:
#line 709 "parser.y"
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

  case 58:
#line 737 "parser.y"
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

  case 59:
#line 765 "parser.y"
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

  case 60:
#line 791 "parser.y"
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

  case 61:
#line 827 "parser.y"
    {
						SlonikStmt_set_add_sequence *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_SET_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_STR( O_FQNAME, NULL ),
							STMT_OPTION_STR( O_COMMENT, NULL ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_set_add_sequence *)
								malloc(sizeof(SlonikStmt_set_add_sequence));
						memset(new, 0, sizeof(SlonikStmt_set_add_sequence));
						new->hdr.stmt_type		= STMT_SET_ADD_SEQUENCE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
							new->seq_id			= opt[2].ival;
							new->seq_fqname		= opt[3].str;
							new->seq_comment	= opt[4].str;
						}

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 62:
#line 859 "parser.y"
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

  case 63:
#line 889 "parser.y"
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

  case 64:
#line 915 "parser.y"
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

  case 65:
#line 941 "parser.y"
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

  case 66:
#line 967 "parser.y"
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

  case 67:
#line 995 "parser.y"
    { yyval.opt_list = NULL; }
    break;

  case 68:
#line 997 "parser.y"
    { yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 69:
#line 1001 "parser.y"
    { yyval.opt_list = yyvsp[0].opt_list; }
    break;

  case 70:
#line 1003 "parser.y"
    { yyvsp[-2].opt_list->next = yyvsp[0].opt_list; yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 71:
#line 1007 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 72:
#line 1012 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_BACKUP_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 73:
#line 1017 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_EVENT_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 74:
#line 1022 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SERVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 75:
#line 1027 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CLIENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 76:
#line 1032 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 77:
#line 1037 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_OLD_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 78:
#line 1042 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NEW_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 79:
#line 1047 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_RECEIVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 80:
#line 1052 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_PROVIDER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 81:
#line 1057 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNRETRY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 82:
#line 1062 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_COMMENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 83:
#line 1067 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNINFO;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 84:
#line 1072 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SET_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 85:
#line 1077 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NODE_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 86:
#line 1082 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FQNAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 87:
#line 1087 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_USE_KEY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 88:
#line 1092 "parser.y"
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

  case 89:
#line 1105 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FORWARD;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 90:
#line 1112 "parser.y"
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

  case 91:
#line 1126 "parser.y"
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

  case 92:
#line 1140 "parser.y"
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

  case 93:
#line 1152 "parser.y"
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

  case 100:
#line 1176 "parser.y"
    {
						yyval.ival = strtol(yytext, NULL, 10);
					}
    break;

  case 101:
#line 1182 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 102:
#line 1194 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 103:
#line 1206 "parser.y"
    { yyval.ival = yylineno; }
    break;


    }

/* Line 991 of yacc.c.  */
#line 2563 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1209 "parser.y"



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
		case O_BACKUP_NODE:		return "backup node";
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




