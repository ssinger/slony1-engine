#ifndef _MISC_H_
#define _MISC_H_
/* ----------
 * Functions in misc.c
 * ----------
 */
#include "config.h"
#include "c.h"

typedef enum {
        SLON_FATAL,
        SLON_ERROR,
        SLON_WARN,
        SLON_CONFIG,
        SLON_INFO,
        SLON_DEBUG1,
        SLON_DEBUG2,
        SLON_DEBUG3,
        SLON_DEBUG4
} SlonLogLevel;

extern void             slon_log(SlonLogLevel level, char * fmt, ...);

extern int              slon_scanint64(char *str, int64 *result);

#endif
