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
     K_ALL = 260,
     K_BACKUP = 261,
     K_CLIENT = 262,
     K_CLUSTER = 263,
     K_CLUSTERNAME = 264,
     K_COMMENT = 265,
     K_CONFIRMED = 266,
     K_CONNINFO = 267,
     K_CONNRETRY = 268,
     K_CREATE = 269,
     K_DROP = 270,
     K_ECHO = 271,
     K_ERROR = 272,
     K_EVENT = 273,
     K_EXECUTE = 274,
     K_EXIT = 275,
     K_FAILOVER = 276,
     K_FALSE = 277,
     K_FILENAME = 278,
     K_FOR = 279,
     K_FORWARD = 280,
     K_FULL = 281,
     K_FUNCTIONS = 282,
     K_ID = 283,
     K_INIT = 284,
     K_KEY = 285,
     K_LISTEN = 286,
     K_LOCK = 287,
     K_MERGE = 288,
     K_MOVE = 289,
     K_NAME = 290,
     K_NEW = 291,
     K_NO = 292,
     K_NODE = 293,
     K_OFF = 294,
     K_OLD = 295,
     K_ON = 296,
     K_ORIGIN = 297,
     K_PATH = 298,
     K_PROVIDER = 299,
     K_QUALIFIED = 300,
     K_RECEIVER = 301,
     K_RESTART = 302,
     K_SCRIPT = 303,
     K_SEQUENCE = 304,
     K_SERIAL = 305,
     K_SERVER = 306,
     K_SET = 307,
     K_STORE = 308,
     K_SUBSCRIBE = 309,
     K_SUCCESS = 310,
     K_TABLE = 311,
     K_TIMEOUT = 312,
     K_TRIGGER = 313,
     K_TRUE = 314,
     K_TRY = 315,
     K_UNINSTALL = 316,
     K_UNLOCK = 317,
     K_UNSUBSCRIBE = 318,
     K_UPDATE = 319,
     K_YES = 320,
     K_WAIT = 321,
     T_IDENT = 322,
     T_LITERAL = 323,
     T_NUMBER = 324
   };
#endif
#define K_ADD 258
#define K_ADMIN 259
#define K_ALL 260
#define K_BACKUP 261
#define K_CLIENT 262
#define K_CLUSTER 263
#define K_CLUSTERNAME 264
#define K_COMMENT 265
#define K_CONFIRMED 266
#define K_CONNINFO 267
#define K_CONNRETRY 268
#define K_CREATE 269
#define K_DROP 270
#define K_ECHO 271
#define K_ERROR 272
#define K_EVENT 273
#define K_EXECUTE 274
#define K_EXIT 275
#define K_FAILOVER 276
#define K_FALSE 277
#define K_FILENAME 278
#define K_FOR 279
#define K_FORWARD 280
#define K_FULL 281
#define K_FUNCTIONS 282
#define K_ID 283
#define K_INIT 284
#define K_KEY 285
#define K_LISTEN 286
#define K_LOCK 287
#define K_MERGE 288
#define K_MOVE 289
#define K_NAME 290
#define K_NEW 291
#define K_NO 292
#define K_NODE 293
#define K_OFF 294
#define K_OLD 295
#define K_ON 296
#define K_ORIGIN 297
#define K_PATH 298
#define K_PROVIDER 299
#define K_QUALIFIED 300
#define K_RECEIVER 301
#define K_RESTART 302
#define K_SCRIPT 303
#define K_SEQUENCE 304
#define K_SERIAL 305
#define K_SERVER 306
#define K_SET 307
#define K_STORE 308
#define K_SUBSCRIBE 309
#define K_SUCCESS 310
#define K_TABLE 311
#define K_TIMEOUT 312
#define K_TRIGGER 313
#define K_TRUE 314
#define K_TRY 315
#define K_UNINSTALL 316
#define K_UNLOCK 317
#define K_UNSUBSCRIBE 318
#define K_UPDATE 319
#define K_YES 320
#define K_WAIT 321
#define T_IDENT 322
#define T_LITERAL 323
#define T_NUMBER 324




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
 *	$Id: parser.c,v 1.16.2.3 2004-10-08 16:30:12 wieck Exp $
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "libpq-fe.h"
#include "slonik.h"


/*
 * Common option types
 */
typedef enum {
	O_ADD_ID,
	O_BACKUP_NODE,
	O_CLIENT,
	O_COMMENT,
	O_CONNINFO,
	O_CONNRETRY,
	O_EVENT_NODE,
	O_FILENAME,
	O_FORWARD,
	O_FQNAME,
	O_ID,
	O_NEW_ORIGIN,
	O_NEW_SET,
	O_NODE_ID,
	O_OLD_ORIGIN,
	O_ORIGIN,
	O_PROVIDER,
	O_RECEIVER,
	O_SERVER,
	O_SER_KEY,
	O_SET_ID,
	O_TAB_ID,
	O_TIMEOUT,
	O_TRIG_NAME,
	O_USE_KEY,
	O_WAIT_CONFIRMED,
	O_WAIT_ON,

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
#line 102 "parser.y"
typedef union YYSTYPE {
	int32			ival;
	char		   *str;
	option_list	   *opt_list;
	SlonikAdmInfo  *adm_info;
	SlonikStmt	   *statement;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 319 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 331 "y.tab.c"

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
#define YYLAST   306

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  78
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  58
/* YYNRULES -- Number of rules. */
#define YYNRULES  134
/* YYNRULES -- Number of states. */
#define YYNSTATES  290

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   324

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      75,    76,     2,     2,    77,    74,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    71,
       2,    70,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    72,     2,    73,     2,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69
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
     130,   132,   134,   136,   138,   140,   142,   144,   146,   148,
     150,   152,   154,   157,   162,   167,   173,   175,   178,   181,
     186,   191,   196,   200,   205,   210,   215,   220,   225,   230,
     235,   240,   246,   252,   258,   264,   270,   276,   282,   287,
     292,   297,   302,   307,   312,   317,   322,   327,   333,   335,
     340,   342,   346,   350,   355,   360,   364,   368,   372,   377,
     382,   387,   391,   395,   399,   403,   407,   412,   417,   422,
     427,   432,   438,   442,   446,   450,   454,   458,   462,   466,
     471,   475,   477,   479,   481,   483,   485,   487,   489,   491,
     493,   495,   497,   499,   501
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      79,     0,    -1,    80,    81,    83,    -1,   135,     8,    35,
      70,   133,    71,    -1,    82,    -1,    82,    81,    -1,   135,
      38,   132,     4,    12,    70,   134,    71,    -1,    84,    -1,
      84,    83,    -1,    85,    -1,    89,    -1,   135,    60,    72,
      88,    73,    86,    -1,   135,    60,    72,    88,    73,    87,
      86,    -1,   135,    60,    72,    88,    73,    86,    87,    -1,
     135,    60,    72,    88,    73,    87,    -1,   135,    60,    72,
      88,    73,    -1,    41,    17,    72,    83,    73,    -1,    41,
      55,    72,    83,    73,    -1,    89,    -1,    89,    88,    -1,
      90,    -1,    91,    -1,    92,    -1,    95,    -1,    96,    -1,
      97,    -1,    98,    -1,    99,    -1,   100,    -1,   101,    -1,
     102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,    -1,
     107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,    -1,
     112,    -1,   113,    -1,   114,    -1,   115,    -1,   116,    -1,
     117,    -1,   118,    -1,   119,    -1,   120,    -1,   121,    -1,
     122,    -1,   123,    -1,    94,    71,    -1,   135,    16,   134,
      71,    -1,   135,    20,    93,    71,    -1,   135,    47,    38,
     132,    71,    -1,    69,    -1,    74,    93,    -1,   135,     1,
      -1,   135,    29,     8,   124,    -1,   135,    53,    38,   124,
      -1,   135,    15,    38,   124,    -1,   135,    21,   124,    -1,
     135,    61,    38,   124,    -1,   135,    53,    43,   124,    -1,
     135,    15,    43,   124,    -1,   135,    53,    31,   124,    -1,
     135,    15,    31,   124,    -1,   135,    14,    52,   124,    -1,
     135,    15,    52,   124,    -1,   135,    33,    52,   124,    -1,
     135,    56,     3,    30,   124,    -1,   135,    52,     3,    56,
     124,    -1,   135,    52,     3,    49,   124,    -1,   135,    52,
      15,    56,   124,    -1,   135,    52,    15,    49,   124,    -1,
     135,    52,    34,    56,   124,    -1,   135,    52,    34,    49,
     124,    -1,   135,    53,    58,   124,    -1,   135,    15,    58,
     124,    -1,   135,    54,    52,   124,    -1,   135,    63,    52,
     124,    -1,   135,    32,    52,   124,    -1,   135,    62,    52,
     124,    -1,   135,    34,    52,   124,    -1,   135,    19,    48,
     124,    -1,   135,    64,    27,   124,    -1,   135,    66,    24,
      18,   124,    -1,    71,    -1,    75,   125,    76,    71,    -1,
     126,    -1,   126,    77,   125,    -1,    28,    70,   127,    -1,
       6,    38,    70,   127,    -1,    18,    38,    70,   127,    -1,
      51,    70,   127,    -1,     7,    70,   127,    -1,    42,    70,
     127,    -1,    40,    42,    70,   127,    -1,    36,    42,    70,
     127,    -1,    36,    52,    70,   127,    -1,    46,    70,   127,
      -1,    44,    70,   127,    -1,    13,    70,   127,    -1,    10,
      70,   128,    -1,    12,    70,   128,    -1,    52,    28,    70,
     127,    -1,     3,    28,    70,   127,    -1,    38,    28,    70,
     127,    -1,    56,    28,    70,   127,    -1,    58,    35,    70,
     128,    -1,    26,    45,    35,    70,   128,    -1,    30,    70,
     128,    -1,    30,    70,    50,    -1,    25,    70,   129,    -1,
      23,    70,   128,    -1,    42,    70,     5,    -1,    11,    70,
     127,    -1,    11,    70,     5,    -1,    66,    41,    70,   127,
      -1,    57,    70,   127,    -1,   132,    -1,   134,    -1,   130,
      -1,   131,    -1,    65,    -1,    41,    -1,    59,    -1,    37,
      -1,    39,    -1,    22,    -1,    69,    -1,    67,    -1,    68,
      -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   247,   247,   262,   268,   270,   274,   292,   294,   298,
     300,   304,   321,   339,   357,   374,   391,   394,   398,   400,
     404,   406,   408,   410,   412,   414,   416,   418,   420,   422,
     424,   426,   428,   430,   432,   434,   436,   438,   440,   442,
     444,   446,   448,   450,   452,   454,   456,   458,   460,   462,
     464,   466,   468,   473,   490,   507,   524,   526,   530,   545,
     573,   603,   631,   659,   685,   717,   747,   777,   807,   837,
     865,   895,   923,   961,   995,  1021,  1049,  1077,  1107,  1137,
    1167,  1199,  1227,  1255,  1283,  1313,  1344,  1370,  1402,  1404,
    1408,  1410,  1414,  1419,  1424,  1429,  1434,  1439,  1444,  1449,
    1454,  1459,  1464,  1469,  1474,  1479,  1484,  1489,  1494,  1499,
    1504,  1509,  1514,  1519,  1532,  1537,  1542,  1555,  1560,  1573,
    1578,  1585,  1599,  1613,  1625,  1639,  1640,  1641,  1644,  1645,
    1646,  1649,  1655,  1667,  1680
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "K_ADD", "K_ADMIN", "K_ALL", "K_BACKUP", 
  "K_CLIENT", "K_CLUSTER", "K_CLUSTERNAME", "K_COMMENT", "K_CONFIRMED", 
  "K_CONNINFO", "K_CONNRETRY", "K_CREATE", "K_DROP", "K_ECHO", "K_ERROR", 
  "K_EVENT", "K_EXECUTE", "K_EXIT", "K_FAILOVER", "K_FALSE", "K_FILENAME", 
  "K_FOR", "K_FORWARD", "K_FULL", "K_FUNCTIONS", "K_ID", "K_INIT", 
  "K_KEY", "K_LISTEN", "K_LOCK", "K_MERGE", "K_MOVE", "K_NAME", "K_NEW", 
  "K_NO", "K_NODE", "K_OFF", "K_OLD", "K_ON", "K_ORIGIN", "K_PATH", 
  "K_PROVIDER", "K_QUALIFIED", "K_RECEIVER", "K_RESTART", "K_SCRIPT", 
  "K_SEQUENCE", "K_SERIAL", "K_SERVER", "K_SET", "K_STORE", "K_SUBSCRIBE", 
  "K_SUCCESS", "K_TABLE", "K_TIMEOUT", "K_TRIGGER", "K_TRUE", "K_TRY", 
  "K_UNINSTALL", "K_UNLOCK", "K_UNSUBSCRIBE", "K_UPDATE", "K_YES", 
  "K_WAIT", "T_IDENT", "T_LITERAL", "T_NUMBER", "'='", "';'", "'{'", 
  "'}'", "'-'", "'('", "')'", "','", "$accept", "script", 
  "hdr_clustername", "hdr_admconninfos", "hdr_admconninfo", "stmts", 
  "stmt", "stmt_try", "try_on_error", "try_on_success", "try_stmts", 
  "try_stmt", "stmt_echo", "stmt_exit", "stmt_restart_node", "exit_code", 
  "stmt_error", "stmt_init_cluster", "stmt_store_node", "stmt_drop_node", 
  "stmt_failed_node", "stmt_uninstall_node", "stmt_store_path", 
  "stmt_drop_path", "stmt_store_listen", "stmt_drop_listen", 
  "stmt_create_set", "stmt_drop_set", "stmt_merge_set", 
  "stmt_table_add_key", "stmt_set_add_table", "stmt_set_add_sequence", 
  "stmt_set_drop_table", "stmt_set_drop_sequence", "stmt_set_move_table", 
  "stmt_set_move_sequence", "stmt_store_trigger", "stmt_drop_trigger", 
  "stmt_subscribe_set", "stmt_unsubscribe_set", "stmt_lock_set", 
  "stmt_unlock_set", "stmt_move_set", "stmt_ddl_script", 
  "stmt_update_functions", "stmt_wait_event", "option_list", 
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
      61,    59,   123,   125,    45,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    78,    79,    80,    81,    81,    82,    83,    83,    84,
      84,    85,    85,    85,    85,    85,    86,    87,    88,    88,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    90,    91,    92,    93,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   124,
     125,   125,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   127,   128,   129,   129,   130,   130,   130,   131,   131,
     131,   132,   133,   134,   135
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     6,     1,     2,     8,     1,     2,     1,
       1,     6,     7,     7,     6,     5,     5,     5,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     4,     4,     5,     1,     2,     2,     4,
       4,     4,     3,     4,     4,     4,     4,     4,     4,     4,
       4,     5,     5,     5,     5,     5,     5,     5,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     5,     1,     4,
       1,     3,     3,     4,     4,     3,     3,     3,     4,     4,
       4,     3,     3,     3,     3,     3,     4,     4,     4,     4,
       4,     5,     3,     3,     3,     3,     3,     3,     3,     4,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     134,     0,   134,     0,     1,   134,     4,     0,     0,     2,
     134,     9,    10,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,     0,     5,     0,     0,
       8,    52,    58,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   131,     0,     0,     0,     0,     0,
       0,     0,     0,   133,     0,     0,    56,     0,     0,    88,
       0,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,     0,     0,     0,
       0,     0,     0,   132,     0,    68,    67,    61,    65,    69,
      79,    53,    85,    57,    54,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    90,    59,    82,    70,    84,     0,     0,     0,     0,
       0,     0,     0,    66,    60,    64,    78,    80,     0,     0,
     134,     0,    63,    83,    81,    86,     0,     0,     3,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    55,    73,    72,
      75,    74,    77,    76,    71,    15,    19,    87,     0,     0,
       0,    96,   121,   104,   122,   118,   117,   105,   103,     0,
     115,   130,   128,   129,   126,   127,   125,   114,   123,   124,
       0,    92,   113,   112,     0,     0,     0,     0,   116,    97,
     102,   101,    95,     0,     0,   120,     0,     0,    89,    91,
       0,    11,    14,     0,   107,    93,    94,     0,    99,   100,
     108,    98,   106,   109,   110,   119,     0,     0,     0,    13,
       0,    12,     6,   111,   134,   134,     0,     0,    16,    17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     5,     6,     9,    10,    11,   261,   262,
     169,    12,    13,    14,    15,    88,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    91,   150,   151,   221,
     223,   237,   238,   239,   222,   114,   224,    46
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -170
static const short yypact[] =
{
    -170,    20,  -170,    21,  -170,  -170,     7,    22,    23,  -170,
       4,  -170,  -170,  -170,  -170,  -170,     2,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,   114,  -170,     5,     9,
    -170,  -170,  -170,    19,    45,    12,    34,    -8,   -23,    76,
      47,    49,    50,    58,    44,    78,    52,   102,    42,    68,
      65,    66,    95,   100,  -170,   121,    59,   -23,   -23,   -23,
     -23,   -23,   -23,  -170,    56,   -23,  -170,    -8,    60,  -170,
     240,  -170,   -23,   -23,   -23,   -23,     5,   -26,   -12,    -6,
     -23,   -23,   -23,   -23,   -23,   107,  -170,   -23,   -23,   -23,
     -23,   120,   127,  -170,    61,  -170,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,   112,   103,    72,    74,    75,
      79,    80,   113,    82,    83,   109,    85,    86,   -21,   129,
     116,    89,    90,    92,    93,   136,   144,   111,   138,   145,
     115,   108,  -170,  -170,  -170,  -170,   119,   -23,   -23,   -23,
     -23,   -23,   -23,  -170,  -170,  -170,  -170,  -170,   -23,   106,
     122,   168,  -170,  -170,  -170,  -170,   -23,   123,  -170,   124,
     126,     5,    12,     0,    12,     5,   128,    12,    16,   157,
       5,   -22,   133,   134,   135,   137,     3,     5,     5,     5,
     139,   140,     5,   141,   142,   143,   240,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,   158,  -170,  -170,    12,     5,
       5,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,     5,
    -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,
     146,  -170,  -170,  -170,     5,     5,     5,     5,  -170,  -170,
    -170,  -170,  -170,     5,     5,  -170,    12,     5,  -170,  -170,
      -1,   165,   167,   147,  -170,  -170,  -170,    12,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,   151,   153,   162,  -170,
     196,  -170,  -170,  -170,  -170,  -170,   154,   155,  -170,  -170
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -170,  -170,  -170,   213,  -170,   -10,  -170,  -170,   -36,   -28,
      67,  -100,  -170,  -170,  -170,   148,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,  -170,
    -170,  -170,  -170,  -170,  -170,  -170,   -68,    30,  -170,  -134,
    -169,  -170,  -170,  -170,   -29,  -170,   -53,     1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -135
static const short yytable[] =
{
      50,     3,    84,     7,    -7,   225,   170,     7,   248,   115,
     116,   117,   118,   119,   120,   227,   276,   122,   230,    75,
       4,   192,   243,   157,   152,   153,   154,   155,   242,     8,
     158,   193,   163,   164,   165,   166,   167,   159,   231,   172,
     173,   174,   175,   161,   160,  -134,    83,    97,    89,   226,
     162,   228,    90,   232,   277,   233,   241,   234,    49,    98,
      48,    86,   249,   250,   251,   252,    87,   156,   255,    74,
     170,    77,    74,    51,    74,   235,    78,    -7,    99,    76,
      83,   236,    85,    79,    92,   264,   265,   274,    80,   208,
     209,   210,   211,   212,   213,   266,    96,    81,   283,    93,
     214,    94,    95,    82,   104,   105,   107,   171,   217,   100,
     268,   269,   270,   271,   106,    52,   101,   108,   109,   272,
     273,   102,   110,   275,   111,   112,   113,   121,    53,    54,
      55,   124,   178,    56,    57,    58,   103,   168,   176,   177,
     179,   180,   181,    59,   182,   183,    60,    61,    62,   184,
     185,   186,   187,   188,   189,   190,   191,   194,   195,   196,
     197,    63,   198,   199,   200,   263,    64,    65,    66,    52,
      67,   171,   201,   203,    68,    69,    70,    71,    72,   215,
      73,   202,    53,    54,    55,   206,   204,    56,    57,    58,
     207,   205,   240,   218,   219,   -18,   220,    59,   229,   260,
      60,    61,    62,   244,   245,   246,   278,   247,   280,   253,
     254,   256,   257,   276,   258,    63,   267,   277,   282,    47,
      64,    65,    66,   284,    67,   285,   281,   288,   289,    69,
      70,    71,    72,   279,    73,   123,   259,   216,     0,     0,
       0,     0,     0,   125,     0,     0,   126,   127,     0,     0,
     128,   129,   130,   131,     0,     0,     0,     0,   132,     0,
       0,     0,     0,   133,     0,   134,   135,     0,   136,     0,
     137,     0,     0,     0,   286,   287,   138,     0,   139,     0,
     140,     0,   141,     0,   142,     0,   143,     0,     0,     0,
       0,   144,   145,     0,     0,     0,   146,   147,   148,     0,
       0,     0,     0,     0,     0,     0,   149
};

static const short yycheck[] =
{
      10,     0,    55,     2,     0,     5,   106,     6,     5,    77,
      78,    79,    80,    81,    82,   184,    17,    85,   187,    48,
       0,    42,   191,    49,    92,    93,    94,    95,    50,     8,
      56,    52,   100,   101,   102,   103,   104,    49,    22,   107,
     108,   109,   110,    49,    56,    38,    68,     3,    71,   183,
      56,   185,    75,    37,    55,    39,   190,    41,    35,    15,
      38,    69,   196,   197,   198,   199,    74,    96,   202,    69,
     170,    52,    69,    71,    69,    59,    31,    73,    34,    70,
      68,    65,    48,    38,     8,   219,   220,   256,    43,   157,
     158,   159,   160,   161,   162,   229,    38,    52,   267,    52,
     168,    52,    52,    58,    52,     3,    38,   106,   176,    31,
     244,   245,   246,   247,    72,     1,    38,    52,    52,   253,
     254,    43,    27,   257,    24,     4,    67,    71,    14,    15,
      16,    71,    71,    19,    20,    21,    58,    30,    18,    12,
      28,    38,    70,    29,    70,    70,    32,    33,    34,    70,
      70,    38,    70,    70,    45,    70,    70,    28,    42,    70,
      70,    47,    70,    70,    28,   218,    52,    53,    54,     1,
      56,   170,    28,    35,    60,    61,    62,    63,    64,    73,
      66,    70,    14,    15,    16,    77,    41,    19,    20,    21,
      71,    76,    35,    70,    70,    73,    70,    29,    70,    41,
      32,    33,    34,    70,    70,    70,    41,    70,    41,    70,
      70,    70,    70,    17,    71,    47,    70,    55,    71,     6,
      52,    53,    54,    72,    56,    72,   262,    73,    73,    61,
      62,    63,    64,   261,    66,    87,   206,   170,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,     6,     7,    -1,    -1,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    18,    -1,
      -1,    -1,    -1,    23,    -1,    25,    26,    -1,    28,    -1,
      30,    -1,    -1,    -1,   284,   285,    36,    -1,    38,    -1,
      40,    -1,    42,    -1,    44,    -1,    46,    -1,    -1,    -1,
      -1,    51,    52,    -1,    -1,    -1,    56,    57,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    66
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    79,    80,   135,     0,    81,    82,   135,     8,    83,
      84,    85,    89,    90,    91,    92,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   135,    81,    38,    35,
      83,    71,     1,    14,    15,    16,    19,    20,    21,    29,
      32,    33,    34,    47,    52,    53,    54,    56,    60,    61,
      62,    63,    64,    66,    69,   132,    70,    52,    31,    38,
      43,    52,    58,    68,   134,    48,    69,    74,    93,    71,
      75,   124,     8,    52,    52,    52,    38,     3,    15,    34,
      31,    38,    43,    58,    52,     3,    72,    38,    52,    52,
      27,    24,     4,    67,   133,   124,   124,   124,   124,   124,
     124,    71,   124,    93,    71,     3,     6,     7,    10,    11,
      12,    13,    18,    23,    25,    26,    28,    30,    36,    38,
      40,    42,    44,    46,    51,    52,    56,    57,    58,    66,
     125,   126,   124,   124,   124,   124,   132,    49,    56,    49,
      56,    49,    56,   124,   124,   124,   124,   124,    30,    88,
      89,   135,   124,   124,   124,   124,    18,    12,    71,    28,
      38,    70,    70,    70,    70,    70,    38,    70,    70,    45,
      70,    70,    42,    52,    28,    42,    70,    70,    70,    70,
      28,    28,    70,    35,    41,    76,    77,    71,   124,   124,
     124,   124,   124,   124,   124,    73,    88,   124,    70,    70,
      70,   127,   132,   128,   134,     5,   127,   128,   127,    70,
     128,    22,    37,    39,    41,    59,    65,   129,   130,   131,
      35,   127,    50,   128,    70,    70,    70,    70,     5,   127,
     127,   127,   127,    70,    70,   127,    70,    70,    71,   125,
      41,    86,    87,   134,   127,   127,   127,    70,   127,   127,
     127,   127,   127,   127,   128,   127,    17,    55,    41,    87,
      41,    86,    71,   128,    72,    72,    83,    83,    73,    73
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
#line 250 "parser.y"
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
#line 263 "parser.y"
    {
						yyval.str = yyvsp[-1].str;
					}
    break;

  case 4:
#line 269 "parser.y"
    { yyval.adm_info = yyvsp[0].adm_info; }
    break;

  case 5:
#line 271 "parser.y"
    { yyvsp[-1].adm_info->next = yyvsp[0].adm_info; yyval.adm_info = yyvsp[-1].adm_info; }
    break;

  case 6:
#line 275 "parser.y"
    {
						SlonikAdmInfo	   *new;

						new = (SlonikAdmInfo *)
								malloc(sizeof(SlonikAdmInfo));
						memset(new, 0, sizeof(SlonikAdmInfo));

						new->no_id			= yyvsp[-5].ival;
						new->stmt_filename	= current_file;
						new->stmt_lno		= yyvsp[-7].ival;
						new->conninfo		= yyvsp[-1].str;
						new->last_event		= -1;

						yyval.adm_info = new;
					}
    break;

  case 7:
#line 293 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 8:
#line 295 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 9:
#line 299 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 10:
#line 301 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 11:
#line 306 "parser.y"
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
#line 323 "parser.y"
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
#line 341 "parser.y"
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
#line 359 "parser.y"
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
#line 375 "parser.y"
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
#line 392 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 17:
#line 395 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 18:
#line 399 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 19:
#line 401 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 20:
#line 405 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 21:
#line 407 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 22:
#line 409 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 23:
#line 411 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 24:
#line 413 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 25:
#line 415 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 26:
#line 417 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 27:
#line 419 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 28:
#line 421 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 29:
#line 423 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 30:
#line 425 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 31:
#line 427 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 32:
#line 429 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 33:
#line 431 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 34:
#line 433 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 35:
#line 435 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 36:
#line 437 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 37:
#line 439 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 38:
#line 441 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 39:
#line 443 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 40:
#line 445 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 41:
#line 447 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 42:
#line 449 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 43:
#line 451 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 44:
#line 453 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 45:
#line 455 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 46:
#line 457 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 47:
#line 459 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 48:
#line 461 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 49:
#line 463 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 50:
#line 465 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 51:
#line 467 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 52:
#line 469 "parser.y"
    { yyerrok;
						  yyval.statement = yyvsp[-1].statement; }
    break;

  case 53:
#line 474 "parser.y"
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

  case 54:
#line 491 "parser.y"
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

  case 55:
#line 508 "parser.y"
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

  case 56:
#line 525 "parser.y"
    { yyval.ival = strtol(yytext, NULL, 10); }
    break;

  case 57:
#line 527 "parser.y"
    { yyval.ival = -yyvsp[0].ival; }
    break;

  case 58:
#line 531 "parser.y"
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

  case 59:
#line 546 "parser.y"
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

  case 60:
#line 574 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 61:
#line 604 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 62:
#line 632 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 63:
#line 660 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 64:
#line 686 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 65:
#line 718 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 66:
#line 748 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 67:
#line 778 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 68:
#line 808 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 69:
#line 838 "parser.y"
    {
						SlonikStmt_drop_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_drop_set *)
								malloc(sizeof(SlonikStmt_drop_set));
						memset(new, 0, sizeof(SlonikStmt_drop_set));
						new->hdr.stmt_type		= STMT_DROP_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->set_origin		= opt[1].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 70:
#line 866 "parser.y"
    {
						SlonikStmt_merge_set *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_ADD_ID, -1 ),
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_merge_set *)
								malloc(sizeof(SlonikStmt_merge_set));
						memset(new, 0, sizeof(SlonikStmt_merge_set));
						new->hdr.stmt_type		= STMT_MERGE_SET;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_id			= opt[0].ival;
							new->add_id			= opt[1].ival;
							new->set_origin		= opt[2].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 71:
#line 896 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 72:
#line 924 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 73:
#line 962 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 74:
#line 996 "parser.y"
    {
						SlonikStmt_set_drop_table *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_END
						};
						new = (SlonikStmt_set_drop_table *)
							malloc(sizeof(SlonikStmt_set_drop_table));
						memset(new, 0, sizeof(SlonikStmt_set_drop_table));
						new->hdr.stmt_type		= STMT_SET_DROP_TABLE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0) {
							new->set_origin		= opt[0].ival;
							new->tab_id			= opt[1].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 75:
#line 1022 "parser.y"
    {
						SlonikStmt_set_drop_sequence *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_set_drop_sequence *)
								malloc(sizeof(SlonikStmt_set_drop_sequence));
						memset(new, 0, sizeof(SlonikStmt_set_drop_sequence));
						new->hdr.stmt_type		= STMT_SET_DROP_SEQUENCE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_origin		= opt[0].ival;
							new->seq_id			= opt[1].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 76:
#line 1050 "parser.y"
    {
						SlonikStmt_set_move_table *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_NEW_SET, -1 ),
							STMT_OPTION_END
						};
						new = (SlonikStmt_set_move_table *)
							malloc(sizeof(SlonikStmt_set_move_table));
						memset(new, 0, sizeof(SlonikStmt_set_move_table));
						new->hdr.stmt_type		= STMT_SET_MOVE_TABLE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0) {
							new->set_origin		= opt[0].ival;
							new->tab_id			= opt[1].ival;
							new->new_set_id		= opt[2].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 77:
#line 1078 "parser.y"
    {
						SlonikStmt_set_move_sequence *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_ID, -1 ),
							STMT_OPTION_INT( O_NEW_SET, -1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_set_move_sequence *)
								malloc(sizeof(SlonikStmt_set_move_sequence));
						memset(new, 0, sizeof(SlonikStmt_set_move_sequence));
						new->hdr.stmt_type		= STMT_SET_MOVE_SEQUENCE;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->set_origin		= opt[0].ival;
							new->seq_id			= opt[1].ival;
							new->new_set_id		= opt[2].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 78:
#line 1108 "parser.y"
    {
						SlonikStmt_store_trigger *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_TAB_ID, -1 ),
							STMT_OPTION_STR( O_TRIG_NAME, NULL ),
							STMT_OPTION_INT( O_EVENT_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_store_trigger *)
								malloc(sizeof(SlonikStmt_store_trigger));
						memset(new, 0, sizeof(SlonikStmt_store_trigger));
						new->hdr.stmt_type		= STMT_STORE_TRIGGER;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->trig_tabid		= opt[0].ival;
							new->trig_tgname	= opt[1].str;
							new->ev_origin		= opt[2].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 79:
#line 1138 "parser.y"
    {
						SlonikStmt_drop_trigger *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_TAB_ID, -1 ),
							STMT_OPTION_STR( O_TRIG_NAME, NULL ),
							STMT_OPTION_INT( O_EVENT_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_drop_trigger *)
								malloc(sizeof(SlonikStmt_drop_trigger));
						memset(new, 0, sizeof(SlonikStmt_drop_trigger));
						new->hdr.stmt_type		= STMT_DROP_TRIGGER;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->trig_tabid		= opt[0].ival;
							new->trig_tgname	= opt[1].str;
							new->ev_origin		= opt[2].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 80:
#line 1168 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 81:
#line 1200 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 82:
#line 1228 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 83:
#line 1256 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 84:
#line 1284 "parser.y"
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
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 85:
#line 1314 "parser.y"
    {
						SlonikStmt_ddl_script *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_SET_ID, -1 ),
							STMT_OPTION_STR( O_FILENAME, NULL ),
							STMT_OPTION_INT( O_EVENT_NODE, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_ddl_script *)
								malloc(sizeof(SlonikStmt_ddl_script));
						memset(new, 0, sizeof(SlonikStmt_ddl_script));
						new->hdr.stmt_type		= STMT_DDL_SCRIPT;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->ddl_setid		= opt[0].ival;
							new->ddl_fname		= opt[1].str;
							new->ev_origin		= opt[2].ival;
							new->ddl_fd			= -1;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 86:
#line 1345 "parser.y"
    {
						SlonikStmt_update_functions *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ID, 1 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_update_functions *)
								malloc(sizeof(SlonikStmt_update_functions));
						memset(new, 0, sizeof(SlonikStmt_update_functions));
						new->hdr.stmt_type		= STMT_UPDATE_FUNCTIONS;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-3].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->no_id			= opt[0].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 87:
#line 1371 "parser.y"
    {
						SlonikStmt_wait_event *new;
						statement_option opt[] = {
							STMT_OPTION_INT( O_ORIGIN, -1 ),
							STMT_OPTION_INT( O_WAIT_CONFIRMED, -1 ),
							STMT_OPTION_INT( O_WAIT_ON, 1 ),
							STMT_OPTION_INT( O_TIMEOUT, 600 ),
							STMT_OPTION_END
						};

						new = (SlonikStmt_wait_event *)
								malloc(sizeof(SlonikStmt_wait_event));
						memset(new, 0, sizeof(SlonikStmt_wait_event));
						new->hdr.stmt_type		= STMT_WAIT_EVENT;
						new->hdr.stmt_filename	= current_file;
						new->hdr.stmt_lno		= yyvsp[-4].ival;

						if (assign_options(opt, yyvsp[0].opt_list) == 0)
						{
							new->wait_origin	= opt[0].ival;
							new->wait_confirmed	= opt[1].ival;
							new->wait_on		= opt[2].ival;
							new->wait_timeout	= opt[3].ival;
						}
						else
							parser_errors++;

						yyval.statement = (SlonikStmt *)new;
					}
    break;

  case 88:
#line 1403 "parser.y"
    { yyval.opt_list = NULL; }
    break;

  case 89:
#line 1405 "parser.y"
    { yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 90:
#line 1409 "parser.y"
    { yyval.opt_list = yyvsp[0].opt_list; }
    break;

  case 91:
#line 1411 "parser.y"
    { yyvsp[-2].opt_list->next = yyvsp[0].opt_list; yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 92:
#line 1415 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 93:
#line 1420 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_BACKUP_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 94:
#line 1425 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_EVENT_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 95:
#line 1430 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SERVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 96:
#line 1435 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CLIENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 97:
#line 1440 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 98:
#line 1445 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_OLD_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 99:
#line 1450 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NEW_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 100:
#line 1455 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NEW_SET;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 101:
#line 1460 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_RECEIVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 102:
#line 1465 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_PROVIDER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 103:
#line 1470 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNRETRY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 104:
#line 1475 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_COMMENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 105:
#line 1480 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNINFO;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 106:
#line 1485 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SET_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 107:
#line 1490 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ADD_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 108:
#line 1495 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NODE_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 109:
#line 1500 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TAB_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 110:
#line 1505 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TRIG_NAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 111:
#line 1510 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FQNAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 112:
#line 1515 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_USE_KEY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 113:
#line 1520 "parser.y"
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

  case 114:
#line 1533 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FORWARD;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 115:
#line 1538 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FILENAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 116:
#line 1543 "parser.y"
    {
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->opt_code	= O_ORIGIN;
						new->ival	= -2;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
    break;

  case 117:
#line 1556 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_WAIT_CONFIRMED;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 118:
#line 1561 "parser.y"
    {
						option_list *new;
						new = (option_list *)malloc(sizeof(option_list));

						new->opt_code	= O_WAIT_CONFIRMED;
						new->ival	= -2;
						new->str	= NULL;
						new->lineno	= yylineno;
						new->next	= NULL;

						yyval.opt_list = new;
					}
    break;

  case 119:
#line 1574 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_WAIT_ON;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 120:
#line 1579 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TIMEOUT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 121:
#line 1586 "parser.y"
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

  case 122:
#line 1600 "parser.y"
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

  case 123:
#line 1614 "parser.y"
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

  case 124:
#line 1626 "parser.y"
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

  case 131:
#line 1650 "parser.y"
    {
						yyval.ival = strtol(yytext, NULL, 10);
					}
    break;

  case 132:
#line 1656 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 133:
#line 1668 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 134:
#line 1680 "parser.y"
    { yyval.ival = yylineno; }
    break;


    }

/* Line 991 of yacc.c.  */
#line 3180 "y.tab.c"

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


#line 1683 "parser.y"



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
		case O_ADD_ID:			return "add id";
		case O_BACKUP_NODE:		return "backup node";
		case O_CLIENT:			return "client";
		case O_COMMENT:			return "comment";
		case O_CONNINFO:		return "conninfo";
		case O_CONNRETRY:		return "connretry";
		case O_EVENT_NODE:		return "event node";
		case O_FILENAME:		return "filename";
		case O_FORWARD:			return "forward";
		case O_FQNAME:			return "full qualified name";
		case O_ID:				return "id";
		case O_NEW_ORIGIN:		return "new origin";
		case O_NEW_SET:			return "new set";
		case O_NODE_ID:			return "node id";
		case O_OLD_ORIGIN:		return "old origin";
		case O_ORIGIN:			return "origin";
		case O_PROVIDER:		return "provider";
		case O_RECEIVER:		return "receiver";
		case O_SERVER:			return "server";
		case O_SER_KEY:			return "key";
		case O_SET_ID:			return "set id";
		case O_TAB_ID:			return "table id";
		case O_TIMEOUT:			return "timeout";
		case O_TRIG_NAME:		return "trigger name";
		case O_USE_KEY:			return "key";
		case O_WAIT_CONFIRMED:	return "confirmed";
		case O_WAIT_ON:			return "wait on";
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



/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */

