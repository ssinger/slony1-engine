/*-------------------------------------------------------------------------
 * misc.c
 *
 *	Miscellaneous support functions
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: misc.c,v 1.4 2004-02-25 19:47:37 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "libpq-fe.h"
#include "c.h"

#include "slon.h"


static pthread_mutex_t	log_mutex = PTHREAD_MUTEX_INITIALIZER;


void
slon_log(SlonLogLevel level, char *fmt, ...)
{
	va_list			ap;
	static char	   *outbuf = NULL;
	static int		outsize = -1;
	int				off;
	char		   *level_c = NULL;;

	if (false && level >= SLON_DEBUG1)
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

	if (outbuf == NULL)
	{
		outsize = 1024;
		outbuf = malloc(outsize);
		if (outbuf == NULL)
		{
			perror("slon_log: malloc()");
			slon_abort();
		}
	}

	sprintf(outbuf, "%-6.6s ", level_c); /* date and time here too */
	off = strlen(outbuf);

	while(vsnprintf(&outbuf[off], outsize - off, fmt, ap) < 0)
	{
		outsize *= 2;
		outbuf = realloc(outbuf, outsize);
		if (outbuf == NULL)
		{
			perror("slon_log: realloc()");
			slon_abort();
		}
	}

	pthread_mutex_lock(&log_mutex);
	fwrite(outbuf, strlen(outbuf), 1, stdout);
	fflush(stdout);
	pthread_mutex_unlock(&log_mutex);

	va_end(ap);
}
