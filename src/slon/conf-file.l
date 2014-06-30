/* Scanner for slon config file */

%{
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "slon.h"


#define CONFIG_FILENAME "./slon.conf"

static unsigned ConfigFileLineno;

enum {
        SLON_ID = 1,
        SLON_STRING = 2,
        SLON_INTEGER = 3,
        SLON_REAL = 4,
        SLON_EQUALS = 5,
        SLON_UNQUOTED_STRING = 6,
        SLON_QUALIFIED_ID = 7,
	SLON_ESCAPED_STRING = 8,
        SLON_EOL = 99,
        SLON_FERROR = 100
};

#define YY_USER_INIT (ConfigFileLineno = 1)

/* prototype, so compiler is happy with our high warnings setting */
int SLON_yylex(void);
char *SLON_scanstr(char *);
%}

%option 8bit
%option never-interactive
%option nodefault
%option nounput
%option noyywrap

SIGN            ("-"|"+")
DIGIT           [0-9]
HEXDIGIT        [0-9a-fA-F]

INTEGER         {SIGN}?({DIGIT}+|0x{HEXDIGIT}+)

EXPONENT        [Ee]{SIGN}?{DIGIT}+
REAL            {SIGN}?{DIGIT}*"."{DIGIT}*{EXPONENT}?

LETTER          [A-Za-z_\200-\377]
LETTER_OR_DIGIT [A-Za-z_0-9\200-\377]

ID              {LETTER}{LETTER_OR_DIGIT}*
QUALIFIED_ID    {ID}"."{ID}

UNQUOTED_STRING {LETTER}({LETTER_OR_DIGIT}|[-._:/])*
STRING          \'([^'\n]|\\.)*\'
ESCAPED_STRING  \"([^"\n]|\\.)*\"
%%

\n              ConfigFileLineno++; return SLON_EOL;
[ \t\r]+        /* eat whitespace */
#.*             /* eat comment */

{ID}            return SLON_ID;
{QUALIFIED_ID}  return SLON_QUALIFIED_ID;
{STRING}        return SLON_STRING;
{UNQUOTED_STRING} return SLON_UNQUOTED_STRING;
{ESCAPED_STRING}  return SLON_ESCAPED_STRING;
{INTEGER}       return SLON_INTEGER;
{REAL}          return SLON_REAL;
=               return SLON_EQUALS;

.               return SLON_FERROR;

%%

struct name_value_pair
{
        char       *name;
        char       *value;
        struct name_value_pair *next;
};

/*
 * Free a list of name/value pairs, including the names and the values
 */
static void
free_name_value_list(struct name_value_pair * list)
{
        struct name_value_pair *item;

        item = list;
        while (item)
        {
                struct name_value_pair *save;

                save = item->next;
                free(item->name);
                free(item->value);
                free(item);
                item = save;
        }
}

void ProcessConfigFile(const char *filename)
{
	int token, parse_state;
	char *opt_name, *opt_value;
	int elevel;
	struct name_value_pair *item, *head, *tail;
	FILE * fp;

	elevel = SLON_ERROR;

	/*
	 * Open the conf file
	 */
	if (filename == NULL)
	{
		fp = fopen(CONFIG_FILENAME, "r");
	}
	else
	{
		fp = fopen(filename, "r");
	}
	if (!fp)
	{
		/* File not found is fine */
		if (errno != ENOENT)
		{
			slon_log(elevel,"could not open configuration file \"%s\"\n", filename);
			
		}
		return;
	} 

	/* Ok we have the file, lets parse it */

	yyin = fp;
	parse_state = 0;
	head = NULL;
	tail = head;
	opt_name = NULL;
	opt_value = opt_name;

	while ( (token = yylex()) )
	{
		switch(parse_state)
		{
			case 0:
				if (token == SLON_EOL) /* empty line */
				{
					continue;
				}
				if (token != SLON_ID && token != SLON_QUALIFIED_ID)
				{
					goto parse_error;
				}
				opt_name = strdup(yytext);
				parse_state = 1;
				break;
			case 1:
				/* ignore equals sign */
				if (token == SLON_EQUALS)
				{
					token =  yylex();
				}
				if (token != SLON_ID && token != SLON_STRING && token != SLON_INTEGER && token != SLON_REAL && token != SLON_UNQUOTED_STRING && token != SLON_ESCAPED_STRING)
				{
					goto parse_error;
				}
				opt_value = strdup(yytext);
				if (token == SLON_STRING || token == SLON_ESCAPED_STRING)
				{
					memmove(opt_value,opt_value+1,strlen(opt_value)-1);
					opt_value[strlen(opt_value)-2]='\0';
					opt_value=SLON_scanstr(opt_value);
				}
				parse_state = 2;
				break;
			case 2:	/* OEL ? */
				if (token != SLON_EOL)
				{
					 goto parse_error;
				}
				item = malloc(sizeof *item);
				item->name = opt_name;
				item->value = opt_value;

				if (strcmp(opt_name, "custom_variable_classes") == 0)
				{
					item->next = head;
					head = item;
					if (!tail)
					{
						tail = item;
					}
				}
				else
				{
					/* append to list */
					item->next = NULL;
					if (!head)
					{
						head = item;
					}
					else
					{
						tail->next = item;
					}
					tail = item;
				}
				parse_state = 0;
				break;
		}
	}
	/*
	 * If we encountered an EOF after we've already
	 * reached parse_state of 2, we already have a complete
	 * configuration line, it's just terminated with EOF
	 * instead of EOL. Store that config option.
	 */
	if(parse_state == 2) 
	{
		item = malloc(sizeof *item);
		item->name = opt_name;
		item->value = opt_value;
		if (strcmp(opt_name, "custom_variable_classes") == 0)
		{
			item->next = head;
			head = item;
			if (!tail)
			{
				tail = item;
			}
		}
		else
		{
			/* append to list */
			item->next = NULL;
			if (!head)
			{
				head = item;
			}
			else
			{
				tail->next = item;
			}
			tail = item;
		}
		parse_state = 0;
	}

	fclose(fp);

	for(item = head; item; item=item->next)
	{
		set_config_option(item->name, item->value);
	}

	cleanup_exit:
		free_name_value_list(head);
		return;

	parse_error:
		fclose(fp);
		free_name_value_list(head);
		if (token == SLON_EOL)
		{
			slon_log(elevel, "syntax error in file \"%s\" line %u, near end of line\n", filename, ConfigFileLineno - 1);
		}
		
		else
		{
			slon_log(elevel, "syntax error in file \"%s\" line %u, near end of line\n", filename, ConfigFileLineno - 1);
		}
}







char *SLON_scanstr(char *s)
{
        char       *newStr;
        int                     len,
                                i,
                                j;

        if (s == NULL || s[0] == '\0')
        {
                if (s != NULL)
                        free(s);
                return strdup("");
        }
        len = strlen(s);

        newStr = malloc(len + 1);       /* string cannot get longer */
        if (newStr == NULL)
		slon_log(SLON_FATAL, "out of memory\n");

        for (i = 0, j = 0; i < len; i++)
        {
                if (s[i] == '\\')
                {
                        i++;
                        switch (s[i])
                        {
                                case 'b':
                                        newStr[j] = '\b';
                                        break;
                                case 'f':
                                        newStr[j] = '\f';
                                        break;
                                case 'n':
                                        newStr[j] = '\n';
                                        break;
                                case 'r':
                                        newStr[j] = '\r';
                                        break;
                                case 't':
                                        newStr[j] = '\t';
                                        break;
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                        {
                                                int                     k;
                                                long            octVal = 0;

                                                for (k = 0;
                                                         s[i + k] >= '0' && s[i 
+ k] <= '7' && k < 3;
                                                         k++)
                                                        octVal = (octVal << 3) +
 (s[i + k] - '0');
                                                i += k - 1;
                                                newStr[j] = ((char) octVal);
                                        }
                                        break;
                                default:
                                        newStr[j] = s[i];
                                        break;
                                }
                        }                                       /* switch */
                else
                        newStr[j] = s[i];
                j++;
        }
        newStr[j] = '\0';
        free(s);
        return newStr;
}

/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
