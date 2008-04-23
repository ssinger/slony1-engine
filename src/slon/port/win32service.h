/*-------------------------------------------------------------------------
 * win32service.h
 *
 *	Windows service definitions
 *
 *	Copyright (c) 2005, PostgreSQL Global Development Group
 *
 *	$Id: win32service.h,v 1.5 2008-04-23 20:35:44 cbbrowne Exp $
 *-------------------------------------------------------------------------
 */

void		win32_servicestart(void);
void		win32_eventlog(int level, char *msg);
extern int	win32_isservice;
void		win32_serviceconfig(int argc, char *const argv[]);
