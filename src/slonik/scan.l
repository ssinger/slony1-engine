%{
/*-------------------------------------------------------------------------
 * scan.l
 *
 *	Flex keyword and token scanner for slonik
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: scan.l,v 1.12 2004-05-31 15:24:16 wieck Exp $
 *-------------------------------------------------------------------------
 */

%}

%option 8bit
%option nounput
%option noyywrap
%option yylineno
%option case-insensitive

%x	IN_STRING

digit			[0-9]
ident_start		[A-Za-z\200-\377_]
ident_cont		[A-Za-z\200-\377_0-9\$]
space			[ \t\n\r\f]

/*
quoted_ident	(\"[^\"]*\")+
identifier		({ident_start}{ident_cont}*|{quoted_ident})
*/
identifier		({ident_start}{ident_cont}*)

%%

	char   *start_charpos = NULL;

	BEGIN(INITIAL);

add				{ return K_ADD;					}
admin			{ return K_ADMIN;				}
all				{ return K_ALL;					}
backup			{ return K_BACKUP;				}
client			{ return K_CLIENT;				}
cluster			{ return K_CLUSTER;				}
comment			{ return K_COMMENT;				}
confirmed		{ return K_CONFIRMED;			}
conninfo		{ return K_CONNINFO;			}
connretry		{ return K_CONNRETRY;			}
create			{ return K_CREATE;				}
drop			{ return K_DROP;				}
echo			{ return K_ECHO;				}
error			{ return K_ERROR;				}
event			{ return K_EVENT;				}
execute			{ return K_EXECUTE;				}
exit			{ return K_EXIT;				}
failover		{ return K_FAILOVER;			}
false			{ return K_FALSE;				}
filename		{ return K_FILENAME;			}
for				{ return K_FOR;					}
forward			{ return K_FORWARD;				}
full			{ return K_FULL;				}
id				{ return K_ID;					}
init			{ return K_INIT;				}
key				{ return K_KEY;					}
listen			{ return K_LISTEN;				}
lock			{ return K_LOCK;				}
merge			{ return K_MERGE;				}
move			{ return K_MOVE;				}
name			{ return K_NAME;				}
new				{ return K_NEW;					}
no				{ return K_YES;					}
node			{ return K_NODE;				}
off				{ return K_OFF;					}
old				{ return K_OLD;					}
on				{ return K_ON;					}
origin			{ return K_ORIGIN;				}
path			{ return K_PATH;				}
provider		{ return K_PROVIDER;			}
qualified		{ return K_QUALIFIED;			}
receiver		{ return K_RECEIVER;			}
restart			{ return K_RESTART;				}
script			{ return K_SCRIPT;				}
sequence		{ return K_SEQUENCE;			}
serial			{ return K_SERIAL;				}
server			{ return K_SERVER;				}
set				{ return K_SET;					}
store			{ return K_STORE;				}
subscribe		{ return K_SUBSCRIBE;			}
success			{ return K_SUCCESS;				}
table			{ return K_TABLE;				}
timeout			{ return K_TIMEOUT;				}
trigger			{ return K_TRIGGER;				}
true			{ return K_TRUE;				}
try				{ return K_TRY;					}
uninstall		{ return K_UNINSTALL;			}
unlock			{ return K_UNLOCK;				}
unsubscribe		{ return K_UNSUBSCRIBE;			}
yes				{ return K_YES;					}
wait			{ return K_WAIT;				}

{digit}+		{ return T_NUMBER;				}
{identifier}	{ return T_IDENT;				}

{space}+		;

'				{ 
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

#[^\r\n]*		;

.				{ return yytext[0];				}

%%

