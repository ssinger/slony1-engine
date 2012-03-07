#ifndef _MISC_H_
#define _MISC_H_
/* ----------
 * Functions in misc.c
 * ----------
 */

typedef enum
{
	SLON_FATAL = -4,
	SLON_ERROR,
	SLON_WARN,
	SLON_CONFIG,
	SLON_INFO,
	SLON_DEBUG1,
	SLON_DEBUG2,
	SLON_DEBUG3,
	SLON_DEBUG4
}	Slon_Log_Level;

extern void slon_log(Slon_Log_Level level, char *fmt,...);

extern int	slon_scanint64(char *str, int64 *result);
#endif

#ifdef WIN32
/* Remove some defines that are imported from the postgresql headers, but
 * that refer to backend porting functions. */
#undef select
#undef accept
#undef connect
#undef socket
#undef recv
#undef send
#endif

/* Adjustment windows */
#ifdef WIN32
#define sleep(x) Sleep(x*1000)
#define strtoll(x,y,z) (__int64) strtol(x,y,z)
#define strncasecmp(x,y,z)	strnicmp(x,y,z)
#endif

/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
