/*-------------------------------------------------------------------------
 * misc.c
 *
 *	Miscellaneous support functions
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: misc.c,v 1.20 2005-08-12 11:07:50 dpage Exp $
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

#ifndef WIN32
#include <syslog.h>
#else
#include "port/win32service.h"
#endif
#include <stdarg.h>

#include "libpq-fe.h"
#include "c.h"

#include "confoptions.h"
#include "slon.h"


extern int	slon_log_level;

extern bool logpid;
extern bool logtimestamp;
extern char *log_timestamp_format;

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef HAVE_SYSLOG
/*
 * 0 = only stdout/stderr
 * 1 = stdout+stderr and syslog
 * 2 = syslog only
 * ... in theory anyway
 */

#ifndef SLON_SYSLOG_LIMIT
#define SLON_SYSLOG_LIMIT 128
#endif

extern int	Use_syslog;
extern char *Syslog_facility;	/* openlog() parameters */
extern char *Syslog_ident;

static void write_syslog(int level, const char *line);

#else

#define Use_syslog 0
#endif   /* HAVE_SYSLOG */


void
slon_log(Slon_Log_Level level, char *fmt,...)
{
	va_list		ap;
	static char *outbuf = NULL;
	static int	outsize = -1;
	int			off;
	char	   *level_c = NULL;

	char		time_buf[128];
	time_t		stamp_time = time(NULL);

#ifdef HAVE_SYSLOG
	int			syslog_level = LOG_ERR;
#endif
	if (level > slon_log_level)
		return;

	switch (level)
	{
		case SLON_DEBUG4:
			level_c = "DEBUG4";
			break;
		case SLON_DEBUG3:
			level_c = "DEBUG3";
			break;
		case SLON_DEBUG2:
			level_c = "DEBUG2";
			break;
		case SLON_DEBUG1:
			level_c = "DEBUG1";
#ifdef HAVE_SYSLOG
			syslog_level = LOG_DEBUG;
#endif
			break;
		case SLON_INFO:
			level_c = "INFO";
#ifdef HAVE_SYSLOG
			syslog_level = LOG_INFO;
#endif
			break;
		case SLON_CONFIG:
			level_c = "CONFIG";
			break;
		case SLON_WARN:
			level_c = "WARN";
#ifdef HAVE_SYSLOG
			syslog_level = LOG_WARNING;
#endif
			break;
		case SLON_ERROR:
			level_c = "ERROR";
#ifdef HAVE_SYSLOG
			syslog_level = LOG_ERR;
#endif
			break;
		case SLON_FATAL:
			level_c = "FATAL";
#ifdef HAVE_SYSLOG
			syslog_level = LOG_ERR;
#endif

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
	outbuf[0] = 0;

	if (logtimestamp == true && (Use_syslog != 1)
#ifdef WIN32
			&& !win32_isservice
#endif
			)
	{
		strftime(time_buf, sizeof(time_buf), log_timestamp_format, localtime(&stamp_time));
		sprintf(outbuf, "%s ", time_buf);
	}
	if (logpid == true)
	{
		sprintf(outbuf, "%s[%d] ", outbuf, slon_pid);
	}
	sprintf(outbuf, "%s%-6.6s ", outbuf, level_c);

	off = strlen(outbuf);

	while (vsnprintf(&outbuf[off], outsize - off, fmt, ap) >= outsize - off)
	{
		outsize *= 2;
		outbuf = realloc(outbuf, outsize);
		if (outbuf == NULL)
		{
			perror("slon_log: realloc()");
			slon_abort();
		}
	}
#ifdef HAVE_SYSLOG
	if (Use_syslog >= 1)
	{
		write_syslog(syslog_level, outbuf);
	}
#endif
#ifdef WIN32
	if (win32_isservice)
		win32_eventlog(level, outbuf);
#endif
	fwrite(outbuf, strlen(outbuf), 1, stdout);
	fflush(stdout);
	pthread_mutex_unlock(&log_mutex);

	va_end(ap);
}


/*
 * scanint8 --- try to parse a string into an int8.
 *
 * If errorOK is false, ereport a useful error message if the string is bad. If
 * errorOK is true, just return "false" for bad input.
 */
int
slon_scanint64(char *str, int64 * result)
{
	char	   *ptr = str;
	int64		tmp = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be broken
	 * for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char)*ptr))
		ptr++;

	/* handle sign */
	if (*ptr == '-')
	{
		ptr++;
		sign = -1;

		/*
		 * Do an explicit check for INT64_MIN.	Ugly though this is, it's
		 * cleaner than trying to get the loop below to handle it portably.
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
	if (!isdigit((unsigned char)*ptr))
		return false;

	/* process digits */
	while (*ptr && isdigit((unsigned char)*ptr))
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

#if HAVE_SYSLOG
static void
write_syslog(int level, const char *line)
{
	static bool openlog_done = false;
	static unsigned long seq = 0;
	static int	syslog_fac = LOG_LOCAL0;

	int			len = strlen(line);

	if (Use_syslog == 0)
		return;

	if (!openlog_done)
	{
		if (strcasecmp(Syslog_facility, "LOCAL0") == 0)
			syslog_fac = LOG_LOCAL0;
		if (strcasecmp(Syslog_facility, "LOCAL1") == 0)
			syslog_fac = LOG_LOCAL1;
		if (strcasecmp(Syslog_facility, "LOCAL2") == 0)
			syslog_fac = LOG_LOCAL2;
		if (strcasecmp(Syslog_facility, "LOCAL3") == 0)
			syslog_fac = LOG_LOCAL3;
		if (strcasecmp(Syslog_facility, "LOCAL4") == 0)
			syslog_fac = LOG_LOCAL4;
		if (strcasecmp(Syslog_facility, "LOCAL5") == 0)
			syslog_fac = LOG_LOCAL5;
		if (strcasecmp(Syslog_facility, "LOCAL6") == 0)
			syslog_fac = LOG_LOCAL6;
		if (strcasecmp(Syslog_facility, "LOCAL7") == 0)
			syslog_fac = LOG_LOCAL7;
		openlog(Syslog_ident, LOG_PID | LOG_NDELAY, syslog_fac);
		openlog_done = true;
	}

	/*
	 * We add a sequence number to each log message to suppress "same"
	 * messages.
	 */
	seq++;

	/* divide into multiple syslog() calls if message is too long */
	/* or if the message contains embedded NewLine(s) '\n' */
	if (len > SLON_SYSLOG_LIMIT || strchr(line, '\n') != NULL)
	{
		int			chunk_nr = 0;

		while (len > 0)
		{
			char		buf    [SLON_SYSLOG_LIMIT + 1];
			int			buflen;
			int			i;

			/* if we start at a newline, move ahead one char */
			if (line[0] == '\n')
			{
				line++;
				len--;
				continue;
			}

			strncpy(buf, line, SLON_SYSLOG_LIMIT);
			buf[SLON_SYSLOG_LIMIT] = '\0';
			if (strchr(buf, '\n') != NULL)
				*strchr(buf, '\n') = '\0';

			buflen = strlen(buf);

			if (buflen <= 0)
				return;
			buf[buflen] = '\0';

			/* already word boundary? */
			if (!isspace((unsigned char)line[buflen]) &&
				line[buflen] != '\0')
			{
				/* try to divide at word boundary */
				i = buflen - 1;
				while (i > 0 && !isspace((unsigned char)buf[i]))
					i--;

				if (i > 0)		/* else couldn't divide word boundary */
				{
					buflen = i;
					buf[i] = '\0';
				}
			}

			chunk_nr++;

			syslog(level, "[%lu-%d] %s", seq, chunk_nr, buf);
			line += buflen;
			len -= buflen;
		}
	}
	else
	{
		/* message short enough */
		syslog(level, "[%lu] %s", seq, line);
	}
}

#endif   /* HAVE_SYSLOG */
