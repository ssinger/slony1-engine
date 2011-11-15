%{
/*-------------------------------------------------------------------------
 * scan.l
 *
 *	Flex keyword and token scanner for slony_logshipper
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	
 *-------------------------------------------------------------------------
 */

/*
 * Structure used to implement the input buffer stack.
 */
struct __yy_buffer
{
	YY_BUFFER_STATE	     buffer;		/* lexer buffer to restore on pop	*/
	long		         lineno;		/* line number to restore on pop	*/
	char               * fileName;		/* file name to restore on pop		*/
	FILE			   * yyin;			/* yyin to restore on pop			*/
	struct __yy_buffer * prev;			/* pointer to previous stack frame	*/
} * yy_buffer = NULL;

/*
 * Structure to hold defined symbols
 */
typedef struct _symbol
{
    char           * name;				/* Name of symbol with % prepended	*/
	char           * value;				/* Value of symbol					*/
	struct _symbol * next;				/* Pointer to next symbol			*/
} symbol;

/*
 * A buffer to send large items like literals in pieces to the parser.
 */
char yychunk[YY_BUF_SIZE / 2];

/*
 * Local data
 */
static symbol * symbols;				/* Head of list of symbols			*/

static char  *getSymbol(const char * name);	/* Return a symbol's value		*/
static void   addSymbol(char * str);	/* Add a new symbol					*/
static void   freeSymbols(void);		/* Free all symbols                 */
static void   pushBuffer(char *context);/* Push lexer buffer onto the stack	*/
static void   popBuffer( void );		/* Pop previous lexer buffer		*/

extern char * current_file;
%}


%{
#include "config.h"
#include "../slonik/types.h"
#include "libpq-fe.h"
#include "slony_logshipper.h"
#include "y.tab.h"

%}

%option 8bit
%option noyywrap
%option yylineno
%option case-insensitive
%option pointer

%x  incl define
%x	IN_STRING
%x	COPYSTART COPY COPYLS

digit			[0-9]
ident_start		[A-Za-z\200-\377_]
ident_cont		[A-Za-z\200-\377_0-9\$]
space			[ \t\n\r\f]
instr_start		(.|{space})

quoted_ident	(\"[^\"]*\")+
identifier		({ident_start}{ident_cont}*|{quoted_ident})

conf_comment	(#[^\n]*)
exec_ddl		(--{space}DDL_SCRIPT{space}*)
archive_comment	(--[^\n]*)

%%

include{space}* 		{ BEGIN(incl);				}
define{space}*  		{ BEGIN(define);			}

%{
/* ----------
 * Keywords
 * ----------
 */
%}
start_config			{ return K_START_CONFIG;	}
start_archive			{ return K_START_ARCHIVE;	}
{conf_comment}			{ return K_CONF_COMMENT;	}
{archive_comment}		{ return K_ARCHIVE_COMMENT;	}
{exec_ddl}				{ return K_EXEC_DDL;		}

analyze					{ return K_ANALYZE;			}
and						{ return K_AND;				}
archive					{ return K_ARCHIVE;			}
archives				{ return K_ARCHIVES;		}
cascade					{ return K_CASCADE;			}
cluster					{ return K_CLUSTER;			}
command					{ return K_COMMAND;			}
commit					{ return K_COMMIT;			}
copy					{ return K_COPY;			}
database				{ return K_DATABASE;		}
delete					{ return K_DELETE;			}
destination				{ return K_DESTINATION;		}
dir						{ return K_DIR;				}
error					{ return K_ERROR;			}
from					{ return K_FROM;			}
ignore					{ return K_IGNORE;			}
insert					{ return K_INSERT;			}
into					{ return K_INTO;			}
logfile					{ return K_LOGFILE;			}
max						{ return K_MAX;				}
name					{ return K_NAME;			}
namespace				{ return K_NAMESPACE;		}
null					{ return K_NULL;			}
only					{ return K_ONLY;			}
post					{ return K_POST;			}
pre						{ return K_PRE;				}
processing				{ return K_PROCESSING;		}
rename					{ return K_RENAME;			}
replica					{ return K_REPLICA;			}
select					{ return K_SELECT;			}
session_replication_role { return K_SESSION_ROLE;	}
set						{ return K_SET;				}
start					{ return K_START;			}
table					{ return K_TABLE;			}
to						{ return K_TO;				}
transaction				{ return K_TRANSACTION;		}
truncate				{ return K_TRUNCATE;		}
update					{ return K_UPDATE;			}
vacuum					{ return K_VACUUM;			}
values					{ return K_VALUES;			}
where					{ return K_WHERE;			}

archivetracking_offline { return T_TRACKING_FUNCTION; }
finishtableaftercopy	{ return T_FINISH_FUNCTION;	}
sequencesetvalue_offline { return T_SEQSETVAL_FUNCTION;	}
setval 					{ return T_PGSETVAL_FUNCTION;	}

%{
/* ----------
 * Generic "things"
 * ----------
 */
%}
{digit}+				{ return T_NUMBER;			}
{identifier}			{ return T_IDENT;			}

{space}+		;

'				{
					char	   *cp = yychunk;
					size_t		   len = 0;
					int			c;

					BEGIN(IN_STRING);
					while(len < sizeof(yychunk) - 1)
					{
						c = input();
						if (c == EOF)
						{
							errlog(LOG_ERROR, "%s: EOF inside literal\n",
									current_file);
							BEGIN(INITIAL);
							break;
						}
						if (c == '\\')
						{
							c = input();
							if (c == EOF)
							{
								errlog(LOG_ERROR, "%s: EOF inside literal\n",
										current_file);
								BEGIN(INITIAL);
								break;
							}
							*cp++ = c;
							*cp++ = c;
							len+=2;
							continue;
						}
						if (c == '\'')
						{
							c = input();
							if (c == EOF)
							{
								errlog(LOG_ERROR, "%s: EOF inside literal\n",
										current_file);
								BEGIN(INITIAL);
								break;
							}
							if (c == '\'')
							{
								*cp++ = c;
								*cp++ = c;
								len+=2;
								continue;
							}
							unput(c);
							*cp = '\0';
							BEGIN(INITIAL);
							return T_LITERAL;
						}
						*cp++ = c;
						len++;
					}
					*cp = '\0';
					return T_LITERAL_PART;
				}

<IN_STRING>{instr_start}	{
					char	   *cp = yychunk;
					size_t 		len = 0;
					int			c;
					bool		chunk_start;

					BEGIN(IN_STRING);

					chunk_start = true;
					while(len < sizeof(yychunk) - 1)
					{
						if (chunk_start)
						{
							c = *yytext;
							chunk_start = false;
						}
						else
						{
							c = input();
						}
						if (c == EOF)
						{
							errlog(LOG_ERROR, "%s: EOF inside literal\n",
									current_file);
							BEGIN(INITIAL);
							break;
						}
						if (c == '\\')
						{
							c = input();
							if (c == EOF)
							{
								errlog(LOG_ERROR, "%s: EOF inside literal\n",
										current_file);
								BEGIN(INITIAL);
								break;
							}
							*cp++ = c;
							*cp++ = c;
							len+=2;
							continue;
						}
						if (c == '\'')
						{
							c = input();
							if (c == EOF)
							{
								errlog(LOG_ERROR, "%s: EOF inside literal\n",
										current_file);
								BEGIN(INITIAL);
								break;
							}
							if (c == '\'')
							{
								*cp++ = c;
								*cp++ = c;
								len+=2;
								continue;
							}
							unput(c);
							*cp = '\0';
							BEGIN(INITIAL);
							return T_LITERAL;
						}
						*cp++ = c;
						len++;
					}
					*cp = '\0';
					return T_LITERAL_PART;
				}

<COPYSTART>{space}+	{
					BEGIN(COPYLS);
				}
<COPYLS>\\.\n	{
					BEGIN(INITIAL);
					return T_COPYEND;
				}
<COPYLS>\\.\r\n	{
					BEGIN(INITIAL);
					return T_COPYEND;
				}
<COPYLS>.|{space} {
					char	   *cp = yychunk;
					size_t	   	len = 0;
					int			c;
					bool		chunk_start;

					BEGIN(COPY);

					chunk_start = true;
					while(len < sizeof(yychunk) - 2)
					{
						if (chunk_start)
						{
							c = yytext[0];
							chunk_start = false;
						}
						else
						{
							c = input();
						}
						if (c == EOF)
						{
							errlog(LOG_ERROR, "%s: EOF inside of COPY data\n",
									current_file);
							BEGIN(INITIAL);
							return T_COPYEND;
						}
						*cp++ = c;
						len++;
						if (c == '\n')
						{
							*cp = '\0';
							BEGIN(COPYLS);
							return T_COPYDATA;
						}
						if (len == 5)
						{
							*cp = '\0';
							return T_COPYDATA_PART;
						}
					}
					*cp = '\0';
					return T_COPYDATA_PART;
				}

<COPY>.|{space}	{
					char	   *cp = yychunk;
					size_t			len = 0;
					int			c;
					bool		chunk_start;

					chunk_start = true;
					while(len < sizeof(yychunk) - 2)
					{
						if (chunk_start)
						{
							c = yytext[0];
							chunk_start = false;
						}
						else
						{
							c = input();
						}
						if (c == EOF)
						{
							errlog(LOG_ERROR, "%s: EOF inside of COPY data\n",
									current_file);
							BEGIN(INITIAL);
							return T_COPYEND;
						}
						*cp++ = c;
						len++;
						if (c == '\n')
						{
							*cp = '\0';
							BEGIN(COPYLS);
							return T_COPYDATA;
						}
						if (len == 5)
						{
							*cp = '\0';
							return T_COPYDATA_PART;
						}
					}
					*cp = '\0';
					return T_COPYDATA_PART;
				}


%{ /*
'		{ 
			start_charpos = yytext;
			BEGIN(IN_STRING);
		}
<IN_STRING>\\.	{ }
<IN_STRING>\\	{ }
<IN_STRING>''	{ }
<IN_STRING>'	{
			yyleng += (yytext - start_charpos) - 2;
			yytext = start_charpos + 1;
			BEGIN(INITIAL);
			return T_LITERAL;
		}
<IN_STRING>[^'\\]+ {}
*/
%}

<define>{identifier}{space}+.*";" { addSymbol( yytext ); BEGIN(INITIAL); }

@{identifier}   { 
                  char * value = getSymbol( yytext );
                  
                  if( value )
                  {
                    pushBuffer( strdup( current_file ));
                    yy_scan_string( value );
                  }
                }

<incl>\<[^\>]+\>{space}*";"? {

                    char * fileName = strdup( yytext + 1 ); /* Skip '<' */

                    *strchr( fileName, '>' ) = '\0';        /* Trim '>' */

                    pushBuffer( fileName );

                    if(( yyin = fopen( fileName, "r" )) == NULL )
                    {
                        errlog(LOG_ERROR,  "Include file (%s) not found\n", fileName );
                        exit( 1 );
                    }
                 
                    yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ));

                    BEGIN(INITIAL);
                }

<<EOF>>         {
                    if( yy_buffer == NULL )
					{
                        yyterminate();
						if (YY_CURRENT_BUFFER)
							yy_delete_buffer(YY_CURRENT_BUFFER);
					}
                    else
                        popBuffer();
                }

#[^\r\n]*		;

.		{ return yytext[0];				}

%%

void
scan_new_input_file(FILE *in)
{
	while (yy_buffer != NULL)
		popBuffer();

	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);

	yy_switch_to_buffer(yy_create_buffer(in, YY_BUF_SIZE));

	yylineno = 1;

    freeSymbols();
}

void scan_push_string(char *str)
{
	pushBuffer(strdup("init"));
	yy_scan_string (str);
}


int scan_yyinput(void)
{
	return input();
}

void scan_copy_start(void)
{
	BEGIN(COPYSTART);
}

void pushBuffer( char * context )
{
	struct __yy_buffer * yb = malloc( sizeof( *yb ));

	yb->buffer   = YY_CURRENT_BUFFER;
	yb->lineno   = yylineno;
	yb->fileName = current_file;
	yb->yyin	 = yyin;
	yb->prev     = yy_buffer;

	yy_buffer = yb;

	current_file = context;
	yylineno     = 1;
	yyin         = NULL;
}

static void popBuffer( void )
{
	struct __yy_buffer * yb = yy_buffer;

	if( yyin != NULL )
		fclose( yyin );
	
	yy_delete_buffer( YY_CURRENT_BUFFER );
	yy_switch_to_buffer( yy_buffer->buffer );
	
	current_file = yy_buffer->fileName;
	yylineno     = yy_buffer->lineno;
	yyin         = yy_buffer->yyin;
	
	yy_buffer = yy_buffer->prev;
	
	free( yb );
}

static void addSymbol( char * str )
{
	char   * name  = str;
	char   * value = str;
	symbol * sym   = NULL;

	while( *value != ' ' && *value != '\t' )
		value++;

	*(value++) = '\0';

	while( *value == ' ' || *value == '\t' )
		value++;

	value[strlen(value) - 1 ] = '\0';

	sym = malloc( sizeof( *sym ));
	sym->value = strdup( value );
	sym->next  = NULL;
	
	/* Store the symbol name in searchable form  with a leading @ */

	sym->name  = malloc( strlen( name ) + 1 + 1 );

	sym->name[0] = '@';
	strcpy( sym->name+1, name );

	if( symbols != NULL )
		sym->next = symbols;

	symbols = sym;
}

static char * getSymbol( const char * name )
{
	symbol * sym;

	for( sym = symbols; sym; sym = sym->next )
	{
		if( strcmp( name, sym->name ) == 0 )
			return( sym->value );
	}

	return( NULL );
}

static void freeSymbols( void )
{
	symbol * sym = symbols;

	while( sym )
	{
		symbol * victim = sym;

		sym = sym->next;

		free( victim->name );
		free( victim->value );
		free( victim );
	}

	symbols = NULL;
}
/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
