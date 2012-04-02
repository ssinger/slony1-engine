/*	*/
#include <stdio.h>
#include "scanner.h"

int			STMTS[MAXSTATEMENTS];
int
scan_for_statements(const char *extended_statement)
{
	int			cpos;
	int			bquote;
	int			bpos;
	enum quote_states state;
	char		cchar;
	int			d1start,
				d1end,
				d2start,
				d2end,
				d1stemp;
	int			statements;
	int			nparens;
	int			nbrokets;
	int			nsquigb;

	/* Initialize */
	cpos = 0;
	statements = 0;
	bquote = 0;
	bpos = 0;					/* Location of last backquote */
	d1start = 0;
	d2start = 0;
	d1end = 0;
	state = Q_NORMAL_STATE;
	nparens = 0;
	nbrokets = 0;
	nsquigb = 0;

	while (state != Q_DONE)
	{
		cchar = extended_statement[cpos];
		switch (cchar)
		{
			case '\0':
				STMTS[statements++] = cpos;
				state = Q_DONE;
				break;

			case '(':
				if (state == Q_NORMAL_STATE)
				{
					nparens++;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;

			case ')':
				if (state == Q_NORMAL_STATE)
				{
					nparens--;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
			case '[':
				if (state == Q_NORMAL_STATE)
				{
					nbrokets++;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
			case ']':
				if (state == Q_NORMAL_STATE)
				{
					nbrokets--;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
			case '{':
				if (state == Q_NORMAL_STATE)
				{
					nsquigb++;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;

			case '}':
				if (state == Q_NORMAL_STATE)
				{
					nsquigb--;
					break;
				}

				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
			case '/':
				if (state == Q_NORMAL_STATE)
				{
					state = Q_HOPE_TO_CCOMMENT;
					break;
				}
				if (state == Q_HOPE_CEND)
				{
					state = Q_NORMAL_STATE;
					break;
				}
				break;
			case '*':
				if (state == Q_HOPE_TO_CCOMMENT)
				{
					state = Q_CCOMMENT;
					break;
				}
				if (state == Q_CCOMMENT)
				{
					state = Q_HOPE_CEND;
					break;
				}

				break;
			case '\\':
				if ((state == Q_DOUBLE_QUOTING) || (state == Q_SINGLE_QUOTING))
				{
					if (bquote == 0)
					{
						bquote = 1;
						bpos = cpos;
						break;
					}
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '$':
				if (state == Q_NORMAL_STATE)
				{
					d1start = cpos;
					state = Q_DOLLAR_BUILDING;
					break;
				}
				if (state == Q_DOLLAR_BUILDING)
				{
					d1end = cpos;
					state = Q_DOLLAR_QUOTING;
					break;
				}
				if (state == Q_DOLLAR_QUOTING)
				{
					d2start = cpos;
					state = Q_DOLLAR_UNBUILDING;
					break;
				}
				if (state == Q_DOLLAR_UNBUILDING)
				{
					d2end = cpos;

					/*
					 * Compare strings - is this the delimiter the imperials
					 * are looking for?
					 */
					if ((d1end - d1start) != (d2end - d2start))
					{
						/*
						 * Lengths don't even match - these aren't the droids
						 * we're looking for
						 */
						state = Q_DOLLAR_QUOTING;		/* Return to dollar
														 * quoting mode */
						break;
					}
					d1stemp = d1start;
					while (d1stemp < d1end)
					{
						if (extended_statement[d1stemp] != extended_statement[d2start])
						{
							/* mismatch - these aren't the droids... */
							state = Q_DOLLAR_QUOTING;
							break;
						}
						d1stemp++;		/* Step forward to the next character */
						d2start++;
					}
					if ((d1stemp >= d1end) && (state == Q_DOLLAR_UNBUILDING))
					{			/* No mismatches */
						state = Q_NORMAL_STATE;
						break;
					}
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '"':
				if (state == Q_NORMAL_STATE)
				{
					state = Q_DOUBLE_QUOTING;
					break;
				}
				if (state == Q_DOUBLE_QUOTING)
				{
					/* But a backquote hides this! */
					if ((bquote == 1) && (bpos == cpos - 1))
					{
						break;	/* Ignore the quote */
					}
					state = Q_NORMAL_STATE;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '\'':
				if (state == Q_NORMAL_STATE)
				{
					state = Q_SINGLE_QUOTING;
					break;
				}
				if (state == Q_SINGLE_QUOTING)
				{
					/* But a backquote hides this! */
					if ((bquote == 1) && (bpos == cpos - 1))
					{
						break;	/* Ignore the quote */
					}
					state = Q_NORMAL_STATE;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '-':
				if (state == Q_NORMAL_STATE && extended_statement[cpos + 1] == '-')
				{
					state = Q_HOPE_TO_DASH;
					break;
				}
				if (state == Q_HOPE_TO_DASH)
				{
					state = Q_DASHING_STATE;
					break;
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '\n':
				if (state == Q_DASHING_STATE)
				{
					state = Q_NORMAL_STATE;
				}
				if (state == Q_DOLLAR_BUILDING)
					state = Q_NORMAL_STATE;
				if (state == Q_DOLLAR_UNBUILDING)
					state = Q_DOLLAR_QUOTING;
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case '\r':
				if (state == Q_DASHING_STATE)
				{
					state = Q_NORMAL_STATE;
				}
				if (state == Q_DOLLAR_BUILDING)
					state = Q_NORMAL_STATE;
				if (state == Q_DOLLAR_UNBUILDING)
					state = Q_DOLLAR_QUOTING;
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case ' ':
				if (state == Q_DOLLAR_BUILDING)
					state = Q_NORMAL_STATE;
				if (state == Q_DOLLAR_UNBUILDING)
					state = Q_DOLLAR_QUOTING;
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			case ';':
				if ((state == Q_NORMAL_STATE) && (nparens == 0) && (nbrokets == 0) && (nsquigb == 0))
				{
					STMTS[statements++] = cpos + 1;
					if (statements >= MAXSTATEMENTS)
					{
						return statements;
					}
				}
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
			default:
				if (state == Q_HOPE_CEND)
					state = Q_CCOMMENT;
				break;
		}
		cpos++;
	}
	return statements;
}
