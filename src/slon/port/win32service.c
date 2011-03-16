/*-------------------------------------------------------------------------
 * win32service.c
 *
 *	Windows service integration and eventlog
 *
 *	Copyright (c) 2005-2009, PostgreSQL Global Development Group
 *	Author: Magnus Hagander
 *
 *	
 *-------------------------------------------------------------------------
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libpq-fe.h>
#include<stdarg.h>
#include "slon.h"

/* Indicates if we are running as a service */
int			win32_isservice = 0;

/* forward declarations */
static void WINAPI win32_servicemain(DWORD argc, LPTSTR * argv);
static void WINAPI win32_servicehandler(DWORD request);
static void win32_setservicestatus(DWORD state);
static bool win32_load_child_list(void);
static HANDLE win32_start_engine(int num);

static void RegisterService(char *servicename);
static void UnRegisterService(char *servicename);
static void ListEngines(char *servicename);
static void AddEngine(char *servicename, char *configfile);
static void DelEngine(char *servicename, char *configfile);


/* Gobals for service control */
static SERVICE_STATUS status;
static SERVICE_STATUS_HANDLE hStatus;
static HANDLE shutdownEvent;

/* Not used in WIN32_OWN_PROCESS, but has to exist */
static char *servicename = "Slony";

/* Name of the service as the SCM sees it */
static char running_servicename[256];

/* Child engines */
static int	childcount;
static char **children_config_files;


/* Start running as a service */
void
win32_servicestart(void)
{
	SERVICE_TABLE_ENTRY st[] = {{servicename, win32_servicemain}, {NULL, NULL}};

	win32_isservice = 1;

	if (StartServiceCtrlDispatcher(st) == 0)
	{
		fprintf(stderr, "could not start service control dispatcher: %lu\n", GetLastError());
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
static void WINAPI
win32_servicemain(DWORD argc, LPTSTR * argv)
{
	DWORD		ret;
	HANDLE	   *waithandles = NULL;
	int			i;
	int			startcount;

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
		slon_log(SLON_FATAL, "could not connect to service control handler: %lu", GetLastError());
		exit(1);
	}

	/* Tell SCM we are running before we make any API calls */
	win32_setservicestatus(SERVICE_RUNNING);

	/* create event to handle shutdown */
	shutdownEvent = CreateEvent(NULL, true, false, NULL);
	if (shutdownEvent == NULL)
	{
		slon_log(SLON_FATAL, "could not create shutdown event: %lu", GetLastError());
		exit(1);
	}

	/* Report we're up and running */
	slon_log(SLON_INFO, "Slony service controller version %s started", SLONY_I_VERSION_STRING);

	/*
	 * Read our configuration from the registry to determine which enginesto
	 * start. Will do it's own error logging.
	 */
	if (!win32_load_child_list())
		exit(1);

	/*
	 * Set up our array of handles to wait on. First handle is the shutdown
	 * handle, then one handle for each child
	 */
	waithandles = malloc((childcount + 1) * sizeof(HANDLE));
	if (!waithandles)
	{
		slon_log(SLON_FATAL, "win32_servicemain: out of memory");
		exit(1);
	}
	waithandles[0] = shutdownEvent;

	/* Start the required slony processes */
	startcount = 0;
	for (i = 0; i < childcount; i++)
	{
		waithandles[i + 1] = win32_start_engine(i);

		/*
		 * If start failed, set to shutdown event to prevent failure in wait
		 * code
		 */
		if (waithandles[i + 1] == INVALID_HANDLE_VALUE)
			waithandles[i + 1] = shutdownEvent;
		else
			startcount++;
	}
	slon_log(SLON_INFO, "started %i slon engine(s)", startcount);

	/* Stay in a loop until SCM shuts us down */
	while (true)
	{
		ret = WaitForMultipleObjectsEx(childcount + 1, waithandles, FALSE, INFINITE, FALSE);
		if (ret == WAIT_FAILED)
		{
			slon_log(SLON_FATAL, "win32_servicemain: could not wait for child handles: %lu", GetLastError());
			exit(1);
		}
		if (ret == WAIT_OBJECT_0)		/* shutdown */
		{
			win32_setservicestatus(SERVICE_STOP_PENDING);
			/* Shut down all Slon processes */
			slon_log(SLON_INFO, "received shutdown event, terminating all engines");
			for (i = 0; i < childcount; i++)
			{
				if (waithandles[i + 1] != shutdownEvent &&
					waithandles[i + 1] != INVALID_HANDLE_VALUE)
				{
					TerminateProcess(waithandles[i + 1], 0);
					CloseHandle(waithandles[i + 1]);
				}
			}
			win32_setservicestatus(SERVICE_STOPPED);
			break;
		}
		else if (ret > WAIT_OBJECT_0 &&
				 ret <= WAIT_OBJECT_0 + childcount)
		{
			/* a child process died! */
			int			ofs = ret - WAIT_OBJECT_0 - 1;

			slon_log(SLON_WARN, "engine for '%s' terminated, restarting.", children_config_files[ofs]);
			CloseHandle(waithandles[ofs + 1]);
			waithandles[ofs + 1] = win32_start_engine(ofs);
			if (waithandles[ofs + 1] == INVALID_HANDLE_VALUE)
				waithandles[ofs + 1] = shutdownEvent;
		}
		/* else just ignore what happened */
	}
}

/* Set the current service status */
static void
win32_setservicestatus(DWORD state)
{
	status.dwCurrentState = state;
	SetServiceStatus(hStatus, (LPSERVICE_STATUS) & status);
}

/*
 * Handle any events sent by the service control manager
 * NOTE! Events are sent on a different thread! And it's
 * not a pthreads thread, so avoid calling anything that
 * may use pthreads - like slon_log()
 */
static void WINAPI
win32_servicehandler(DWORD request)
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
 * Open the <servicename>\Parameters\Engines registry key, which holds the list
 * of all the engines associated with this service.
 */
static HKEY
OpenEnginesKey(DWORD access)
{
	char		rootkey[1024];
	HKEY		key;
	int			r;

	sprintf(rootkey, "SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters\\Engines", running_servicename);

	if ((r = RegCreateKeyEx(HKEY_LOCAL_MACHINE, rootkey, 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL, &key, NULL)) != ERROR_SUCCESS)
	{
		slon_log(SLON_FATAL, "Failed to open registry key '%s': %lu", rootkey, r);
		return NULL;
	}
	return key;
}

/*
 * Load the list of slon engines to start
 */
static bool
win32_load_child_list(void)
{
	char		rootkey[1024];
	HKEY		key;
	char		valname[256];
	char		valval[256];
	DWORD		valnamesize = sizeof(valname);
	DWORD		valvalsize = sizeof(valval);
	DWORD		regtype;
	int			r;

	key = OpenEnginesKey(KEY_READ);
	if (!key)
		return false;

	childcount = 0;
	while ((r = RegEnumValue(key, childcount, valname, &valnamesize, NULL, &regtype, NULL, NULL)) == ERROR_SUCCESS)
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
	while ((r = RegEnumValue(key, childcount, valname, &valnamesize, NULL, &regtype, valval, &valvalsize)) == ERROR_SUCCESS)
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
static HANDLE
win32_start_engine(int num)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char		cmdline[512];
	static char self_process[512] = {0};
	int			r;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	if (!self_process[0])
	{
		if (!GetModuleFileName(NULL, self_process, sizeof(self_process)))
		{
			slon_log(SLON_FATAL, "Failed to determine own filename: %lu", GetLastError());
			return INVALID_HANDLE_VALUE;
		}
	}
	wsprintf(cmdline, "\"%s\" -subservice -f \"%s\"", self_process, children_config_files[num]);

	if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		slon_log(SLON_ERROR, "Failed to spawn process for engine at '%s': %lu", children_config_files[num], GetLastError());
		return INVALID_HANDLE_VALUE;
	}
	CloseHandle(pi.hThread);

	/*
	 * Give the process five seconds to start up, and see if it's still
	 * around. If not, we call it dead on startup.
	 */
	r = WaitForSingleObject(pi.hProcess, 5000);
	if (r == WAIT_TIMEOUT)
		/* nothing happened, so things seem ok */
		return pi.hProcess;
	else if (r == WAIT_OBJECT_0)
	{
		/* process died within one second */
		slon_log(SLON_ERROR, "Process for engine at '%s' died on startup.", children_config_files[num]);
		CloseHandle(pi.hProcess);
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		slon_log(SLON_FATAL, "Failed to wait for newly started process: %i", GetLastError());
		TerminateProcess(pi.hProcess, 250);
		CloseHandle(pi.hProcess);
		return INVALID_HANDLE_VALUE;
	}
	slon_log(SLON_INFO, "Started Slon engine for '%s' with pid %i", children_config_files[num], pi.dwProcessId);
}


/* Write a log entry to the eventlog */
void
win32_eventlog(int level, char *msg)
{
	int			elevel;
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
		evtHandle = RegisterEventSource(NULL, "Slony");
		if (evtHandle == NULL)
		{
			evtHandle = INVALID_HANDLE_VALUE;
			return;
		}
	}
	ReportEvent(evtHandle, elevel, 0, 0, NULL, 1, 0, (const char **) &msg, NULL);
}



/* Deal with service and engine registration and unregistration */
void
win32_serviceconfig(int argc, char *argv[])
{
	if (!strcmp(argv[1], "-regservice"))
	{
		if (argc != 2 && argc != 3)
			Usage(argv);
		RegisterService((argc == 3) ? argv[2] : "Slony-I");
	}
	else if (!strcmp(argv[1], "-unregservice"))
	{
		if (argc != 2 && argc != 3)
			Usage(argv);
		UnRegisterService((argc == 3) ? argv[2] : "Slony-I");
	}
	else if (!strcmp(argv[1], "-listengines"))
	{
		if (argc != 2 && argc != 3)
			Usage(argv);
		ListEngines((argc == 3) ? argv[2] : "Slony-I");
	}
	else if (!strcmp(argv[1], "-addengine"))
	{
		if (argc != 3 && argc != 4)
			Usage(argv);
		AddEngine((argc == 4) ? argv[2] : "Slony-I", (argc == 4) ? argv[3] : argv[2]);
	}
	else if (!strcmp(argv[1], "-delengine"))
	{
		if (argc != 3 && argc != 4)
			Usage(argv);
		DelEngine((argc == 4) ? argv[2] : "Slony-I", (argc == 4) ? argv[3] : argv[2]);
	}
	else
		Usage(argv);
	exit(0);
}

/* Check windows version against Server 2003 to determine service functionality */
static bool
is_windows2003ornewer()
{
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);

	if (!GetVersionEx(&vi))
	{
		fprintf(stderr, "Failed to determine OS version: %lu\n", GetLastError());
		exit(1);
	}
	if (vi.dwMajorVersion > 5)
		return true;			/* Vista + */
	if (vi.dwMajorVersion == 5 && vi.dwMinorVersion >= 2)
		return true;			/* Win 2003 */
	return false;
}

/* Open Service Control Manager */
static SC_HANDLE
openSCM()
{
	SC_HANDLE	manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!manager)
	{
		fprintf(stderr, "Failed to open service control manager: %lu\n", GetLastError());
		exit(1);
	}
	return manager;
}

/* Register a service with the specified name with the local service control manager */
static void
RegisterService(char *servicename)
{
	char		self[1024];
	char		execpath[1200];
	SC_HANDLE	manager;
	SC_HANDLE	service;
	char	   *account = is_windows2003ornewer() ? "NT AUTHORITY\\Local Service" : NULL;

	ZeroMemory(self, sizeof(self));

	if (!GetModuleFileName(NULL, self, sizeof(self)))
	{
		fprintf(stderr, "Failed to determine path name: %lu\n", GetLastError());
		exit(1);
	}
	wsprintf(execpath, "%s -service", self);

	manager = openSCM();

	service = CreateService(
							manager,
							servicename,
							servicename,
							SERVICE_ALL_ACCESS,
							SERVICE_WIN32_OWN_PROCESS,
							SERVICE_AUTO_START,
							SERVICE_ERROR_NORMAL,
							execpath,
							NULL,
							NULL,
							"RPCSS\0",
							account,
							"");
	if (!service)
	{
		fprintf(stderr, "Failed to create service: %lu\n", GetLastError());
		exit(1);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	printf("Service registered.\n");
	printf("Before you can run Slony, you must also register an engine!\n\n");
	if (account == NULL)
	{
		printf("WARNING! Service is registered to run as Local System. You are\n");
		printf("encouraged to change this to a low privilege account to increase\n");
		printf("system security.\n");
	}
}

/* Remove a service with the specified name from the local service control manager */
static void
UnRegisterService(char *servicename)
{
	SC_HANDLE	manager;
	SC_HANDLE	service;

	manager = openSCM();

	service = OpenService(manager, servicename, SC_MANAGER_ALL_ACCESS);
	if (!service)
	{
		fprintf(stderr, "Failed to open service: %lu\n", GetLastError());
		exit(1);
	}

	if (!DeleteService(service))
	{
		fprintf(stderr, "Failed to delete service: %lu\n", GetLastError());
		exit(1);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	printf("Service removed.\n");
}

/* Print a list of all engines associated with the specified service */
static void
ListEngines(char *servicename)
{
	int			i;

	strcpy(running_servicename, servicename);
	if (!win32_load_child_list())
		exit(1);

	printf("\n%i engine(s) registered for service '%s'\n", childcount, servicename);
	for (i = 0; i < childcount; i++)
	{
		printf("Engine %i: %s\n", i + 1, children_config_files[i]);
	}
}

/*
 * Verify that a file exists, and also expand the filename to
 * an absolute path.
 */
#define MAXPGPATH 256
static char _vfe_buf[MAXPGPATH];
static char *
VerifyFileExists(char *filename)
{
	DWORD		r;

	ZeroMemory(_vfe_buf, sizeof(_vfe_buf));
	r = GetFullPathName(filename, sizeof(_vfe_buf), _vfe_buf, NULL);
	if (r == 0 || r > sizeof(_vfe_buf))
	{
		fprintf(stderr, "Failed to get full pathname for '%s': %i\n", filename, GetLastError());
		exit(1);
	}

	if (GetFileAttributes(_vfe_buf) == 0xFFFFFFFF)
	{
		fprintf(stderr, "File '%s' could not be opened: %i\n", _vfe_buf, GetLastError());
		exit(1);
	}

	return _vfe_buf;
}

/* Register a new engine with a specific config file with the specified service */
static void
AddEngine(char *servicename, char *configfile)
{
	HKEY		key;
	int			r;

	char	   *full_configfile = VerifyFileExists(configfile);

	strcpy(running_servicename, servicename);
	key = OpenEnginesKey(KEY_ALL_ACCESS);
	if (!key)
		exit(1);

	r = RegQueryValueEx(key, full_configfile, 0, NULL, NULL, NULL);
	if (r == 0)
	{
		fprintf(stderr, "Engine '%s' already registered for service '%s'.\n", full_configfile, servicename);
		exit(1);
	}

	r = RegSetValueEx(key, full_configfile, 0, REG_SZ, full_configfile, strlen(full_configfile) + 1);
	RegCloseKey(key);
	if (r == ERROR_SUCCESS)
	{
		printf("Engine added.\n");
		printf("NOTE! You need to restart the Slony service before this takes effect.\n");
		return;
	}
	else
		fprintf(stderr, "Failed to register engine: %lu.\n", r);
	exit(1);
}

/* Remove an engine registration from the specified service */
static void
DelEngine(char *servicename, char *configfile)
{
	HKEY		key;
	int			r;

	strcpy(running_servicename, servicename);
	key = OpenEnginesKey(KEY_ALL_ACCESS);
	if (!key)
		exit(1);

	r = RegDeleteValue(key, configfile);
	RegCloseKey(key);
	if (r == ERROR_SUCCESS)
	{
		printf("Engine removed.\n");
		printf("NOTE! You need to restart the Slony service before this takes effect.\n");
		return;
	}
	else if (r == 2)
	{
		fprintf(stderr, "Engine '%s' not registered for service '%s'.\n", configfile, servicename);
	}
	else
		fprintf(stderr, "Failed to unregister engine: %lu\n", r);
	exit(1);
}

