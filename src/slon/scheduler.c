/*-------------------------------------------------------------------------
 * scheduler.c
 *
 *	Event scheduling subsystem for slon.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: scheduler.c,v 1.9 2004-03-23 18:53:11 wieck Exp $
 *-------------------------------------------------------------------------
 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "libpq-fe.h"
#include "c.h"

#include "slon.h"


/*
 * If PF_LOCAL is not defined, use the old BSD name PF_UNIX
 */
#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif


/* ----------
 * Static data
 * ----------
 */
static int				sched_status = SCHED_STATUS_OK;

static int				sched_numfd = 0;
static fd_set			sched_fdset_read;
static fd_set			sched_fdset_write;
static int				sched_sockpair[2];
static SlonConn		   *sched_waitqueue_head = NULL;
static SlonConn		   *sched_waitqueue_tail = NULL;

static pthread_t		sched_main_thread;
static pthread_t		sched_scheduler_thread;

static pthread_mutex_t	sched_master_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	sched_master_cond = PTHREAD_COND_INITIALIZER;

static sigset_t			sched_sigset;


/* ----------
 * Local functions
 * ----------
 */
static void			   *sched_mainloop(void *);
static void				sched_sighandler(int signo);
static void				sched_sighuphandler(int signo);
static void				sched_add_fdset(int fd, fd_set *fds);
static void				sched_remove_fdset(int fd, fd_set *fds);


/* ----------
 * sched_start_mainloop
 *
 *	Called from main() before starting up any worker thread.
 *
 *	This will spawn the event scheduling thread that does the
 *	central select(2) system call.
 * ----------
 */
int
sched_start_mainloop(void)
{
	/*
	 * Remember the main threads identifier
	 */
	sched_main_thread = pthread_self();

	/*
	 * Block signals. Since sched_start_mainloop() is called before
	 * any other thread is created, this will be inherited by all
	 * threads in the system. The two signals will explicitly be
	 * unblocked again while we're waiting for the scheduler thread.
	 */
	signal(SIGHUP, sched_sighuphandler);
	signal(SIGINT, sched_sighandler);
	signal(SIGTERM, sched_sighandler);
	
	sigemptyset(&sched_sigset);
	sigaddset(&sched_sigset, SIGHUP);
	sigaddset(&sched_sigset, SIGINT);
	sigaddset(&sched_sigset, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sched_sigset, NULL);

	/*
	 * Grab the scheduler master lock
	 */
	if (pthread_mutex_lock(&sched_master_lock) < 0)
	{
		perror("sched_start_mainloop: pthread_mutex_lock()");
		return -1;
	}

	/*
	 * Start the scheduler thread
	 */
	if (pthread_create(&sched_scheduler_thread, NULL, sched_mainloop, NULL) < 0)
	{
		perror("sched_start_mainloop: pthread_create()");
		return -1;
	}

	/*
	 * When the scheduler is ready, he'll signal the scheduler cond
	 */
	if (pthread_cond_wait(&sched_master_cond, &sched_master_lock) < 0)
	{
		perror("sched_start_mainloop: pthread_cond_wait()");
		return -1;
	}

	/*
	 * Release the scheduler lock
	 */
	if (pthread_mutex_unlock(&sched_master_lock) < 0)
	{
		perror("sched_start_mainloop: pthread_mutex_unlock()");
		return -1;
	}

	/*
	 * Check for errors
	 */
	if (sched_status != SCHED_STATUS_OK)
		return -1;

	/*
	 * Scheduler started successfully
	 */
	return 0;
}


/* ----------
 * sched_wait_mainloop
 *
 *	Called from main() after all working threads according to the
 *	initial configuration are started. Will wait until the scheduler
 *	mainloop terminates.
 * ----------
 */
int
sched_wait_mainloop(void)
{
	/*
	 * Unblock signals.
	 */
	pthread_sigmask(SIG_UNBLOCK, &sched_sigset, NULL);

	/*
	 * Wait for the scheduler to finish.
	 */
	if (pthread_join(sched_scheduler_thread, NULL) < 0)
	{
		perror("sched_wait_mainloop: pthread_join()");
		return -1;
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	return 0;
}


/* ----------
 * sched_wait_conn
 *
 *	Assumes that the thread holds the lock on conn->conn_lock.
 *
 *	Adds the connection to the central wait queue and wakes up
 *	the scheduler thread to reloop onto the select(2) call.
 * ----------
 */
int
sched_wait_conn(SlonConn *conn, int condition)
{
	int		rc;

	/*
	 * Grab the master lock and check that we're in normal runmode
	 */
	pthread_mutex_lock(&sched_master_lock);
	if (sched_status != SCHED_STATUS_OK)
	{
		pthread_mutex_unlock(&sched_master_lock);
		return -1;
	}

	/*
	 * Remember the event we're waiting for and add the database 
	 * connection to the fdset(s)
	 */
	conn->condition = condition;
	if (condition & SCHED_WAIT_SOCK_READ)
		sched_add_fdset(PQsocket(conn->dbconn), &sched_fdset_read);
	if (condition & SCHED_WAIT_SOCK_WRITE)
		sched_add_fdset(PQsocket(conn->dbconn), &sched_fdset_write);

	/*
	 * Add the connection to the wait queue
	 */
	DLLIST_ADD_HEAD(sched_waitqueue_head, sched_waitqueue_tail, conn);

	/*
	 * Give the scheduler thread a heads up, release the master lock
	 * and wait for it to tell us that the event we're waiting for happened.
	 */
	if (write(sched_sockpair[1], "x", 1) < 0)
	{
		perror("sched_wait_conn: write()");
		exit(-1);
	}
	pthread_mutex_unlock(&sched_master_lock);
	pthread_cond_wait(&(conn->conn_cond), &(conn->conn_lock));

	/*
	 * Determine the return code
	 */
	pthread_mutex_lock(&sched_master_lock);
	if ((rc = sched_status) == SCHED_STATUS_OK)
	{
		if (conn->condition & SCHED_WAIT_CANCEL)
		{
			conn->condition &= ~(SCHED_WAIT_CANCEL);
			rc = SCHED_STATUS_CANCEL;
		}
	}
	pthread_mutex_unlock(&sched_master_lock);

	return rc;
}


/* ----------
 * sched_wait_time
 *
 *	Assumes that the thread holds the lock on conn->conn_lock.
 *
 *	Like sched_wait_time() but with a timeout. Can be called without
 *	any read/write condition to wait for to resemble a pure timeout
 *	mechanism.
 * ----------
 */
int
sched_wait_time(SlonConn *conn, int condition, int msec)
{
	struct timeval *tv = &(conn->timeout);

	/*
	 * Calculate the end-time of the desired timeout.
	 */
	gettimeofday(tv, NULL);
	tv->tv_sec += (long)(msec / 1000) + 
					(((msec % 1000) * 1000) + tv->tv_usec) / 1000000;
	tv->tv_usec = (tv->tv_usec + (msec % 1000) * 1000) % 1000000;

	/*
	 * Let sched_wait_conn() do the rest.
	 */
	return sched_wait_conn(conn, condition | SCHED_WAIT_TIMEOUT);
}


/* ----------
 * sched_msleep
 *
 *	Use the schedulers event loop to sleep for msec milliseconds.
 * ----------
 */
int
sched_msleep(SlonNode *node, int msec)
{
	SlonConn   *conn;
	char		dummyconn_name[64];
	int			rc;

	if (node)
	{
		snprintf(dummyconn_name, 64, "msleep_node_%d", node->no_id);
		conn = slon_make_dummyconn(dummyconn_name);
	}
	else
		conn = slon_make_dummyconn("msleep_local");

	rc = sched_wait_time(conn, 0, msec);
	slon_free_dummyconn(conn);

	return rc;
}


/* ----------
 * sched_get_status
 *
 *	Return the current scheduler status in a thread safe fashion
 * ----------
 */
int
sched_get_status(void)
{
	int		status;

	pthread_mutex_lock(&sched_master_lock);
	status = sched_status;
	pthread_mutex_unlock(&sched_master_lock);
	return status;
}


/* ----------
 * sched_wakeup_node
 *
 *	Wakeup the threads (listen and worker) of one or all remote
 *	nodes to cause them rechecking the current runtime status or
 *	adjust their configuration to changes.
 * ----------
 */
int
sched_wakeup_node (int no_id)
{
	SlonConn   *conn;
	int			num_wakeup = 0;

	pthread_mutex_lock(&sched_master_lock);

	/*
	 * Set all waiters that belong to that node to cancel
	 */
	for (conn = sched_waitqueue_head; conn; conn = conn->next)
	{
		if (conn->node != NULL)
		{
			if (no_id < 0 || conn->node->no_id == no_id)
			{
				conn->condition |= SCHED_WAIT_CANCEL;
				num_wakeup++;
			}
		}
	}

	/*
	 * Give the scheduler thread a heads up if some wait was canceled;
	 */
	if (num_wakeup > 0)
	{
		if (write(sched_sockpair[1], "x", 1) < 0)
		{
			perror("sched_wait_conn: write()");
			slon_abort();
		}
	}

	pthread_mutex_unlock(&sched_master_lock);

	remoteWorker_wakeup(no_id);

	slon_log(SLON_DEBUG2, "sched_wakeup_node(): no_id=%d "
			"(%d threads + worker signaled)\n", no_id, num_wakeup);

	return num_wakeup;
}


/* ----------
 * sched_mainloop
 *
 *	The thread handling the master scheduling.
 * ----------
 */
static void *
sched_mainloop(void *dummy)
{
	fd_set			rfds;
	fd_set			wfds;
	int				rc;
	SlonConn	   *conn;
	SlonConn	   *next;
	struct timeval	min_timeout;
	struct timeval *tv;

	/*
	 * Grab the scheduler master lock. This will wait until the
	 * main thread acutally blocks on the master cond.
	 */
	pthread_mutex_lock(&sched_master_lock);

	/*
	 * Initialize the fdsets for select(2)
	 */
	FD_ZERO(&sched_fdset_read);
	FD_ZERO(&sched_fdset_write);

	/*
	 * Create a socketpair used by the main thread to cleanly
	 * wakeup the scheduler on signals.
	 */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sched_sockpair) < 0)
	{
		perror("sched_mainloop: socketpair()");
		sched_status = SCHED_STATUS_ERROR;
		pthread_cond_signal(&sched_master_cond);
		pthread_exit(NULL);
	}
	sched_add_fdset(sched_sockpair[0], &sched_fdset_read);

	/*
	 * Done with all initialization. Let the main thread go
	 * ahead and get everyone else dancing.
	 */
	pthread_cond_signal(&sched_master_cond);

	/*
	 * And we are now entering the endless loop of scheduling events
	 */
	while(sched_status == SCHED_STATUS_OK)
	{
		struct timeval	now;
		struct timeval	timeout;

		/*
		 * Make copies of the file descriptor sets for select(2)
		 */
		memcpy(&rfds, &sched_fdset_read, sizeof(fd_set));
		memcpy(&wfds, &sched_fdset_write, sizeof(fd_set));

		/*
		 * Check if any of the connections in the wait queue
		 * have reached there timeout. While doing so, we also
		 * remember the closest timeout in the future.
		 */
		tv = NULL;
		gettimeofday(&now, NULL);
		for (conn = sched_waitqueue_head; conn;)
		{
			next = conn->next;

			if (conn->condition & SCHED_WAIT_CANCEL)
			{
				/*
				 * Some other thread wants this thread to
				 * wake up.
				 */
				DLLIST_REMOVE(sched_waitqueue_head, 
							sched_waitqueue_tail, conn);

				if (conn->condition & SCHED_WAIT_SOCK_READ)
					sched_remove_fdset(PQsocket(conn->dbconn), 
									&sched_fdset_read);
				if (conn->condition & SCHED_WAIT_SOCK_WRITE)
					sched_remove_fdset(PQsocket(conn->dbconn),
									&sched_fdset_write);

				pthread_mutex_lock(&(conn->conn_lock));
				pthread_cond_signal(&(conn->conn_cond));
				pthread_mutex_unlock(&(conn->conn_lock));

				conn = next;
				continue;
			}

			if (conn->condition & SCHED_WAIT_TIMEOUT)
			{
				/*
				 * This connection has a timeout. Calculate the
				 * time until that.
				 */
				timeout.tv_sec  = conn->timeout.tv_sec  - now.tv_sec;
				timeout.tv_usec = conn->timeout.tv_usec - now.tv_usec;
				while (timeout.tv_usec < 0)
				{
					timeout.tv_sec--;
					timeout.tv_usec += 1000000;
				}

				/*
				 * Check if the timeout has elapsed
				 */
				if (timeout.tv_sec < 0 ||
						(timeout.tv_sec == 0 && timeout.tv_usec < 20000))
				{
					/*
					 * Remove the connection from the wait queue. We
					 * consider everything closer than 20 msec being
					 * elapsed to avoid a full scheduler round just
					 * for one kernel tick.
					 */
					DLLIST_REMOVE(sched_waitqueue_head, 
								sched_waitqueue_tail, conn);

					if (conn->condition & SCHED_WAIT_SOCK_READ)
						sched_remove_fdset(PQsocket(conn->dbconn), 
										&sched_fdset_read);
					if (conn->condition & SCHED_WAIT_SOCK_WRITE)
						sched_remove_fdset(PQsocket(conn->dbconn),
										&sched_fdset_write);

					pthread_mutex_lock(&(conn->conn_lock));
					pthread_cond_signal(&(conn->conn_cond));
					pthread_mutex_unlock(&(conn->conn_lock));
				}
				else
				{
					/*
					 * Timeout not elapsed. Remember the nearest.
					 */
					if (tv == NULL ||
							timeout.tv_sec < min_timeout.tv_sec ||
							(timeout.tv_sec == min_timeout.tv_sec &&
								timeout.tv_usec < min_timeout.tv_usec))
					{
						tv = &min_timeout;
						min_timeout.tv_sec  = timeout.tv_sec;
						min_timeout.tv_usec = timeout.tv_usec;
					}
				}
			}

			conn = next;
		}

		/*
		 * Do the select(2) while unlocking the master lock.
		 */
		pthread_mutex_unlock(&sched_master_lock);
		rc = select(sched_numfd, &rfds, &wfds, NULL, tv);
		pthread_mutex_lock(&sched_master_lock);

		/*
		 * Check for errors
		 */
		if (rc < 0)
		{
			perror("sched_mainloop: select()");
			sched_status = SCHED_STATUS_ERROR;
			break;
		}

		/*
		 * Check the special socketpair for a heads up.
		 */
		if (FD_ISSET(sched_sockpair[0], &rfds))
		{
			char	buf[1];

			rc--;
			if (read(sched_sockpair[0], buf, 1) != 1)
			{
				perror("sched_mainloop: read()");
				sched_status = SCHED_STATUS_ERROR;
				break;
			}
		}

		/*
		 * Check all remaining connections if the IO condition the
		 * thread is waiting for has occured.
		 */
		conn = sched_waitqueue_head;
		while(rc > 0 && conn)
		{
			if (conn->condition & SCHED_WAIT_SOCK_READ)
			{
				if (FD_ISSET(PQsocket(conn->dbconn), &rfds))
				{
					next = conn->next;

					pthread_mutex_lock(&(conn->conn_lock));
					pthread_cond_signal(&(conn->conn_cond));
					pthread_mutex_unlock(&(conn->conn_lock));
					rc--;

					DLLIST_REMOVE(sched_waitqueue_head, 
								sched_waitqueue_tail, conn);

					if (conn->condition & SCHED_WAIT_SOCK_READ)
						sched_remove_fdset(PQsocket(conn->dbconn), 
										&sched_fdset_read);
					if (conn->condition & SCHED_WAIT_SOCK_WRITE)
						sched_remove_fdset(PQsocket(conn->dbconn),
										&sched_fdset_write);

					conn = next;
					continue;
				}
			}

			if (conn->condition & SCHED_WAIT_SOCK_WRITE)
			{
				if (FD_ISSET(PQsocket(conn->dbconn), &wfds))
				{
					next = conn->next;

					pthread_mutex_lock(&(conn->conn_lock));
					pthread_cond_signal(&(conn->conn_cond));
					pthread_mutex_unlock(&(conn->conn_lock));
					rc--;

					DLLIST_REMOVE(sched_waitqueue_head, 
								sched_waitqueue_tail, conn);

					if (conn->condition & SCHED_WAIT_SOCK_READ)
						sched_remove_fdset(PQsocket(conn->dbconn), 
										&sched_fdset_read);
					if (conn->condition & SCHED_WAIT_SOCK_WRITE)
						sched_remove_fdset(PQsocket(conn->dbconn),
										&sched_fdset_write);

					conn = next;
					continue;
				}
			}

			conn = conn->next;
		}
	}

	/*
	 * If we reach here the scheduler runmode has been changed by
	 * by the main threads signal handler. We currently hold the
	 * master lock. First we close the scheduler heads-up socket
	 * pair so nobody will think we're listening any longer.
	 */
	close(sched_sockpair[0]);
	close(sched_sockpair[1]);
	sched_sockpair[0] = sched_sockpair[1] = -1;

	/*
	 * Then we cond_signal all connections that are in the queue.
	 */
	for (conn = sched_waitqueue_head; conn;)
	{
		next = conn->next;

		pthread_mutex_lock(&(conn->conn_lock));
		pthread_cond_signal(&(conn->conn_cond));
		pthread_mutex_unlock(&(conn->conn_lock));

		DLLIST_REMOVE(sched_waitqueue_head, 
					sched_waitqueue_tail, conn);

		if (conn->condition & SCHED_WAIT_SOCK_READ)
			sched_remove_fdset(PQsocket(conn->dbconn), 
							&sched_fdset_read);
		if (conn->condition & SCHED_WAIT_SOCK_WRITE)
			sched_remove_fdset(PQsocket(conn->dbconn),
							&sched_fdset_write);

		conn = next;
	}

	/*
	 * Release the master lock and terminate the scheduler thread.
	 */
	pthread_mutex_unlock(&sched_master_lock);
	pthread_exit(NULL);
}


/* ----------
 * sched_sighandler
 *
 *	After starting up the sole purpose of the main thread (the original
 *	process) is to respond to signals while waiting for the scheduler to
 *	finish. The signal handler is here because it is actually
 *	sched_start_mainloop() that arranges for the signal handling and
 *	sched_wait_mainloop() that enables them. And the only one really
 *	interested in the signals is the scheduler thread ... but that's doing
 *	select(2) mainly and we have to avoid race conditions with signals.
 * ----------
 */
static void
sched_sighandler(int signo)
{
	/*
	 * Lock the master mutex and make sure that we are the main thread
	 */
	pthread_mutex_lock(&sched_master_lock);
	if (pthread_self() != sched_main_thread)
	{
		slon_log(SLON_FATAL, "sched_sighandler: called in non-main thread\n");
		slon_abort();
	}

	/*
	 * Set the scheduling status to shutdown
	 */
	if (sched_status == SCHED_STATUS_OK)
		sched_status = SCHED_STATUS_SHUTDOWN;

	/*
	 * Try to wakeup the scheduler thread by throwing a bait
	 */
	if (sched_sockpair[1] < 0)
	{
		slon_log(SLON_ERROR, "sched_sighandler: sockpair already closed\n");
		pthread_mutex_unlock(&sched_master_lock);
		return;
	}
	if (write(sched_sockpair[1], "x", 1) < 0)
	{
		perror("sched_sighandler: write()");
		pthread_mutex_unlock(&sched_master_lock);
		exit(-1);
	}

	/*
	 * Unlock the master mutex
	 */
	pthread_mutex_unlock(&sched_master_lock);
}


static void
sched_sighuphandler(int signo)
{
	slon_restart_request = true;
	sched_sighandler(signo);
}


/* ----------
 * sched_add_fdset
 *
 *	Add a file descriptor to one of the global scheduler sets and
 *	adjust sched_numfd accordingly.
 * ----------
 */
static void
sched_add_fdset(int fd, fd_set *fds)
{
	FD_SET(fd, fds);
	if (fd >= sched_numfd)
		sched_numfd = fd + 1;
}


/* ----------
 * sched_add_fdset
 *
 *	Remove a file descriptor from one of the global scheduler sets and
 *	adjust sched_numfd accordingly.
 * ----------
 */
static void
sched_remove_fdset(int fd, fd_set *fds)
{
	FD_CLR(fd, fds);
	if (sched_numfd == (fd + 1))
	{
		while (sched_numfd > 0)
		{
			if (FD_ISSET(sched_numfd - 1, &sched_fdset_read))
				break;
			if (FD_ISSET(sched_numfd - 1, &sched_fdset_write))
				break;
			sched_numfd--;
		}
	}
}


