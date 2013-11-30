/*-------------------------------------------------------------------------
 * monitor_thread.c
 *
 *	Implementation of the thread that manages monitoring
 *
 *	Copyright (c) 2011, PostgreSQL Global Development Group
 *	Author: Christopher Browne, Afilias Canada
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include "types.h"
#include "slon.h"

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

static void stack_init(void);
static bool stack_pop(SlonState * current);
#ifdef UNUSED
static void stack_dump();
static void entry_dump(int i, SlonState * tos);
#endif
static int	initial_stack_size = 6;

/* ----------
 * Global variables
 * ----------
 */
#define EMPTY_STACK -1
static SlonState *mstack;
static int	stack_size = EMPTY_STACK;
static int	stack_maxlength = 1;
static pthread_mutex_t stack_lock = PTHREAD_MUTEX_INITIALIZER;
int			monitor_interval;

/* ----------
 * slon_localMonitorThread
 *
 * Monitoring thread that periodically flushes stacked-up monitoring requests to database
 * ----------
 */
void *
monitorThread_main(void *dummy)
{
	SlonConn   *conn;
	SlonDString beginquery,
				commitquery;
	SlonDString monquery;

	PGconn	   *dbconn;
	PGresult   *res;
	SlonState	state;
	ScheduleStatus rc;

	slon_log(SLON_INFO,
			 "monitorThread: thread starts\n");
	pthread_mutex_lock(&stack_lock);
	stack_init();
	pthread_mutex_unlock(&stack_lock);


	/*
	 * Connect to the local database
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "local_monitor")) == NULL)
	{
		slon_log(SLON_ERROR, "monitorThread: failure to connect to local database\n");
	}
	else
	{
		dbconn = conn->dbconn;
		slon_log(SLON_DEBUG2, "monitorThread: setup DB conn\n");

		/* Start by emptying the sl_components table */
		dstring_init(&monquery);
		slon_mkquery(&monquery,
					 "delete from %s.sl_components;",
					 rtcfg_namespace);

		res = PQexec(dbconn, dstring_data(&monquery));
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			slon_log(SLON_ERROR,
					 "monitorThread: \"%s\" - %s\n",
					 dstring_data(&monquery), PQresultErrorMessage(res));
			PQclear(res);
			dstring_free(&monquery);
			monitor_threads = false;
			slon_disconnectdb(conn);
			slon_log(SLON_ERROR, "monitorThread: exit monitoring thread\n");
			pthread_exit(NULL);
			return (void *) 0;
		}
		else
		{
			PQclear(res);
			dstring_free(&monquery);
		}

		monitor_state("local_monitor", 0, (pid_t) conn->conn_pid, "thread main loop", 0, "n/a");

		/*
		 * set up queries that are run in each iteration
		 */
		dstring_init(&beginquery);
		slon_mkquery(&beginquery,
					 "start transaction;");

		dstring_init(&commitquery);
		slon_mkquery(&commitquery, "commit;");

		while ((rc = (ScheduleStatus) sched_wait_time(conn, SCHED_WAIT_SOCK_READ, monitor_interval) == SCHED_STATUS_OK))
		{
			int			qlen;

			pthread_mutex_lock(&stack_lock);	/* lock access to stack size */
			qlen = stack_size;
			pthread_mutex_unlock(&stack_lock);
			if (qlen >= 0)
			{
				res = PQexec(dbconn, dstring_data(&beginquery));
				if (PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					slon_log(SLON_ERROR,
							 "monitorThread: \"%s\" - %s",
					   dstring_data(&beginquery), PQresultErrorMessage(res));
					PQclear(res);
					break;
				}
				PQclear(res);

				/*
				 * Now, iterate through stack contents, and dump them all to
				 * the database
				 */
				while (stack_pop(&state))
				{
					dstring_init(&monquery);
					slon_mkquery(&monquery,
								 "select %s.component_state('%s', %d, %d,",
						rtcfg_namespace, state.actor, state.pid, state.node);
					if (state.conn_pid > 0)
					{
						slon_appendquery(&monquery, "%d, ", state.conn_pid);
					}
					else
					{
						slon_appendquery(&monquery, "NULL::integer, ");
					}
					if ((state.activity != 0) && strlen(state.activity) > 0)
					{
						slon_appendquery(&monquery, "'%s', ", state.activity);
					}
					else
					{
						slon_appendquery(&monquery, "NULL::text, ");
					}
					slon_appendquery(&monquery, "'1970-01-01 0:0:0 UTC'::timestamptz + '%d seconds'::interval, ", time(NULL));
					if (state.event > 0)
					{
						slon_appendquery(&monquery, "%L, ", state.event);
					}
					else
					{
						slon_appendquery(&monquery, "NULL::bigint, ");
					}
					if ((state.event_type != 0) && strlen(state.event_type) > 0)
					{
						slon_appendquery(&monquery, "'%s');", state.event_type);
					}
					else
					{
						slon_appendquery(&monquery, "NULL::text);");
					}
					if (state.actor != NULL)
						free(state.actor);
					if (state.activity != NULL)
						free(state.activity);
					if (state.event_type != NULL)
						free(state.event_type);
					res = PQexec(dbconn, dstring_data(&monquery));
					if (PQresultStatus(res) != PGRES_TUPLES_OK)
					{
						slon_log(SLON_ERROR,
								 "monitorThread: \"%s\" - %s",
						 dstring_data(&monquery), PQresultErrorMessage(res));
						PQclear(res);
						dstring_free(&monquery);
						break;
					}
					PQclear(res);
					dstring_free(&monquery);
				}

				res = PQexec(dbconn, dstring_data(&commitquery));
				if (PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					slon_log(SLON_ERROR,
							 "monitorThread: %s - %s\n",
							 dstring_data(&commitquery),
							 PQresultErrorMessage(res));
				}
				PQclear(res);
			}
			if ((rc = (ScheduleStatus) sched_msleep(NULL, monitor_interval)) != SCHED_STATUS_OK)
			{
				break;
			}
		}
		monitor_state("local_monitor", 0, (pid_t) conn->conn_pid, "just running", 0, "n/a");
	}
	slon_log(SLON_CONFIG, "monitorThread: exit main loop\n");

	dstring_free(&beginquery);
	dstring_free(&commitquery);
	slon_disconnectdb(conn);

	slon_log(SLON_INFO, "monitorThread: thread done\n");
	monitor_threads = false;
	pthread_exit(NULL);
	return (void *) 0;
}

static void
stack_init(void)
{
	stack_maxlength = initial_stack_size;
	mstack = malloc(sizeof(SlonState) * (stack_maxlength + 1));
	if (mstack == NULL)
	{
		slon_log(SLON_FATAL, "stack_init() - malloc() failure could not allocate %d stack slots\n", stack_maxlength);
		slon_retry();
	}
	else
	{
		slon_log(SLON_DEBUG2, "stack_init() - initialize stack to size %d\n", stack_maxlength);
	}
	stack_size = EMPTY_STACK;
}

void
monitor_state(const char *actor, int node, pid_t conn_pid, /* @null@ */ const char *activity, int64 event, /* @null@ */ const char *event_type)
{
	size_t		len;
	SlonState  *tos;
	SlonState  *nstack;
	char	   *ns;
	pid_t		mypid;

	if (!monitor_threads)		/* Don't collect if this thread is shut off */
		return;

	mypid = getpid();
	pthread_mutex_lock(&stack_lock);
	if (mstack == NULL)
	{
		stack_init();
	}
	if (stack_size >= stack_maxlength)
	{
		/* Need to reallocate stack */
		if (stack_size > 100)
		{
			slon_log(SLON_WARN, "monitorThread: stack reallocation - size %d > warning threshold of 100.  Stack perhaps isn't getting processed properly by monitoring thread\n", stack_size);
		}
		stack_maxlength *= 2;

		nstack = realloc(mstack, (size_t) ((stack_maxlength + 1) * sizeof(SlonState)));
		if (nstack == NULL)
		{
			slon_log(SLON_FATAL, "stack_init() - malloc() failure could not allocate %d stack slots\n", stack_maxlength);
			pthread_mutex_unlock(&stack_lock);
			slon_retry();
		}
		mstack = nstack;
	}

	/* if actor matches, then we can do an in-place update */
	if (stack_size != EMPTY_STACK)
	{
		tos = mstack + stack_size;
		len = strlen(actor);
		if (strncmp(actor, tos->actor, len) == 0)
		{
			if (tos->actor != NULL)
			{
				free(tos->actor);
			}
			if (tos->activity != NULL)
			{
				free(tos->activity);
			}
			if (tos->event_type != NULL)
			{
				free(tos->event_type);
			}
		}
		else
		{
			stack_size++;
		}
	}
	else
	{
		stack_size++;
	}
	tos = mstack + stack_size;
	tos->pid = mypid;
	tos->node = node;
	tos->conn_pid = conn_pid;
	tos->event = event;

/* It might seem somewhat desirable for the database to record
 *	DB-centred timestamps, unfortunately that would only be the
 *	correct time if each thread were responsible for stowing its own
 *	activities in sl_components in the database.  This would multiply
 *	database activity, and the implementation instead passes requests
 *	to a single thread that uses a single DB connection to record
 *	things, with the consequence that timestamps must be captured
 *	based on the system clock of the slon process. */

	tos->start_time = time(NULL);
	if (actor != NULL)
	{
		len = strlen(actor);
		ns = malloc(sizeof(char) * len + 1);
		if (ns)
		{
			strncpy(ns, actor, len);
			ns[len] = '\0';
			tos->actor = ns;
		}
		else
		{
			slon_log(SLON_FATAL, "monitor_state - unable to allocate memory for actor (len %d)\n", len);
			pthread_mutex_unlock(&stack_lock);
			slon_retry();
		}
	}
	else
	{
		tos->actor = NULL;
	}
	if (activity != NULL)
	{
		len = strlen(activity);
		ns = malloc(sizeof(char) * len + 1);
		if (ns)
		{
			strncpy(ns, activity, len);
			ns[len] = (char) 0;
			tos->activity = ns;
		}
		else
		{
			slon_log(SLON_FATAL, "monitor_state - unable to allocate memory for activity (len %d)\n", len);
			pthread_mutex_unlock(&stack_lock);
			slon_retry();
		}
	}
	else
	{
		tos->activity = NULL;
	}
	if (event_type != NULL)
	{
		len = strlen(event_type);
		ns = malloc(sizeof(char) * len + 1);
		if (ns)
		{
			strncpy(ns, event_type, len);
			ns[len] = (char) 0;
			tos->event_type = ns;
		}
		else
		{
			slon_log(SLON_FATAL, "monitor_state - unable to allocate memory for event_type (len %d)\n", len);
			pthread_mutex_unlock(&stack_lock);
			slon_retry();
		}
	}
	else
	{
		tos->event_type = NULL;
	}
	pthread_mutex_unlock(&stack_lock);
}

/* Note that it is the caller's responsibility to free() the contents
   of strings ->actor, ->activity, ->event_type */
static bool
stack_pop( /* @out@ */ SlonState * qentry)
{
	SlonState  *ce = NULL;

	pthread_mutex_lock(&stack_lock);
	if (stack_size == EMPTY_STACK)
	{
		pthread_mutex_unlock(&stack_lock);
		return false;
	}
	else
	{
		ce = mstack + stack_size;
		qentry->actor = ce->actor;
		qentry->pid = ce->pid;
		qentry->node = ce->node;
		qentry->conn_pid = ce->conn_pid;
		qentry->activity = ce->activity;
		qentry->event = ce->event;
		qentry->event_type = ce->event_type;
		qentry->start_time = ce->start_time;
		/* entry_dump(stack_size, qentry); */
		stack_size--;
		pthread_mutex_unlock(&stack_lock);
		return true;
	}
}

#ifdef UNUSED
static void
stack_dump()
{
	int			i;
	SlonState  *tos;

	slon_log(SLON_DEBUG2, "monitorThread: stack_dump()\n");
	pthread_mutex_lock(&stack_lock);
	for (i = 0; i < stack_size; i++)
	{
		tos = mstack + i;
		entry_dump(i, tos);
	}
	slon_log(SLON_DEBUG2, "monitorThread: stack_dump done\n");
	pthread_mutex_unlock(&stack_lock);
}

/* Note that this function accesses stack contents, and thus needs to
 * be guarded by the pthread mutex on stack_lock */

static void
entry_dump(int i, SlonState * tos)
{
	slon_log(SLON_DEBUG2, "stack[%d]=%d\n",
			 i, tos);
	slon_log(SLON_DEBUG2, "pid:%d node:%d connpid:%d event:%lld\n",
			 tos->pid, tos->node, tos->conn_pid, tos->event);
	slon_log(SLON_DEBUG2, "actor[%s] activity[%s] event_type[%s]\n",
			 tos->actor, tos->activity, tos->event_type);
}
#endif /* UNUSED */
