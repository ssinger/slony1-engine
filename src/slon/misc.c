/*-------------------------------------------------------------------------
 * misc.c
 *
 *	Miscellaneous support functions
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: misc.c,v 1.10.2.1 2004-10-14 15:58:21 cbbrowne Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "libpq-fe.h"
#include "c.h"

#include "slon.h"


int		slon_log_level = SLON_INFO;


static pthread_mutex_t	log_mutex = PTHREAD_MUTEX_INITIALIZER;


void
slon_log(SlonLogLevel level, char *fmt, ...)
{
	va_list			ap;
	static char	   *outbuf = NULL;
	static int		outsize = -1;
	int				off;
	char		   *level_c = NULL;;

	if (level > slon_log_level)
		return;

	switch (level)
	{
		case SLON_DEBUG4:	level_c = "DEBUG4";
							break;
		case SLON_DEBUG3:	level_c = "DEBUG3";
							break;
		case SLON_DEBUG2:	level_c = "DEBUG2";
							break;
		case SLON_DEBUG1:	level_c = "DEBUG1";
							break;
		case SLON_INFO:		level_c = "INFO";
							break;
		case SLON_CONFIG:	level_c = "CONFIG";
							break;
		case SLON_WARN:		level_c = "WARN";
							break;
		case SLON_ERROR:	level_c = "ERROR";
							break;
		case SLON_FATAL:	level_c = "FATAL";
							break;
	}

	va_start(ap, fmt);

	pthread_mutex_lock(&log_mutex);
	if (outbuf == NULL)
	{
		outsize = 8192;
		outbuf = malloc(outsize);
		if (outbuf == NULL)
		{
			perror("slon_log: malloc()");
			pthread_mutex_unlock(&log_mutex);
			slon_abort();
		}
	}

	sprintf(outbuf, "%-6.6s ", level_c); /* date and time here too */
	off = strlen(outbuf);

	while(vsnprintf(&outbuf[off], outsize - off, fmt, ap) >= outsize - off)
	{
		outsize *= 2;
		outbuf = realloc(outbuf, outsize);
		if (outbuf == NULL)
		{
			perror("slon_log: realloc()");
			slon_abort();
		}
	}

	fwrite(outbuf, strlen(outbuf), 1, stdout);
	fflush(stdout);
	pthread_mutex_unlock(&log_mutex);

	va_end(ap);
}


/*
 * scanint8 --- try to parse a string into an int8.
 *
 * If errorOK is false, ereport a useful error message if the string is bad.
 * If errorOK is true, just return "false" for bad input.
 */
int
slon_scanint64(char *str, int64 *result)
{
	char	   *ptr = str;
	int64		tmp = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be
	 * broken for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char) *ptr))
		ptr++;

	/* handle sign */
	if (*ptr == '-')
	{
		ptr++;
		sign = -1;

		/*
		 * Do an explicit check for INT64_MIN.	Ugly though this is, it's
		 * cleaner than trying to get the loop below to handle it
		 * portably.
		 */
#ifndef INT64_IS_BUSTED
		if (strcmp(ptr, "9223372036854775808") == 0)
		{
			*result = -INT64CONST(0x7fffffffffffffff) - 1;
			return true;
		}
#endif
	}
	else if (*ptr == '+')
		ptr++;

	/* require at least one digit */
	if (!isdigit((unsigned char) *ptr))
		return false;

	/* process digits */
	while (*ptr && isdigit((unsigned char) *ptr))
	{
		int64		newtmp = tmp * 10 + (*ptr++ - '0');

		if ((newtmp / 10) != tmp)		/* overflow? */
			return false;
		tmp = newtmp;
	}

	/* trailing junk? */
	if (*ptr)
		return false;

	*result = (sign < 0) ? -tmp : tmp;

	return true;
}

/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
