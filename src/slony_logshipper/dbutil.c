/*-------------------------------------------------------------------------
 * dbutil.c
 *
 *	General database support functions.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#endif

#include "libpq-fe.h"
#include "../slonik/types.h"
#include "slony_logshipper.h"


/*
 * Local functions
 */
static int	slon_appendquery_int(SlonDString * dsp, char *fmt, va_list ap);


/* ----------
 * slon_mkquery
 *
 *	A simple query formatting and quoting function using dynamic string
 *	buffer allocation.
 *	Similar to sprintf() it uses formatting symbols:
 *		%s		String argument
 *		%q		Quoted literal (\ and ' will be escaped)
 *		%d		Integer argument
 * ----------
 */
int
slon_mkquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	dstring_reset(dsp);

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);

	return 0;
}


/* ----------
 * slon_appendquery
 *
 *	Append query string material to an existing dynamic string.
 * ----------
 */
int
slon_appendquery(SlonDString * dsp, char *fmt,...)
{
	va_list		ap;

	va_start(ap, fmt);
	slon_appendquery_int(dsp, fmt, ap);
	va_end(ap);

	dstring_terminate(dsp);

	return 0;
}


/* ----------
 * slon_appendquery_int
 *
 *	Implementation of slon_mkquery() and slon_appendquery().
 * ----------
 */
static int
slon_appendquery_int(SlonDString * dsp, char *fmt, va_list ap)
{
	char	   *s;
	char		buf[64];

	while (*fmt)
	{
		switch (*fmt)
		{
			case '%':
				fmt++;
				switch (*fmt)
				{
					case 's':
						s = va_arg(ap, char *);
						dstring_append(dsp, s);
						fmt++;
						break;

					case 'q':
						s = va_arg(ap, char *);
						while (s && *s != '\0')
						{
							switch (*s)
							{
								case '\'':
									dstring_addchar(dsp, '\'');
									break;
								case '\\':
									dstring_addchar(dsp, '\\');
									break;
								default:
									break;
							}
							dstring_addchar(dsp, *s);
							s++;
						}
						fmt++;
						break;

					case 'Q':
						s = va_arg(ap, char *);
						while (s && *s != '\0')
						{
							switch (*s)
							{
								case '\'':
								case '\\':
									dstring_addchar(dsp, '\\');
									break;
								default:
									break;
							}
							dstring_addchar(dsp, *s);
							s++;
						}
						fmt++;
						break;

					case 'd':
						sprintf(buf, "%d", va_arg(ap, int));
						dstring_append(dsp, buf);
						fmt++;
						break;

					default:
						dstring_addchar(dsp, '%');
						dstring_addchar(dsp, *fmt);
						fmt++;
						break;
				}
				break;

			case '\\':
				fmt++;
				dstring_addchar(dsp, *fmt);
				fmt++;
				break;

			default:
				dstring_addchar(dsp, *fmt);
				fmt++;
				break;
		}
	}

	dstring_terminate(dsp);

	return 0;
}
