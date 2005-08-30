/*-------------------------------------------------------------------------
 * win32service.c
 *
 *  Windows service integration and eventlog 
 *
 *	Copyright (c) 2005, PostgreSQL Global Development Group
 *	Author: Magnus Hagander
 *
 *  $Id: win32service.c,v 1.3 2005-08-30 18:24:04 darcyb Exp $
 *-------------------------------------------------------------------------
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libpq-fe.h>
#include "slon.h"

/* Indicates if we are running as a service */
int win32_isservice = 0;

/* forward declarations */
static void WINAPI win32_servicemain(DWORD argc, LPTSTR * argv);
static void WINAPI win32_servicehandler(DWORD request);
static void win32_setservicestatus(DWORD state);
static bool win32_load_child_list(void);
static HANDLE win32_start_engine(int num);

/* Gobals for service control */
static SERVICE_STATUS status;
static SERVICE_STATUS_HANDLE hStatus;
static HANDLE shutdownEvent;

/* Not used in WIN32_OWN_PROCESS, but has to exist */
static char *servicename = "Slony";

/* Name of the service as the SCM sees it */
static char running_servicename[256];

/* Child engines */
static int childcount;
static char **children_config_files;


/* Start running as a service */
void win32_servicestart(void)
{
	SERVICE_TABLE_ENTRY st[] = {{servicename,win32_servicemain},{NULL,NULL}};

	win32_isservice = 1;

	if (StartServiceCtrlDispatcher(st) == 0)
	{
		fprintf(stderr,"could not start service control dispatcher: %lu\n", GetLastError());
		exit(1);
	}
}



/*
 * Entrypoint for actual service work.
 *
 * Fork of a normal slony process with specified commandline. Wait
 * on them to die (and restart) or the SCM to tell us to shut
 * down (and stop all engines).
 */
static void WINAPI win32_servicemain(DWORD argc, LPTSTR * argv)
{
	DWORD ret;
	HANDLE *waithandles = NULL;
	int i;

	/* fetch our actual service name */
	strcpy(running_servicename, argv[0]);

	/* initialize the status structure with static stuff */
	status.dwWin32ExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 30000;
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	status.dwServiceSpecificExitCode = 0;
	status.dwCurrentState = SERVICE_START_PENDING;

	/* register control request handler */
	hStatus = RegisterServiceCtrlHandler(servicename, win32_servicehandler);
	if (hStatus == 0)
	{
		slon_log(SLON_FATAL,"could not connect to service control handler: %lu", GetLastError());
		exit(1);
	}

	/* Tell SCM we are running before we make any API calls */	
	win32_setservicestatus(SERVICE_RUNNING);

	/* create event to handle shutdown */
	shutdownEvent = CreateEvent(NULL, true, false, NULL);
	if (shutdownEvent == NULL)
	{
		slon_log(SLON_FATAL,"could not create shutdown event: %lu", GetLastError());
		exit(1);
	}

	/* Report we're up and running */
	slon_log(SLON_INFO,"Slony service controller version %s started", SLONY_I_VERSION_STRING);

	/* Read our configuration from the registry to determine which 
	   enginesto start. Will do it's own error logging. */
	if (!win32_load_child_list())
		exit(1);

	/* Set up our array of handles to wait on. First handle is the 
	   shutdown handle, then one handle for each child */
	waithandles = malloc((childcount+1)*sizeof(HANDLE));
	if (!waithandles)
	{
		slon_log(SLON_FATAL, "win32_servicemain: out of memory");
		exit(1);
	}
	waithandles[0] = shutdownEvent;
	
	/* Start the required slony processes */
	for (i = 0; i < childcount; i++)
	{
		waithandles[i+1] = win32_start_engine(i);

		/* If start failed, set to shutdown event to 
		   prevent failure in wait code */
		if (waithandles[i+1] == INVALID_HANDLE_VALUE)
			waithandles[i+1] = shutdownEvent;
	}

	/* Stay in a loop until SCM shuts us down */ 
	while (true)
	{
		ret = WaitForMultipleObjectsEx(childcount+1, waithandles, FALSE, INFINITE, FALSE);
		if (ret == WAIT_FAILED)
		{
			slon_log(SLON_FATAL, "win32_servicemain: could not wait for child handles: %lu", GetLastError());
			exit(1);
		}
		if (ret == WAIT_OBJECT_0) /* shutdown */
		{
			win32_setservicestatus(SERVICE_STOP_PENDING);
			/* Shut down all Slon processes */
			slon_log(SLON_INFO,"received shutdown event, terminating all engines");
			for (i = 0; i < childcount; i++)
			{
				if (waithandles[i+1] != shutdownEvent &&
					waithandles[i+1] != INVALID_HANDLE_VALUE)
				{
					TerminateProcess(waithandles[i+1],0);
					CloseHandle(waithandles[i+1]);
				}
			}
			win32_setservicestatus(SERVICE_STOPPED);
			break;
		}
		else if (ret > WAIT_OBJECT_0 &&
				ret <= WAIT_OBJECT_0 + childcount)
		{
			/* a child process died! */
			int ofs = ret-WAIT_OBJECT_0-1;
				
			slon_log(SLON_WARN,"engine for '%s' terminated, restarting.", children_config_files[ofs]);
			CloseHandle(waithandles[ofs+1]);
			waithandles[ofs+1] = win32_start_engine(ofs);
			if (waithandles[ofs+1] == INVALID_HANDLE_VALUE)
				waithandles[ofs+1] = shutdownEvent;	
		}
		/* else just ignore what happened */
	}
}

/* Set the current service status */
static void win32_setservicestatus(DWORD state)
{
	status.dwCurrentState = state;
	SetServiceStatus(hStatus, (LPSERVICE_STATUS) &status);
}

/*
 * Handle any events sent by the service control manager
 * NOTE! Events are sent on a different thread! And it's
 * not a pthreads thread, so avoid calling anything that
 * may use pthreads - like slon_log() 
 */
static void WINAPI win32_servicehandler(DWORD request)
{
	switch (request)
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			win32_setservicestatus(SERVICE_STOP_PENDING);
			SetEvent(shutdownEvent);	
			return;
		default:
			break;
	}
}

/*
 * Load the list of slon engines to start
 */
static bool win32_load_child_list(void)
{
	char rootkey[1024];
	HKEY key;
	char valname[256];
	char valval[256];
	DWORD valnamesize = sizeof(valname);
	DWORD valvalsize = sizeof(valval);
	DWORD regtype;
	int r;

	sprintf(rootkey,"SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters\\Engines", running_servicename);

	if ((r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, rootkey, 0, KEY_READ, &key)) != ERROR_SUCCESS)
	{
		slon_log(SLON_FATAL, "Failed to open registry key '%s': %lu", rootkey, r);
		return false;
	}
	childcount = 0;
	while ((r=RegEnumValue(key, childcount, valname, &valnamesize, NULL, &regtype, NULL, NULL)) == ERROR_SUCCESS)
	{
		if (regtype != REG_SZ)
		{
			slon_log(SLON_FATAL, "Bad data type in registry key '%s', value '%s': %i", rootkey, valname, regtype);
			RegCloseKey(key);
			return false;
		}

		valnamesize = sizeof(valname);
		childcount++;
	}
	if (r != ERROR_NO_MORE_ITEMS)
	{
		slon_log(SLON_FATAL, "Failed to enumerate registry key '%s': %lu", rootkey, r);
		RegCloseKey(key);
		return false;
	}

	children_config_files = malloc(childcount * sizeof(char *));
	if (!children_config_files)
	{
		slon_log(SLON_FATAL, "Out of memory.");
		RegCloseKey(key);
		return false;
	}

	childcount = 0;
	valnamesize = sizeof(valname);
	valvalsize = sizeof(valval);
	while ((r=RegEnumValue(key, childcount, valname, &valnamesize, NULL, &regtype, valval, &valvalsize)) == ERROR_SUCCESS)
	{
		children_config_files[childcount] = strdup(valval);
		if (!children_config_files[childcount]) 
		{
			slon_log(SLON_FATAL, "Out of memory.");
			RegCloseKey(key);
			return false;
		}
		
		childcount++;
		valnamesize = sizeof(valname);
		valvalsize = sizeof(valval);
	}
	if (r != ERROR_NO_MORE_ITEMS)
	{
		slon_log(SLON_FATAL, "Failed to enumerate registry key '%s' a second time: %lu", rootkey, r);
		RegCloseKey(key);
		return false;
	}
	RegCloseKey(key);
	return true;
		
}

/* Start engine with config file at offset num in children_config_files */
static HANDLE win32_start_engine(int num)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char cmdline[512];
	static char self_process[512] = {0};
	int r;
	
	ZeroMemory(&si,sizeof(si));
	ZeroMemory(&pi,sizeof(pi));
	si.cb = sizeof(si);

	if (!self_process[0])
	{
		if (!GetModuleFileName(NULL, self_process, sizeof(self_process)))
		{
			slon_log(SLON_FATAL,"Failed to determine own filename: %lu",GetLastError());
			return INVALID_HANDLE_VALUE;
		}
	}
	wsprintf(cmdline,"\"%s\" -subservice -f \"%s\"", self_process, children_config_files[num]);

	if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		slon_log(SLON_ERROR,"Failed to spawn process for engine at '%s': %lu", children_config_files[num], GetLastError());
		return INVALID_HANDLE_VALUE;
	}
	CloseHandle(pi.hThread);

	/* Give the process five seconds to start up, and see if 
	   it's still around. If not, we call it dead on startup. */
	r = WaitForSingleObject(pi.hProcess, 5000);
	if (r == WAIT_TIMEOUT)
		/* nothing happened, so things seem ok */
		return pi.hProcess;
	else if (r == WAIT_OBJECT_0)
	{
		/* process died within one second */
		slon_log(SLON_ERROR,"Process for engine at '%s' died on startup.", children_config_files[num]);
		CloseHandle(pi.hProcess);
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		slon_log(SLON_FATAL,"Failed to wait for newly started process: %i", GetLastError());
		TerminateProcess(pi.hProcess, 250);
		CloseHandle(pi.hProcess);
		return INVALID_HANDLE_VALUE;
	}
}


/* Write a log entry to the eventlog */
void win32_eventlog(int level, char *msg)
{
	int elevel;
	static HANDLE evtHandle = INVALID_HANDLE_VALUE;
	
	switch (level)
	{
		case SLON_FATAL:
		case SLON_ERROR:
			elevel = EVENTLOG_ERROR_TYPE;
			break;	
		case SLON_WARN:
			elevel = EVENTLOG_WARNING_TYPE;
		default:
			elevel = EVENTLOG_INFORMATION_TYPE;
	}
	
	if (evtHandle == INVALID_HANDLE_VALUE)
	{
		evtHandle = RegisterEventSource(NULL, "Slony-I");
		if (evtHandle==NULL)
		{
			evtHandle = INVALID_HANDLE_VALUE;
			return;
		}
	}
	ReportEvent(evtHandle, elevel, 0, 0, NULL, 1, 0, (const char **)&msg, NULL);
}
