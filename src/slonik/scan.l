%{
/*-------------------------------------------------------------------------
 * scan.l
 *
 *	Flex keyword and token scanner for slonik
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: scan.l,v 1.2 2004-03-11 23:26:12 wieck Exp $
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
client			{ return K_CLIENT;				}
cluster			{ return K_CLUSTER;				}
comment			{ return K_COMMENT;				}
conninfo		{ return K_CONNINFO;			}
connretry		{ return K_CONNRETRY;			}
create			{ return K_CREATE;				}
drop			{ return K_DROP;				}
echo			{ return K_ECHO;				}
error			{ return K_ERROR;				}
event			{ return K_EVENT;				}
exit			{ return K_EXIT;				}
forward			{ return K_FORWARD;				}
full			{ return K_FULL;				}
id				{ return K_ID;					}
init			{ return K_INIT;				}
key				{ return K_KEY;					}
listen			{ return K_LISTEN;				}
name			{ return K_NAME;				}
no				{ return K_YES;					}
node			{ return K_NODE;				}
on				{ return K_ON;					}
origin			{ return K_ORIGIN;				}
path			{ return K_PATH;				}
provider		{ return K_PROVIDER;			}
qualified		{ return K_QUALIFIED;			}
receiver		{ return K_RECEIVER;			}
restart			{ return K_RESTART;				}
serial			{ return K_SERIAL;				}
server			{ return K_SERVER;				}
set				{ return K_SET;					}
store			{ return K_STORE;				}
subscribe		{ return K_SUBSCRIBE;			}
success			{ return K_SUCCESS;				}
table			{ return K_TABLE;				}
try				{ return K_TRY;					}
yes				{ return K_YES;					}

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
<IN_STRING>\\.	{ }
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

