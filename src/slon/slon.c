/*-------------------------------------------------------------------------
 * slon.c
 *
 *	The control framework for the node daemon.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <sys/types.h>



#ifdef WIN32
#include <winsock2.h>
#include "port/win32service.h"
#define sleep(x) Sleep(x*1000)
#endif

#include "libpq-fe.h"

#include "slon.h"


#include "confoptions.h"



/* ----------
 * Global data
 * ----------
 */
#ifndef WIN32
#define		SLON_WATCHDOG_NORMAL		0
#define		SLON_WATCHDOG_RESTART		1
#define		SLON_WATCHDOG_RETRY			2
#define		SLON_WATCHDOG_SHUTDOWN		3
static int	watchdog_status = SLON_WATCHDOG_NORMAL;
#endif
int			sched_wakeuppipe[2];

pthread_mutex_t slon_wait_listen_lock;
pthread_cond_t slon_wait_listen_cond;
int			slon_listen_started = 0;
bool		monitor_threads;

int			apply_cache_size;

/* ----------
 * Local data
 * ----------
 */
static void slon_exit(int code);
static pthread_t local_event_thread;
static pthread_t local_cleanup_thread;
static pthread_t local_sync_thread;
static pthread_t local_monitor_thread;

static pthread_t main_thread;
static char *const * main_argv;

static void SlonMain(void);

#ifndef WIN32
static void SlonWatchdog(void);
static void sighandler(int signo);
void		slon_terminate_worker(void);
#endif
typedef void (*sighandler_t) (int);
static sighandler_t install_signal_handler(int signum, sighandler_t handler);

int			slon_log_level;
char	   *pid_file;
char	   *archive_dir = NULL;
static int	child_status;

/**
 * A variable to indicate that the
 * worker has been restarted by the watchdog.
 */
int			worker_restarted = 0;

/* ----------
 * Usage
 * ----------
 */
void
Usage(char *const argv[])
{
	fprintf(stderr, "usage: %s [options] clustername conninfo\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");

	fprintf(stderr, "    -h                    print usage message and exit\n");
	fprintf(stderr, "    -v                    print version and exit\n");
	fprintf(stderr, "    -d <debuglevel>       verbosity of logging (1..4)\n");
	fprintf(stderr, "    -s <milliseconds>     SYNC check interval (default 10000)\n");
	fprintf(stderr, "    -t <milliseconds>     SYNC interval timeout (default 60000)\n");
	fprintf(stderr, "    -o <milliseconds>     desired subscriber SYNC processing time\n");
	fprintf(stderr, "    -g <num>              maximum SYNC group size (default 6)\n");
	fprintf(stderr, "    -c <num>              how often to vacuum in cleanup cycles\n");
	fprintf(stderr, "    -p <filename>         slon pid file\n");
	fprintf(stderr, "    -f <filename>         slon configuration file\n");
	fprintf(stderr, "    -a <directory>        directory to store SYNC archive files\n");
	fprintf(stderr, "    -x <command>          program to run after writing archive file\n");
	fprintf(stderr, "    -q <num>              Terminate when this node reaches # of SYNCs\n");
	fprintf(stderr, "    -r <num>              # of syncs for -q option\n");
	fprintf(stderr, "    -l <interval>         this node should lag providers by this interval\n");
#ifdef WIN32
	fprintf(stderr, "\nWindows service registration:\n");
	fprintf(stderr, " slon -regservice [servicename]\n");
	fprintf(stderr, " slon -unregservice [servicename]\n");
	fprintf(stderr, " slon -listengines [servicename]\n");
	fprintf(stderr, " slon -addengine [servicename] <configfile>\n");
	fprintf(stderr, " slon -delengine [servicename] <configfile>\n");
#endif
	exit(1);
}


/* ----------
 * main
 * ----------
 */
int
main(int argc, char *const argv[])
{
	char	   *cp1;
	char	   *cp2;
	int			c;
	int			errors = 0;
	extern int	optind;
	extern char *optarg;


#ifdef WIN32
	WSADATA		wsaData;
	int			err;
#endif


#ifdef WIN32
	if (argc >= 2 && !strcmp(argv[1], "-service"))
	{
		win32_servicestart();
		exit(0);
	}
	if (argc >= 2 && !strcmp(argv[1], "-subservice"))
	{
		win32_isservice = 1;
		argc--;
		argv++;
	}
	if (argc >= 2 && argc <= 4 && (
								   !strcmp(argv[1], "-regservice") ||
								   !strcmp(argv[1], "-unregservice") ||
								   !strcmp(argv[1], "-addengine") ||
								   !strcmp(argv[1], "-delengine") ||
								   !strcmp(argv[1], "-listengines")))
	{
		win32_serviceconfig(argc, argv);
	}
#endif

	InitializeConfOptions();

	while ((c = getopt(argc, argv, "f:a:d:s:t:g:c:p:o:q:r:l:x:hv?")) != EOF)
	{
		switch (c)
		{
			case '?':
				Usage(argv);
			case 'q':
				set_config_option("quit_sync_provider", optarg);
				break;

			case 'r':
				set_config_option("quit_sync_finalsync", optarg);
				break;

			case 'f':
				ProcessConfigFile(optarg);
				break;

			case 'a':
				set_config_option("archive_dir", optarg);
				break;

			case 'd':
				set_config_option("log_level", optarg);
				break;

			case 's':
				set_config_option("sync_interval", optarg);
				break;

			case 't':
				set_config_option("sync_interval_timeout", optarg);
				break;

			case 'g':
				set_config_option("sync_group_maxsize", optarg);
				break;

			case 'c':
				set_config_option("vac_frequency", optarg);
				break;

			case 'p':
				set_config_option("pid_file", optarg);
				break;

			case 'o':
				set_config_option("desired_sync_time", optarg);
				break;

			case 'l':
				set_config_option("lag_interval", optarg);
				break;

			case 'h':
				errors++;
				break;

			case 'v':
				printf("slon version %s\n", SLONY_I_VERSION_STRING);
				exit(0);
				break;

			case 'x':
				set_config_option("command_on_logarchive", optarg);
				break;

			default:
				fprintf(stderr, "unknown option '%c'\n", c);
				errors++;
				break;
		}
	}

	/*
	 * Make sure the sync interval timeout isn't too small.
	 */
	if (sync_interval_timeout != 0 && sync_interval_timeout <= sync_interval)
		sync_interval_timeout = sync_interval * 2;

	/*
	 * Remember the cluster name and build the properly quoted namespace
	 * identifier
	 */
	slon_pid = getpid();
#ifndef WIN32
	if (pthread_mutex_init(&slon_watchdog_lock, NULL) < 0)
	{
		slon_log(SLON_FATAL, "slon: pthread_mutex_init() - %s\n",
				 strerror(errno));
		exit(-1);
	}
	slon_watchdog_pid = slon_pid;
	slon_worker_pid = -1;
#endif
	main_argv = argv;

	if ((char *) argv[optind])
	{
		set_config_option("cluster_name", (char *) argv[optind]);
		set_config_option("conn_info", (char *) argv[++optind]);
	}

	if (rtcfg_cluster_name != NULL)
	{
		rtcfg_namespace = malloc(strlen(rtcfg_cluster_name) * 2 + 4);
		cp2 = rtcfg_namespace;
		*cp2++ = '"';
		*cp2++ = '_';
		for (cp1 = (char *) rtcfg_cluster_name; *cp1; cp1++)
		{
			if (*cp1 == '"')
				*cp2++ = '"';
			*cp2++ = *cp1;
		}
		*cp2++ = '"';
		*cp2 = '\0';
	}
	else
	{
		errors++;
	}

	slon_log(SLON_CONFIG, "main: slon version %s starting up\n",
			 SLONY_I_VERSION_STRING);

	/*
	 * Remember the connection information for the local node.
	 */
	if (rtcfg_conninfo == NULL)
	{
		errors++;
	}

	if (errors != 0)
	{
		Usage(argv);
	}

#ifdef WIN32

	/*
	 * Startup the network subsystem, in case our libpq doesn't
	 */
	err = WSAStartup(MAKEWORD(1, 1), &wsaData);
	if (err != 0)
	{
		slon_log(SLON_FATAL, "main: Cannot start the network subsystem - %d\n", err);
		exit(-1);
	}
#endif

	if (pid_file)
	{
		FILE	   *pidfile;

		pidfile = fopen(pid_file, "w");
		if (pidfile)
		{
			fprintf(pidfile, "%d\n", slon_pid);
			fclose(pidfile);
		}
		else
		{
			slon_log(SLON_FATAL, "Cannot open pid_file \"%s\"\n", pid_file);
			exit(-1);
		}
	}

	/*
	 * Create the pipe used to kick the workers scheduler thread
	 */
	if (pgpipe(sched_wakeuppipe) < 0)
	{
		slon_log(SLON_FATAL, "slon: sched_wakeuppipe create failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}

	if (!PQisthreadsafe())
	{
		slon_log(SLON_FATAL, "slon: libpq was not compiled with thread safety enabled (normally: --enable-thread-safety).  slon is a multithreaded application requiring thread-safe libpq\n");
		slon_exit(-1);
	}

	if (!PQisthreadsafe())
	{
		slon_log(SLON_FATAL, "slon: libpq was not compiled with --enable-thread-safety. Slony-I requires a thread enabled libpq\n");
		slon_exit(-1);
	}

	/*
	 * There is no watchdog process on win32. We delegate restarting and other
	 * such tasks to the Service Control Manager. And win32 doesn't support
	 * signals, so we don't need to catch them...
	 */
#ifndef WIN32
	SlonWatchdog();
#else
	SlonMain();
#endif
	exit(0);
}


/* ----------
 * SlonMain
 * ----------
 */
static void
SlonMain(void)
{
	PGresult   *res;
	SlonDString query;
	int			i,
				n;
	PGconn	   *startup_conn;

	slon_pid = getpid();
#ifndef WIN32
	slon_worker_pid = slon_pid;
#endif

	if (pthread_mutex_init(&slon_wait_listen_lock, NULL) < 0)
	{
		slon_log(SLON_FATAL, "main: pthread_mutex_init() failed - %s\n",
				 strerror(errno));
		slon_abort();
	}
	if (pthread_cond_init(&slon_wait_listen_cond, NULL) < 0)
	{
		slon_log(SLON_FATAL, "main: pthread_cond_init() failed - %s\n",
				 strerror(errno));
		slon_abort();
	}


	/*
	 * Dump out current configuration - all elements of the various arrays...
	 */
	dump_configuration();

	/*
	 * Connect to the local database to read the initial configuration
	 */
	startup_conn = PQconnectdb(rtcfg_conninfo);
	if (startup_conn == NULL)
	{
		slon_log(SLON_FATAL, "main: PQconnectdb() failed - sleep 10s\n");
		sleep(10);
		slon_retry();
		exit(-1);
	}
	if (PQstatus(startup_conn) != CONNECTION_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot connect to local database - %s - sleep 10s\n",
				 PQerrorMessage(startup_conn));
		PQfinish(startup_conn);
		sleep(10);
		slon_retry();
		exit(-1);
	}

	/*
	 * Get our local node ID
	 */
	rtcfg_nodeid = db_getLocalNodeId(startup_conn);
	if (rtcfg_nodeid < 0)
	{
		slon_log(SLON_FATAL, "main: Node is not initialized properly - sleep 10s\n");
		sleep(10);
		slon_retry();
		exit(-1);
	}
	if (db_checkSchemaVersion(startup_conn) < 0)
	{
		slon_log(SLON_FATAL, "main: Node has wrong Slony-I schema or module version loaded\n");
		slon_abort();
	}
	slon_log(SLON_CONFIG, "main: local node id = %d\n", rtcfg_nodeid);

	dstring_init(&query);
	slon_mkquery(&query, "select %s.slon_node_health_check();", rtcfg_namespace);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "could not call slon_node_health_check() - %",
				 PQresultErrorMessage(res));
		slon_abort();
	}
	else
	{
		if (PQntuples(res) != 1)
		{
			slon_log(SLON_FATAL,
					 "query '%s' returned %d rows (expected 1)\n",
					 query, PQntuples(res));
			slon_abort();
		}
		else
		{
			if (*(PQgetvalue(res, 0, 0)) == 'f')
			{
				slon_log(SLON_FATAL,
						 "slon_node_health_check() returned false - fatal health problem!\n%s\nREPAIR CONFIG may be helpful to rectify this problem\n",
						 PQresultErrorMessage(res));
				slon_abort();
			}
		}
	}
	PQclear(res);
	dstring_free(&query);

#ifndef WIN32
	if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "main: SIGHUP signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_abort();
	}
	if (signal(SIGINT, SIG_IGN) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "main: SIGINT signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_abort();
	}
	if (signal(SIGTERM, SIG_IGN) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "main: SIGTERM signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_abort();
	}
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "main: SIGCHLD signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_abort();
	}
	if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "main: SIGQUIT signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_abort();
	}
#endif

	slon_log(SLON_INFO, "main: main process started\n");

	/*
	 * Start the event scheduling system
	 */
	slon_log(SLON_CONFIG, "main: launching sched_start_mainloop\n");
	if (sched_start_mainloop() < 0)
		slon_retry();

	slon_log(SLON_CONFIG, "main: loading current cluster configuration\n");

	/*
	 * Begin a transaction
	 */
	res = PQexec(startup_conn,
				 "start transaction; "
				 "set transaction isolation level serializable;");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		slon_log(SLON_FATAL, "Cannot start transaction - %s - sleep 10s\n",
				 PQresultErrorMessage(res));
		sleep(10);
		PQclear(res);
		slon_retry();
	}
	PQclear(res);

	/*
	 * Read configuration table sl_node
	 */
	dstring_init(&query);
	slon_mkquery(&query,
				 "select no_id, no_active, no_comment, "
				 "    (select coalesce(max(con_seqno),0) from %s.sl_confirm "
				 "        where con_origin = no_id and con_received = %d) "
				 "        as last_event, "
				 "    (select ev_snapshot from %s.sl_event "
				 "        where ev_origin = no_id "
				 "        and ev_seqno = (select max(ev_seqno) "
				 "                    from %s.sl_event "
				 "                    where ev_origin = no_id "
			   "                    and ev_type = 'SYNC')) as last_snapshot "
				 "from %s.sl_node "
				 "order by no_id; ",
				 rtcfg_namespace, rtcfg_nodeid,
				 rtcfg_namespace, rtcfg_namespace,
				 rtcfg_namespace);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot get node list - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int			no_id = (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int			no_active = (*PQgetvalue(res, i, 1) == 't') ? 1 : 0;
		char	   *no_comment = PQgetvalue(res, i, 2);
		int64		last_event;

		if (no_id == rtcfg_nodeid)
		{
			/*
			 * Complete our own local node entry
			 */
			rtcfg_nodeactive = no_active;
			rtcfg_nodecomment = strdup(no_comment);
		}
		else
		{
			/*
			 * Add a remote node
			 */
			slon_scanint64(PQgetvalue(res, i, 3), &last_event);
			rtcfg_storeNode(no_id, no_comment);
			rtcfg_setNodeLastEvent(no_id, last_event);
			rtcfg_setNodeLastSnapshot(no_id, PQgetvalue(res, i, 4));

			/*
			 * If it is active, remember for activation just before we start
			 * processing events.
			 */
			if (no_active)
				rtcfg_needActivate(no_id);
		}
	}
	PQclear(res);

	/*
	 * Read configuration table sl_path - the interesting pieces
	 */
	slon_mkquery(&query,
				 "select pa_server, pa_conninfo, pa_connretry "
				 "from %s.sl_path where pa_client = %d"
				 " and pa_conninfo<>'<event pending>'",
				 rtcfg_namespace, rtcfg_nodeid);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot get path config - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int			pa_server = (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		char	   *pa_conninfo = PQgetvalue(res, i, 1);
		int			pa_connretry = (int) strtol(PQgetvalue(res, i, 2), NULL, 10);

		rtcfg_storePath(pa_server, pa_conninfo, pa_connretry);
	}
	PQclear(res);

	/*
	 * Load the initial listen configuration
	 */
	rtcfg_reloadListen(startup_conn);

	/*
	 * Read configuration table sl_set
	 */
	slon_mkquery(&query,
				 "select set_id, set_origin, set_comment "
				 "from %s.sl_set",
				 rtcfg_namespace);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot get set config - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int			set_id = (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int			set_origin = (int) strtol(PQgetvalue(res, i, 1), NULL, 10);
		char	   *set_comment = PQgetvalue(res, i, 2);

		rtcfg_storeSet(set_id, set_origin, set_comment);
	}
	PQclear(res);

	/*
	 * Read configuration table sl_subscribe - only subscriptions for local
	 * node
	 */
	slon_mkquery(&query,
				 "select sub_set, sub_provider, sub_forward, sub_active "
				 "from %s.sl_subscribe "
				 "where sub_receiver = %d",
				 rtcfg_namespace, rtcfg_nodeid);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot get subscription config - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	for (i = 0, n = PQntuples(res); i < n; i++)
	{
		int			sub_set = (int) strtol(PQgetvalue(res, i, 0), NULL, 10);
		int			sub_provider = (int) strtol(PQgetvalue(res, i, 1), NULL, 10);
		char	   *sub_forward = PQgetvalue(res, i, 2);
		char	   *sub_active = PQgetvalue(res, i, 3);

		rtcfg_storeSubscribe(sub_set, sub_provider, sub_forward);
		if (*sub_active == 't')
			rtcfg_enableSubscription(sub_set, sub_provider, sub_forward);
	}
	PQclear(res);

	/*
	 * Remember the last known local event sequence
	 */
	slon_mkquery(&query,
				 "select coalesce(max(ev_seqno), -1) from %s.sl_event "
				 "where ev_origin = '%d'",
				 rtcfg_namespace, rtcfg_nodeid);
	res = PQexec(startup_conn, dstring_data(&query));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot get last local eventid - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		dstring_free(&query);
		slon_retry();
	}
	if (PQntuples(res) == 0)
		strcpy(rtcfg_lastevent, "-1");
	else if (PQgetisnull(res, 0, 0))
		strcpy(rtcfg_lastevent, "-1");
	else
		strcpy(rtcfg_lastevent, PQgetvalue(res, 0, 0));
	PQclear(res);
	dstring_free(&query);
	slon_log(SLON_CONFIG,
			 "main: last local event sequence = %s\n",
			 rtcfg_lastevent);

	/*
	 * Rollback the transaction we used to get the config snapshot
	 */
	res = PQexec(startup_conn, "rollback transaction;");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		slon_log(SLON_FATAL, "main: Cannot rollback transaction - %s\n",
				 PQresultErrorMessage(res));
		PQclear(res);
		slon_retry();
	}
	PQclear(res);

	/*
	 * Done with the startup, don't need the local connection any more.
	 */
	PQfinish(startup_conn);

	slon_log(SLON_CONFIG, "main: configuration complete - starting threads\n");

	/*
	 * Create the local event thread that monitors the local node for
	 * administrative events to adjust the configuration at runtime. We wait
	 * here until the local listen thread has checked that there is no other
	 * slon daemon running.
	 */
	pthread_mutex_lock(&slon_wait_listen_lock);
	if (pthread_create(&local_event_thread, NULL, localListenThread_main, NULL) < 0)
	{
		slon_log(SLON_FATAL, "main: cannot create localListenThread - %s\n",
				 strerror(errno));
		slon_retry();
	}
	pthread_cond_wait(&slon_wait_listen_cond, &slon_wait_listen_lock);
	if (!slon_listen_started)
	{
		/**
		 * The local listen thread did not start up properly.
		 */
		slon_log(SLON_FATAL, "main: localListenThread did not start\n");
		slon_abort();
	}
	pthread_mutex_unlock(&slon_wait_listen_lock);

	/*
	 * Enable all nodes that are active
	 */
	rtcfg_doActivate();

	/*
	 * Create the local cleanup thread that will remove old events and log
	 * data.
	 */
	if (pthread_create(&local_cleanup_thread, NULL, cleanupThread_main, NULL) < 0)
	{
		slon_log(SLON_FATAL, "main: cannot create cleanupThread - %s\n",
				 strerror(errno));
		slon_retry();
	}

	/*
	 * Create the local sync thread that will generate SYNC events if we had
	 * local database updates.
	 */
	if (pthread_create(&local_sync_thread, NULL, syncThread_main, NULL) < 0)
	{
		slon_log(SLON_FATAL, "main: cannot create syncThread - %s\n",
				 strerror(errno));
		slon_retry();
	}

	/*
	 * Create the local monitor thread that will process monitoring requests
	 */
	if (monitor_threads)
	{
		if (pthread_create(&local_monitor_thread, NULL, monitorThread_main, NULL) < 0)
		{
			slon_log(SLON_FATAL, "main: cannot create monitorThread - %s\n",
					 strerror(errno));
			slon_retry();
		}
	}

	/*
	 * Wait until the scheduler has shut down all remote connections
	 */
	slon_log(SLON_INFO, "main: running scheduler mainloop\n");
	if (sched_wait_mainloop() < 0)
	{
		slon_log(SLON_FATAL, "main: scheduler returned with error\n");
		slon_retry();
	}
	slon_log(SLON_INFO, "main: scheduler mainloop returned\n");

	/*
	 * Wait for all remote threads to finish
	 */
	main_thread = pthread_self();

	slon_log(SLON_CONFIG, "main: wait for remote threads\n");
	rtcfg_joinAllRemoteThreads();

	/*
	 * Wait for the local threads to finish
	 */
	if (pthread_join(local_event_thread, NULL) < 0)
		slon_log(SLON_ERROR, "main: cannot join localListenThread - %s\n",
				 strerror(errno));

	if (pthread_join(local_cleanup_thread, NULL) < 0)
		slon_log(SLON_ERROR, "main: cannot join cleanupThread - %s\n",
				 strerror(errno));

	if (pthread_join(local_sync_thread, NULL) < 0)
		slon_log(SLON_ERROR, "main: cannot join syncThread - %s\n",
				 strerror(errno));

	if (pthread_join(local_monitor_thread, NULL) < 0)
		slon_log(SLON_ERROR, "main: cannot join monitorThread - %s\n",
				 strerror(errno));

	slon_log(SLON_CONFIG, "main: done\n");

	exit(0);
}

#ifndef WIN32
/* ----------
 * SlonWatchdog
 * ----------
 */
static void
SlonWatchdog(void)
{
	pid_t		pid;
	int			shutdown = 0;
	int			return_code = -99;
	char	   *termination_reason = "unknown";

	slon_log(SLON_INFO, "slon: watchdog process started\n");



	slon_log(SLON_CONFIG, "slon: watchdog ready - pid = %d\n", slon_watchdog_pid);

	slon_worker_pid = fork();
	if (slon_worker_pid == 0)
	{
		SlonMain();
		exit(-1);
	}
	else if (slon_worker_pid < 0)
	{
		slon_log(SLON_FATAL, "slon: failed to fork child: %d %s\n",
				 errno, strerror(errno));
		slon_exit(-1);

	}

	/*
	 * Install signal handlers
	 */

	if (install_signal_handler(SIGHUP, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGHUP signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}

	if (install_signal_handler(SIGUSR1, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGUSR1 signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}
	if (install_signal_handler(SIGALRM, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGALRM signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}
	if (install_signal_handler(SIGINT, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGINT signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}
	if (install_signal_handler(SIGTERM, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGTERM signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}


	if (install_signal_handler(SIGQUIT, sighandler) == SIG_ERR)
	{
		slon_log(SLON_FATAL, "slon: SIGQUIT signal handler setup failed -(%d) %s\n", errno, strerror(errno));
		slon_exit(-1);
	}

	slon_log(SLON_CONFIG, "slon: worker process created - pid = %d\n",
			 slon_worker_pid);
	while (!shutdown)
	{
		while ((pid = wait(&child_status)) != slon_worker_pid)
		{
			if (pid < 0 && errno == EINTR)
				continue;

			slon_log(SLON_CONFIG, "slon: child terminated status: %d; pid: %d, current worker pid: %d errno: %d\n", child_status, pid, slon_worker_pid, errno);

			if (pid < 0)
			{
				/**
				 * if errno is not EINTR and pid<0 we have
				 * a problem.
				 * looping on wait() isn't a good idea.
				 */
				slon_log(SLON_FATAL, "slon: wait returned an error pid:%d errno:%d\n",
						 pid, errno);
				exit(-1);
			}
		}
		if (WIFSIGNALED(child_status))
		{
			return_code = WTERMSIG(child_status);
			termination_reason = "signal";
		}
		else if (WIFEXITED(child_status))
		{
			return_code = WEXITSTATUS(child_status);
			termination_reason = "exit code";
		}
		slon_log(SLON_CONFIG, "slon: child terminated %s: %d; pid: %d, current worker pid: %d\n", termination_reason, return_code, pid, slon_worker_pid);


		switch (watchdog_status)
		{
			case SLON_WATCHDOG_RESTART:
				slon_log(SLON_CONFIG, "slon: restart of worker in 20 seconds\n");
				sleep(20);
				slon_worker_pid = fork();
				if (slon_worker_pid == 0)
				{
					worker_restarted = 1;
					SlonMain();
					exit(-1);
				}
				else if (slon_worker_pid < 0)
				{
					slon_log(SLON_FATAL, "slon: failed to fork child: %d %s\n",
							 errno, strerror(errno));
					slon_exit(-1);

				}
				watchdog_status = SLON_WATCHDOG_NORMAL;
				continue;

			case SLON_WATCHDOG_NORMAL:
			case SLON_WATCHDOG_RETRY:
				watchdog_status = SLON_WATCHDOG_RETRY;
				if (child_status != 0)
				{
					slon_log(SLON_CONFIG, "slon: restart of worker in 10 seconds\n");
					(void) sleep(10);
				}
				else
				{
					slon_log(SLON_CONFIG, "slon: restart of worker\n");
				}
				if (watchdog_status == SLON_WATCHDOG_RETRY)
				{
					slon_worker_pid = fork();
					if (slon_worker_pid == 0)
					{
						worker_restarted = 1;
						SlonMain();
						exit(-1);
					}
					else if (slon_worker_pid < 0)
					{
						slon_log(SLON_FATAL, "slon: failed to fork child: %d %s\n",
								 errno, strerror(errno));
						slon_exit(-1);

					}
					watchdog_status = SLON_WATCHDOG_NORMAL;
					continue;
				}
				break;

			default:
				shutdown = 1;
				break;
		}						/* switch */
	}							/* while */

	slon_log(SLON_INFO, "slon: done\n");

	/*
	 * That's it.
	 */
	slon_exit(0);
}


/* ----------
 * sighandler
 * ----------
 */
static void
sighandler(int signo)
{
	switch (signo)
	{
		case SIGALRM:
			kill(slon_worker_pid, SIGKILL);
			break;

		case SIGCHLD:
			break;

		case SIGHUP:
			watchdog_status = SLON_WATCHDOG_RESTART;
			slon_terminate_worker();
			break;

		case SIGUSR1:
			watchdog_status = SLON_WATCHDOG_RETRY;
			slon_terminate_worker();
			break;

		case SIGINT:
		case SIGTERM:
			watchdog_status = SLON_WATCHDOG_SHUTDOWN;
			slon_terminate_worker();
			break;

		case SIGQUIT:
			kill(slon_worker_pid, SIGKILL);
			slon_exit(-1);
			break;
	}
}


/* ----------
 * slon_terminate_worker
 * ----------
 */
void
slon_terminate_worker()
{
	(void) kill(slon_worker_pid, SIGKILL);

}
#endif
/* ----------
 * slon_exit
 * ----------
 */
static void
slon_exit(int code)
{
#ifdef WIN32
	/* Cleanup winsock */
	WSACleanup();
#endif

	if (pid_file)
	{
		slon_log(SLON_INFO, "slon: remove pid file\n");
		(void) unlink(pid_file);
	}

	slon_log(SLON_INFO, "slon: exit(%d)\n", code);

	exit(code);
}

static sighandler_t
install_signal_handler(int signo, sighandler_t handler)
{


#ifndef WIN32
	struct sigaction act;

	act.sa_handler = handler;
	(void) sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;



	if (sigaction(signo, &act, NULL) < 0)
	{
		return SIG_ERR;
	}
	return handler;
#else
	return signal(signo, handler);
#endif
}

/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
