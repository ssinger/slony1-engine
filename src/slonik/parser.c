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
 *	$Id: parser.c,v 1.14 2004-06-07 18:46:17 wieck Exp $
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
#line 101 "parser.y"
typedef union YYSTYPE {
	int32			ival;
	char		   *str;
	option_list	   *opt_list;
	SlonikAdmInfo  *adm_info;
	SlonikStmt	   *statement;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 318 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 330 "y.tab.c"

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
#define YYLAST   290

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  78
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  55
/* YYNRULES -- Number of rules. */
#define YYNRULES  126
/* YYNRULES -- Number of states. */
#define YYNSTATES  274

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
     130,   132,   134,   136,   138,   140,   142,   144,   146,   147,
     151,   156,   161,   167,   169,   172,   175,   180,   185,   190,
     194,   199,   204,   209,   214,   219,   224,   229,   234,   240,
     246,   252,   257,   262,   267,   272,   277,   282,   287,   292,
     297,   303,   305,   310,   312,   316,   320,   325,   330,   334,
     338,   342,   347,   352,   356,   360,   364,   368,   372,   377,
     382,   387,   392,   397,   403,   407,   411,   415,   419,   423,
     427,   431,   436,   440,   442,   444,   446,   448,   450,   452,
     454,   456,   458,   460,   462,   464,   466
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      79,     0,    -1,    80,    81,    83,    -1,   132,     8,    35,
      70,   130,    71,    -1,    82,    -1,    82,    81,    -1,   132,
      38,   129,     4,    12,    70,   131,    71,    -1,    84,    -1,
      84,    83,    -1,    85,    -1,    89,    -1,   132,    60,    72,
      88,    73,    86,    -1,   132,    60,    72,    88,    73,    87,
      86,    -1,   132,    60,    72,    88,    73,    86,    87,    -1,
     132,    60,    72,    88,    73,    87,    -1,   132,    60,    72,
      88,    73,    -1,    41,    17,    72,    83,    73,    -1,    41,
      55,    72,    83,    73,    -1,    89,    -1,    89,    88,    -1,
      91,    -1,    92,    -1,    93,    -1,    96,    -1,    97,    -1,
      98,    -1,    99,    -1,   100,    -1,   101,    -1,   102,    -1,
     103,    -1,   104,    -1,   105,    -1,   106,    -1,   107,    -1,
     108,    -1,   109,    -1,   110,    -1,   111,    -1,   112,    -1,
     113,    -1,   114,    -1,   115,    -1,   116,    -1,   117,    -1,
     118,    -1,   119,    -1,   120,    -1,    -1,    95,    71,    90,
      -1,   132,    16,   131,    71,    -1,   132,    20,    94,    71,
      -1,   132,    47,    38,   129,    71,    -1,    69,    -1,    74,
      94,    -1,   132,     1,    -1,   132,    29,     8,   121,    -1,
     132,    53,    38,   121,    -1,   132,    15,    38,   121,    -1,
     132,    21,   121,    -1,   132,    61,    38,   121,    -1,   132,
      53,    43,   121,    -1,   132,    15,    43,   121,    -1,   132,
      53,    31,   121,    -1,   132,    15,    31,   121,    -1,   132,
      14,    52,   121,    -1,   132,    15,    52,   121,    -1,   132,
      33,    52,   121,    -1,   132,    56,     3,    30,   121,    -1,
     132,    52,     3,    56,   121,    -1,   132,    52,     3,    49,
     121,    -1,   132,    53,    58,   121,    -1,   132,    15,    58,
     121,    -1,   132,    54,    52,   121,    -1,   132,    63,    52,
     121,    -1,   132,    32,    52,   121,    -1,   132,    62,    52,
     121,    -1,   132,    34,    52,   121,    -1,   132,    19,    48,
     121,    -1,   132,    64,    27,   121,    -1,   132,    66,    24,
      18,   121,    -1,    71,    -1,    75,   122,    76,    71,    -1,
     123,    -1,   123,    77,   122,    -1,    28,    70,   124,    -1,
       6,    38,    70,   124,    -1,    18,    38,    70,   124,    -1,
      51,    70,   124,    -1,     7,    70,   124,    -1,    42,    70,
     124,    -1,    40,    42,    70,   124,    -1,    36,    42,    70,
     124,    -1,    46,    70,   124,    -1,    44,    70,   124,    -1,
      13,    70,   124,    -1,    10,    70,   125,    -1,    12,    70,
     125,    -1,    52,    28,    70,   124,    -1,     3,    28,    70,
     124,    -1,    38,    28,    70,   124,    -1,    56,    28,    70,
     124,    -1,    58,    35,    70,   125,    -1,    26,    45,    35,
      70,   125,    -1,    30,    70,   125,    -1,    30,    70,    50,
      -1,    25,    70,   126,    -1,    23,    70,   125,    -1,    42,
      70,     5,    -1,    11,    70,   124,    -1,    11,    70,     5,
      -1,    66,    41,    70,   124,    -1,    57,    70,   124,    -1,
     129,    -1,   131,    -1,   127,    -1,   128,    -1,    65,    -1,
      41,    -1,    59,    -1,    37,    -1,    39,    -1,    22,    -1,
      69,    -1,    67,    -1,    68,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   242,   242,   257,   263,   265,   269,   287,   289,   293,
     295,   299,   316,   334,   352,   369,   386,   389,   393,   395,
     399,   401,   403,   405,   407,   409,   411,   413,   415,   417,
     419,   421,   423,   425,   427,   429,   431,   433,   435,   437,
     439,   441,   443,   445,   447,   449,   451,   453,   455,   455,
     459,   476,   493,   510,   512,   516,   531,   559,   589,   617,
     645,   671,   703,   733,   763,   793,   823,   851,   881,   909,
     947,   981,  1011,  1041,  1073,  1101,  1129,  1157,  1187,  1218,
    1244,  1276,  1278,  1282,  1284,  1288,  1293,  1298,  1303,  1308,
    1313,  1318,  1323,  1328,  1333,  1338,  1343,  1348,  1353,  1358,
    1363,  1368,  1373,  1378,  1383,  1388,  1401,  1406,  1411,  1424,
    1429,  1442,  1447,  1454,  1468,  1482,  1494,  1508,  1509,  1510,
    1513,  1514,  1515,  1518,  1524,  1536,  1549
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
  "try_stmt", "@1", "stmt_echo", "stmt_exit", "stmt_restart_node", 
  "exit_code", "stmt_error", "stmt_init_cluster", "stmt_store_node", 
  "stmt_drop_node", "stmt_failed_node", "stmt_uninstall_node", 
  "stmt_store_path", "stmt_drop_path", "stmt_store_listen", 
  "stmt_drop_listen", "stmt_create_set", "stmt_drop_set", 
  "stmt_merge_set", "stmt_table_add_key", "stmt_set_add_table", 
  "stmt_set_add_sequence", "stmt_store_trigger", "stmt_drop_trigger", 
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
      89,    89,    89,    89,    89,    89,    89,    89,    90,    89,
      91,    92,    93,    94,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   121,   122,   122,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   124,   125,   126,   126,   127,   127,   127,
     128,   128,   128,   129,   130,   131,   132
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     6,     1,     2,     8,     1,     2,     1,
       1,     6,     7,     7,     6,     5,     5,     5,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     3,
       4,     4,     5,     1,     2,     2,     4,     4,     4,     3,
       4,     4,     4,     4,     4,     4,     4,     4,     5,     5,
       5,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       5,     1,     4,     1,     3,     3,     4,     4,     3,     3,
       3,     4,     4,     3,     3,     3,     3,     3,     4,     4,
       4,     4,     4,     5,     3,     3,     3,     3,     3,     3,
       3,     4,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
     126,     0,   126,     0,     1,   126,     4,     0,     0,     2,
     126,     9,    10,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,     0,     5,     0,     0,     8,    48,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     123,     0,     0,    49,     0,     0,     0,     0,     0,     0,
     125,     0,     0,    53,     0,     0,    81,     0,    59,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   126,     0,     0,     0,     0,     0,     0,   124,     0,
      65,    64,    58,    62,    66,    72,    50,    78,    54,    51,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    83,    56,    75,    67,
      77,     0,     0,     0,    63,    57,    61,    71,    73,     0,
       0,   126,     0,    60,    76,    74,    79,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    52,    70,    69,
      68,    15,    19,    80,     0,     0,     0,    89,   113,    96,
     114,   110,   109,    97,    95,     0,   107,   122,   120,   121,
     118,   119,   117,   106,   115,   116,     0,    85,   105,   104,
       0,     0,     0,   108,    90,    94,    93,    88,     0,     0,
     112,     0,     0,    82,    84,     0,    11,    14,     0,    99,
      86,    87,     0,    92,   100,    91,    98,   101,   102,   111,
       0,     0,     0,    13,     0,    12,     6,   103,   126,   126,
       0,     0,    16,    17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     5,     6,     9,    10,    11,   246,   247,
     160,    12,    73,    13,    14,    15,    85,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    88,   145,   146,   207,   209,   223,   224,
     225,   208,   109,   210,    42
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -162
static const short yypact[] =
{
    -162,    22,  -162,    26,  -162,  -162,    -3,     3,     8,  -162,
       4,  -162,  -162,  -162,  -162,  -162,   -21,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
    -162,  -162,   105,  -162,   -18,   -11,  -162,  -162,  -162,    13,
      18,     5,    27,    -6,   -27,    66,    32,    33,    37,    40,
      89,    52,    42,    96,    28,    65,    55,    57,    84,    88,
    -162,   109,    47,  -162,   -27,   -27,   -27,   -27,   -27,   -27,
    -162,    44,   -27,  -162,    -6,    45,  -162,   224,  -162,   -27,
     -27,   -27,   -27,   -18,   -29,   -27,   -27,   -27,   -27,   -27,
      87,  -162,   -27,   -27,   -27,   -27,   100,   110,  -162,    56,
    -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
      95,    90,    59,    60,    61,    62,    63,    97,    70,    71,
      91,    72,    73,   102,   117,   104,    77,    78,    79,    80,
     123,   125,    86,   119,   122,    94,    99,  -162,  -162,  -162,
    -162,    93,   -27,   -27,  -162,  -162,  -162,  -162,  -162,   -27,
     108,   111,   159,  -162,  -162,  -162,  -162,   -27,   107,  -162,
     112,   113,   -18,     5,     0,     5,   -18,   115,     5,    23,
     137,   -18,   -22,   116,   120,   124,    10,   -18,   -18,   -18,
     126,   127,   -18,   128,   129,   118,   224,  -162,  -162,  -162,
    -162,   146,  -162,  -162,     5,   -18,   -18,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,   -18,  -162,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,  -162,   130,  -162,  -162,  -162,
     -18,   -18,   -18,  -162,  -162,  -162,  -162,  -162,   -18,   -18,
    -162,     5,   -18,  -162,  -162,     2,   154,   160,   131,  -162,
    -162,  -162,     5,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
     132,   133,   148,  -162,   190,  -162,  -162,  -162,  -162,  -162,
     135,   136,  -162,  -162
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -162,  -162,  -162,   204,  -162,   -10,  -162,  -162,   -33,   -30,
      58,   -95,  -162,  -162,  -162,  -162,   134,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
    -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,  -162,
    -162,  -162,  -162,   -66,    21,  -162,  -134,  -161,  -162,  -162,
    -162,   -26,  -162,   -49,     1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -127
static const short yytable[] =
{
      46,     3,    81,     7,    -7,   211,   161,     7,   110,   111,
     112,   113,   114,   115,   213,   233,   117,   216,    71,   260,
     152,   229,     4,   147,   148,   149,   150,   153,   228,   154,
     155,   156,   157,   158,     8,  -126,   163,   164,   165,   166,
     212,    44,   214,    45,    86,   217,    80,   227,    87,    75,
      47,    70,   234,   235,   236,   237,    76,   261,   240,    72,
     218,    77,   219,    83,   220,    74,   161,   151,    84,    70,
      78,   249,   250,    80,    89,    82,    79,    -7,    93,    70,
     258,   251,   221,    95,    90,    91,   198,   199,   222,    92,
      96,   267,    94,   200,    99,    97,   253,   254,   255,   100,
     101,   203,   162,   102,   256,   257,    48,   103,   259,   104,
      98,   105,   106,   107,   108,   116,   119,   159,   167,    49,
      50,    51,   168,   170,    52,    53,    54,   169,   171,   172,
     173,   174,   175,   176,    55,   177,   180,    56,    57,    58,
     178,   179,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,    59,   191,   193,   248,   192,    60,    61,    62,
      48,    63,   162,   194,   197,    64,    65,    66,    67,    68,
     195,    69,   226,    49,    50,    51,   196,   204,    52,    53,
      54,   201,   205,   206,   -18,   215,   230,   245,    55,   243,
     231,    56,    57,    58,   232,   262,   238,   239,   241,   242,
     252,   264,   266,   261,   268,   269,    59,   260,   272,   273,
      43,    60,    61,    62,   265,    63,   263,   244,   118,   202,
      65,    66,    67,    68,     0,    69,     0,   120,     0,     0,
     121,   122,     0,     0,   123,   124,   125,   126,     0,     0,
       0,     0,   127,     0,     0,     0,     0,   128,     0,   129,
     130,     0,   131,     0,   132,     0,     0,     0,   270,   271,
     133,     0,   134,     0,   135,     0,   136,     0,   137,     0,
     138,     0,     0,     0,     0,   139,   140,     0,     0,     0,
     141,   142,   143,     0,     0,     0,     0,     0,     0,     0,
     144
};

static const short yycheck[] =
{
      10,     0,    51,     2,     0,     5,   101,     6,    74,    75,
      76,    77,    78,    79,   175,     5,    82,   178,    44,    17,
      49,   182,     0,    89,    90,    91,    92,    56,    50,    95,
      96,    97,    98,    99,     8,    38,   102,   103,   104,   105,
     174,    38,   176,    35,    71,    22,    68,   181,    75,    31,
      71,    69,   186,   187,   188,   189,    38,    55,   192,    70,
      37,    43,    39,    69,    41,    52,   161,    93,    74,    69,
      52,   205,   206,    68,     8,    48,    58,    73,    38,    69,
     241,   215,    59,    31,    52,    52,   152,   153,    65,    52,
      38,   252,     3,   159,    52,    43,   230,   231,   232,     3,
      72,   167,   101,    38,   238,   239,     1,    52,   242,    52,
      58,    27,    24,     4,    67,    71,    71,    30,    18,    14,
      15,    16,    12,    28,    19,    20,    21,    71,    38,    70,
      70,    70,    70,    70,    29,    38,    45,    32,    33,    34,
      70,    70,    70,    70,    42,    28,    42,    70,    70,    70,
      70,    28,    47,    28,    35,   204,    70,    52,    53,    54,
       1,    56,   161,    41,    71,    60,    61,    62,    63,    64,
      76,    66,    35,    14,    15,    16,    77,    70,    19,    20,
      21,    73,    70,    70,    73,    70,    70,    41,    29,    71,
      70,    32,    33,    34,    70,    41,    70,    70,    70,    70,
      70,    41,    71,    55,    72,    72,    47,    17,    73,    73,
       6,    52,    53,    54,   247,    56,   246,   196,    84,   161,
      61,    62,    63,    64,    -1,    66,    -1,     3,    -1,    -1,
       6,     7,    -1,    -1,    10,    11,    12,    13,    -1,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    23,    -1,    25,
      26,    -1,    28,    -1,    30,    -1,    -1,    -1,   268,   269,
      36,    -1,    38,    -1,    40,    -1,    42,    -1,    44,    -1,
      46,    -1,    -1,    -1,    -1,    51,    52,    -1,    -1,    -1,
      56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    79,    80,   132,     0,    81,    82,   132,     8,    83,
      84,    85,    89,    91,    92,    93,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   132,    81,    38,    35,    83,    71,     1,    14,
      15,    16,    19,    20,    21,    29,    32,    33,    34,    47,
      52,    53,    54,    56,    60,    61,    62,    63,    64,    66,
      69,   129,    70,    90,    52,    31,    38,    43,    52,    58,
      68,   131,    48,    69,    74,    94,    71,    75,   121,     8,
      52,    52,    52,    38,     3,    31,    38,    43,    58,    52,
       3,    72,    38,    52,    52,    27,    24,     4,    67,   130,
     121,   121,   121,   121,   121,   121,    71,   121,    94,    71,
       3,     6,     7,    10,    11,    12,    13,    18,    23,    25,
      26,    28,    30,    36,    38,    40,    42,    44,    46,    51,
      52,    56,    57,    58,    66,   122,   123,   121,   121,   121,
     121,   129,    49,    56,   121,   121,   121,   121,   121,    30,
      88,    89,   132,   121,   121,   121,   121,    18,    12,    71,
      28,    38,    70,    70,    70,    70,    70,    38,    70,    70,
      45,    70,    70,    42,    28,    42,    70,    70,    70,    70,
      28,    28,    70,    35,    41,    76,    77,    71,   121,   121,
     121,    73,    88,   121,    70,    70,    70,   124,   129,   125,
     131,     5,   124,   125,   124,    70,   125,    22,    37,    39,
      41,    59,    65,   126,   127,   128,    35,   124,    50,   125,
      70,    70,    70,     5,   124,   124,   124,   124,    70,    70,
     124,    70,    70,    71,   122,    41,    86,    87,   131,   124,
     124,   124,    70,   124,   124,   124,   124,   124,   125,   124,
      17,    55,    41,    87,    41,    86,    71,   125,    72,    72,
      83,    83,    73,    73
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
#line 245 "parser.y"
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
#line 258 "parser.y"
    {
						yyval.str = yyvsp[-1].str;
					}
    break;

  case 4:
#line 264 "parser.y"
    { yyval.adm_info = yyvsp[0].adm_info; }
    break;

  case 5:
#line 266 "parser.y"
    { yyvsp[-1].adm_info->next = yyvsp[0].adm_info; yyval.adm_info = yyvsp[-1].adm_info; }
    break;

  case 6:
#line 270 "parser.y"
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
#line 288 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 8:
#line 290 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 9:
#line 294 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 10:
#line 296 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 11:
#line 301 "parser.y"
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
#line 318 "parser.y"
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
#line 336 "parser.y"
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
#line 354 "parser.y"
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
#line 370 "parser.y"
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
#line 387 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 17:
#line 390 "parser.y"
    { yyval.statement = yyvsp[-1].statement; }
    break;

  case 18:
#line 394 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 19:
#line 396 "parser.y"
    { yyvsp[-1].statement->next = yyvsp[0].statement; yyval.statement = yyvsp[-1].statement; }
    break;

  case 20:
#line 400 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 21:
#line 402 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 22:
#line 404 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 23:
#line 406 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 24:
#line 408 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 25:
#line 410 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 26:
#line 412 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 27:
#line 414 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 28:
#line 416 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 29:
#line 418 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 30:
#line 420 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 31:
#line 422 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 32:
#line 424 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 33:
#line 426 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 34:
#line 428 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 35:
#line 430 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 36:
#line 432 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 37:
#line 434 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 38:
#line 436 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 39:
#line 438 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 40:
#line 440 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 41:
#line 442 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 42:
#line 444 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 43:
#line 446 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 44:
#line 448 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 45:
#line 450 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 46:
#line 452 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 47:
#line 454 "parser.y"
    { yyval.statement = yyvsp[0].statement; }
    break;

  case 48:
#line 455 "parser.y"
    { yyerrok; }
    break;

  case 49:
#line 456 "parser.y"
    { yyval.statement = yyvsp[-2].statement; }
    break;

  case 50:
#line 460 "parser.y"
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

  case 51:
#line 477 "parser.y"
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

  case 52:
#line 494 "parser.y"
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

  case 53:
#line 511 "parser.y"
    { yyval.ival = strtol(yytext, NULL, 10); }
    break;

  case 54:
#line 513 "parser.y"
    { yyval.ival = -yyvsp[0].ival; }
    break;

  case 55:
#line 517 "parser.y"
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

  case 56:
#line 532 "parser.y"
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

  case 57:
#line 560 "parser.y"
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

  case 58:
#line 590 "parser.y"
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

  case 59:
#line 618 "parser.y"
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

  case 60:
#line 646 "parser.y"
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

  case 61:
#line 672 "parser.y"
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

  case 62:
#line 704 "parser.y"
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

  case 63:
#line 734 "parser.y"
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

  case 64:
#line 764 "parser.y"
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

  case 65:
#line 794 "parser.y"
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

  case 66:
#line 824 "parser.y"
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

  case 67:
#line 852 "parser.y"
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

  case 68:
#line 882 "parser.y"
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

  case 69:
#line 910 "parser.y"
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

  case 70:
#line 948 "parser.y"
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

  case 71:
#line 982 "parser.y"
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

  case 72:
#line 1012 "parser.y"
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

  case 73:
#line 1042 "parser.y"
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

  case 74:
#line 1074 "parser.y"
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

  case 75:
#line 1102 "parser.y"
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

  case 76:
#line 1130 "parser.y"
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

  case 77:
#line 1158 "parser.y"
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

  case 78:
#line 1188 "parser.y"
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

  case 79:
#line 1219 "parser.y"
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

  case 80:
#line 1245 "parser.y"
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

  case 81:
#line 1277 "parser.y"
    { yyval.opt_list = NULL; }
    break;

  case 82:
#line 1279 "parser.y"
    { yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 83:
#line 1283 "parser.y"
    { yyval.opt_list = yyvsp[0].opt_list; }
    break;

  case 84:
#line 1285 "parser.y"
    { yyvsp[-2].opt_list->next = yyvsp[0].opt_list; yyval.opt_list = yyvsp[-2].opt_list; }
    break;

  case 85:
#line 1289 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 86:
#line 1294 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_BACKUP_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 87:
#line 1299 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_EVENT_NODE;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 88:
#line 1304 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SERVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 89:
#line 1309 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CLIENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 90:
#line 1314 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 91:
#line 1319 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_OLD_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 92:
#line 1324 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NEW_ORIGIN;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 93:
#line 1329 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_RECEIVER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 94:
#line 1334 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_PROVIDER;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 95:
#line 1339 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNRETRY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 96:
#line 1344 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_COMMENT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 97:
#line 1349 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_CONNINFO;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 98:
#line 1354 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_SET_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 99:
#line 1359 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_ADD_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 100:
#line 1364 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_NODE_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 101:
#line 1369 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TAB_ID;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 102:
#line 1374 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TRIG_NAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 103:
#line 1379 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FQNAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 104:
#line 1384 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_USE_KEY;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 105:
#line 1389 "parser.y"
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

  case 106:
#line 1402 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FORWARD;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 107:
#line 1407 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_FILENAME;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 108:
#line 1412 "parser.y"
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

  case 109:
#line 1425 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_WAIT_CONFIRMED;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 110:
#line 1430 "parser.y"
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

  case 111:
#line 1443 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_WAIT_ON;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 112:
#line 1448 "parser.y"
    {
						yyvsp[0].opt_list->opt_code	= O_TIMEOUT;
						yyval.opt_list = yyvsp[0].opt_list;
					}
    break;

  case 113:
#line 1455 "parser.y"
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

  case 114:
#line 1469 "parser.y"
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

  case 115:
#line 1483 "parser.y"
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

  case 116:
#line 1495 "parser.y"
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

  case 123:
#line 1519 "parser.y"
    {
						yyval.ival = strtol(yytext, NULL, 10);
					}
    break;

  case 124:
#line 1525 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 125:
#line 1537 "parser.y"
    {
						char   *ret;

						ret = malloc(yyleng + 1);
						memcpy(ret, yytext, yyleng);
						ret[yyleng] = '\0';

						yyval.str = ret;
					}
    break;

  case 126:
#line 1549 "parser.y"
    { yyval.ival = yylineno; }
    break;


    }

/* Line 991 of yacc.c.  */
#line 3025 "y.tab.c"

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


#line 1552 "parser.y"



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




