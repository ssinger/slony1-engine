%{
/*-------------------------------------------------------------------------
 * scan.l
 *
 *	Flex keyword and token scanner for slonik
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	
 *-------------------------------------------------------------------------
 */
struct __yy_buffer
{
	YY_BUFFER_STATE	     buffer;				/* lexer buffer to restore on pop	*/
	long		         lineno;				/* line number to restore on pop	*/
	char               * fileName;				/* file name to restore on pop		*/
	struct __yy_buffer * prev;					/* pointer to previous stack frame	*/
} * yy_buffer = NULL;

typedef struct _symbol
{
    char           * name;						/* Name of symbol with % prepended	*/
	char           * value;						/* Value of symbol					*/
	struct _symbol * next;						/* Pointer to next symbol			*/
} symbol;

static symbol * symbols;						/* Head of list of symbols			*/

static char * getSymbol( const char * name );	/* Return a symbol's value (or NULL)*/
static void   addSymbol( char * str );			/* Add a new symbol					*/
static void   freeSymbols( void );              /* Free all symbols                 */
static void   pushBuffer( char * context );		/* Push a new lexer buffer on stack */
static void   popBuffer( void );				/* Pop previous lexer buffer		*/

extern char * current_file;
%}

%{

#include "libpq-fe.h"
#include "slonik.h"
#include "y.tab.h"
%}

%option 8bit
%option nounput
%option noyywrap
%option yylineno
%option case-insensitive

%x  incl define
%x	IN_STRING

digit			[0-9]
ident_start		[A-Za-z\200-\377_]
ident_cont		[A-Za-z\200-\377_0-9\$]
space			[ \t\n\r\f]
quote           '

/*
quoted_ident	(\"[^\"]*\")+
identifier		({ident_start}{ident_cont}*|{quoted_ident})
*/
identifier		({ident_start}{ident_cont}*)

%%

	char   *instring_buffer = NULL;
	int     instring_length=0;
	int     instring_buf_length=0;

	BEGIN(INITIAL);

include{space}* { BEGIN(incl); }
define{space}*  { BEGIN(define); }

add				{ return K_ADD;				}
admin			{ return K_ADMIN;			}
all				{ return K_ALL;				}
backup			{ return K_BACKUP;			}
client			{ return K_CLIENT;			}
clone			{ return K_CLONE;			}
cluster			{ return K_CLUSTER;			}
comment			{ return K_COMMENT;			}
config			{ return K_CONFIG;			}
confirmed		{ return K_CONFIRMED;		}
conninfo		{ return K_CONNINFO;		}
connretry		{ return K_CONNRETRY;		}
copy			{ return K_COPY;			}
create			{ return K_CREATE;			}
date			{ return K_DATE;			}
drop			{ return K_DROP;			}
echo			{ return K_ECHO;			}
error			{ return K_ERROR;			}
event			{ return K_EVENT;			}
execute			{ return K_EXECUTE;			}
exit			{ return K_EXIT;			}
failover		{ return K_FAILOVER;		}
false			{ return K_FALSE;			}
filename		{ return K_FILENAME;		}
finish			{ return K_FINISH;			}
for				{ return K_FOR;				}
format			{ return K_DFORMAT;			}
forward			{ return K_FORWARD;			}
full			{ return K_FULL;			}
fully			{ return K_FULL;			}
functions		{ return K_FUNCTIONS;		}
id				{ return K_ID;				}
init			{ return K_INIT;			}
key				{ return K_KEY;				}
listen			{ return K_LISTEN;			}
lock			{ return K_LOCK;			}
log				{ return K_LOG;				}
merge			{ return K_MERGE;			}
move			{ return K_MOVE;			}
name			{ return K_NAME;			}
new				{ return K_NEW;				}
no				{ return K_NO;				}
node			{ return K_NODE;			}
off				{ return K_OFF;				}
old				{ return K_OLD;				}
omit			{ return K_OMIT;			}
on				{ return K_ON;				}
only			{ return K_ONLY;			}
origin			{ return K_ORIGIN;			}
path			{ return K_PATH;			}
prepare			{ return K_PREPARE;			}
provider		{ return K_PROVIDER;		}
qualified		{ return K_QUALIFIED;		}
receiver		{ return K_RECEIVER;		}
repair			{ return K_REPAIR;			}
restart			{ return K_RESTART;			}
resubscribe     { return K_RESUBSCRIBE;		}
script			{ return K_SCRIPT;			}
seconds         { return K_SECONDS;         }
sequence		{ return K_SEQUENCE;		}
sequences		{ return K_SEQUENCES;		}
server			{ return K_SERVER;			}
set				{ return K_SET;				}
sleep			{ return K_SLEEP;			}
sql				{ return K_SQL;				}
store			{ return K_STORE;			}
subscribe		{ return K_SUBSCRIBE;		}
success			{ return K_SUCCESS;			}
switch			{ return K_SWITCH;			}
sync			{ return K_SYNC;			}
table			{ return K_TABLE;			}
tables			{ return K_TABLES;			}
timeout			{ return K_TIMEOUT;			}
true			{ return K_TRUE;			}
try				{ return K_TRY;				}
uninstall		{ return K_UNINSTALL;		}
unlock			{ return K_UNLOCK;			}
unsubscribe		{ return K_UNSUBSCRIBE;		}
update			{ return K_UPDATE;			}
wait			{ return K_WAIT;			}
yes				{ return K_YES;				}
{digit}+		{ return T_NUMBER;			}
{identifier}	{ return T_IDENT;			}

{space}+		;

'		{ 
			if( instring_buffer==NULL )
			{
				instring_buffer=malloc(256);
				if( instring_buffer == NULL)
				{
						fprintf(stderr,"insufficient memory\n");
						exit(1);


				}		
				instring_buf_length=256;
			}
			instring_length=0;
			instring_buffer[instring_length]=*yytext;
			BEGIN(IN_STRING);
		}
<IN_STRING>''	{
				if (  instring_buf_length < instring_length + yyleng+1 )
				{
						instring_buffer=realloc(instring_buffer,instring_buf_length*2);
						instring_buf_length=instring_buf_length*2;
				}
				instring_buffer[instring_length++] = '\'';

				
		}
<IN_STRING>[^']+ { 
				if (  instring_buf_length < instring_length + yyleng+1 )
				{
						instring_buffer=realloc(instring_buffer,instring_buf_length*2);
						instring_buf_length=instring_buf_length*2;
				}
				strncpy(instring_buffer+instring_length,yytext,yyleng);
				instring_length+=yyleng;

				
		}
<IN_STRING>'	{
			instring_buffer[instring_length]='\0';
			yytext = instring_buffer;
			yyleng=instring_length;

			BEGIN(INITIAL);
			return T_LITERAL;
		}
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
                        fprintf( stderr,  "Include file (%s) not found\n", fileName );
                        exit( 1 );
                    }
                 
                    yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ));

                    BEGIN(INITIAL);
                }

<<EOF>>         {
                    if( yy_buffer == NULL )
                        yyterminate();
                    else
                        popBuffer();
                }

#[^\r\n]*		;

.		{ return yytext[0];				}

%%

void
scan_new_input_file(FILE *in)
{
	SlonDString		ds;
	char			buf[8192];
	int				len;

	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);

	dstring_init(&ds);
	while ((len = fread(&buf, 1, 8192, in)) > 0)
	{
		dstring_nappend(&ds, buf, len);
	}
	dstring_terminate(&ds);
	yy_switch_to_buffer(yy_scan_string(dstring_data(&ds)));

	dstring_free(&ds);

    freeSymbols();
}

static void pushBuffer( char * context )
{
	struct __yy_buffer * yb = malloc( sizeof( *yb ));

	yb->buffer   = YY_CURRENT_BUFFER;
	yb->lineno   = yylineno;
	yb->fileName = strdup( current_file );
	yb->prev     = yy_buffer;

	yy_buffer = yb;

	current_file = context;
	yylineno     = 1;
}

static void popBuffer( void )
{
	struct __yy_buffer * yb = yy_buffer;

	if( yyin != NULL )
		fclose( yyin );
	
	yy_delete_buffer( YY_CURRENT_BUFFER );
	yy_switch_to_buffer( yy_buffer->buffer );
	
	free( current_file );
	
	current_file = yy_buffer->fileName;
	yylineno     = yy_buffer->lineno;
	
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
