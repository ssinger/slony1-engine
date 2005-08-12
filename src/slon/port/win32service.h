/*-------------------------------------------------------------------------
 * win32service.h
 *
 *  Windows service definitions	
 *
 *	Copyright (c) 2005, PostgreSQL Global Development Group
 *
 *  $Id: win32service.h,v 1.2 2005-08-12 12:45:03 dpage Exp $
 *-------------------------------------------------------------------------
 */

void win32_servicestart(void);
void win32_eventlog(int level, char *msg);
extern int win32_isservice;
