/*-------------------------------------------------------------------------
 * win32service.h
 *
 *  Windows service definitions	
 *
 *	Copyright (c) 2005, PostgreSQL Global Development Group
 *
 *  $Id: win32service.h,v 1.3 2005-08-30 18:24:04 darcyb Exp $
 *-------------------------------------------------------------------------
 */

void win32_servicestart(void);
void win32_eventlog(int level, char *msg);
extern int win32_isservice;
