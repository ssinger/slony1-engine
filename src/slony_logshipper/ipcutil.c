/*-------------------------------------------------------------------------
 * ipcutil.c
 *
 *	Semaphore and message passing support for the slony_logshipper.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 *-------------------------------------------------------------------------
 */


#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#endif
#include "../slonik/types.h"
#include "libpq-fe.h"

#include "slony_logshipper.h"


/*
 * The daemonized logshipper keeps a sorted queue of archive
 * files that need processing.
 */
typedef struct queue_elem_s
{
	char	   *archive_path;
	struct queue_elem_s *next;
}	queue_elem;


/*
 * Static data
 */
static char *ipc_archive_dir = NULL;
static key_t semkey;
static key_t msgkey;
static int	semid;
static int	msgid;
static int	ipc_creator;
static queue_elem *archive_queue_head = NULL;
static queue_elem *archive_queue_tail = NULL;


/*
 * Local functions
 */
static int	ipc_generate_keys(char *archive_dir);
static void ipc_sighandler(int sig);
static int	ipc_add_path(char *path);
static int	ipc_send_code(char *archive_dir, int code);


/*
 * ipc_init() -
 *
 *	Called to setup or connect to the semaphore set and message queue.
 */
int
ipc_init(char *archive_dir)
{
	struct sembuf sops[2];

	if (ipc_generate_keys(archive_dir) < 0)
		return -1;

	/*
	 * We eventually have to start over again in case the existing daemon
	 * destroys the semaphore set after we attached and before we can lock it.
	 */
	while (true)
	{
		/*
		 * Create or attach to the semaphore set
		 */
		semid = semget(semkey, 3, 0700 | IPC_CREAT);
		if (semid < 0)
		{
			fprintf(stderr, "cannot create or attache to semaphore set\n"
					"semget(): %s\n", strerror(errno));
			return -1;
		}

		/*
		 * We now do two initial operations with NOWAIT: wait for #1 =0 inc
		 * sem	#1 +1 We never again touch semaphore #1, so this either
		 * succeeds, meaning that we created the set and hold the current
		 * lock. Or it fails with EAGAIN, meaning we attached to an existing
		 * set. Or it fails with EIDRM, meaning the set was destroyed.
		 */
		sops[0].sem_num = 1;
		sops[0].sem_op = 0;
		sops[0].sem_flg = IPC_NOWAIT;
		sops[1].sem_num = 1;
		sops[1].sem_op = 1;
		sops[1].sem_flg = 0;

		if (semop(semid, sops, 2) < 0)
		{
			if (errno == EIDRM)
				continue;

			if (errno != EAGAIN)
			{
				fprintf(stderr, "semop failed in ipc_init(): %s",
						strerror(errno));
				return -1;
			}

			/*
			 * Grab the lock on semaphore #0
			 */
			if (ipc_lock() < 0)
			{
				/*
				 * Since theres a gap between attaching and locking, the set
				 * could have been destroyed. In that case, start over.
				 */
				if (errno == EIDRM)
					continue;

				fprintf(stderr, "semop() failed in ipc_init(): %s\n",
						strerror(errno));
				return -1;
			}

			ipc_creator = 0;
		}
		else
		{
			/*
			 * Initial semop succeeded - we are the creator of this set.
			 */
			ipc_creator = 1;
			signal(SIGINT, ipc_sighandler);
			signal(SIGTERM, ipc_sighandler);
			signal(SIGPIPE, ipc_sighandler);
		}

		break;
	}

	/*
	 * At this point we have the semaphore set and it is locked.
	 */
	msgid = msgget(msgkey, 0700 | IPC_CREAT);
	if (msgid < 0)
	{
		fprintf(stderr, "msgget() failed in ipc_init(): %s\n", strerror(errno));
		if (ipc_creator)
		{
			if (semctl(semid, 0, IPC_RMID) < 0)
				fprintf(stderr, "semctl() failed in ipc_init(): %s\n",
						strerror(errno));
		}
		return -1;
	}

	return ipc_creator;
}


/*
 * ipc_finish() -
 *
 *	Locks and destroys the semaphore set and message queue if called
 *	in the daemonized queue runner. If force isn't given, it will
 *	only do so if the archive queue is empty after locking the set
 *	and draining the message queue.
 */
int
ipc_finish(bool force)
{
	if (ipc_creator)
	{
		if (!force)
		{
			/*
			 * We are the creator of the semaphore set, so if this isn't a
			 * force operation, we lock it first, poll the message queue and
			 * check that we have an empty queue.
			 */
			if (ipc_lock() < 0)
			{
				fprintf(stderr, "semop() failed in ipc_finish(): %s\n",
						strerror(errno));
				return -1;
			}

			if (ipc_poll(false) < 0)
				return -1;

			if (archive_queue_head != NULL)
			{
				if (ipc_unlock() < 0)
				{
					fprintf(stderr, "semop() failed in ipc_finish(): %s\n",
							strerror(errno));
					return -1;
				}
				return 1;
			}
		}

		/*
		 * At this point, we are either forced to stop or we have a lock and
		 * the queue is empty.
		 */
		if (msgctl(msgid, IPC_RMID, NULL) < 0)
		{
			fprintf(stderr, "msgctl() failed in ipc_finish(): %s\n",
					strerror(errno));
			semctl(semid, 0, IPC_RMID);
			return -1;
		}

		if (semctl(semid, 0, IPC_RMID) < 0)
		{
			fprintf(stderr, "semctl() failed in ipc_finish(): %s\n",
					strerror(errno));
			return -1;
		}
	}

	return 0;
}


/*
 * ipc_poll() -
 *
 *	Check for incoming messages
 */
int
ipc_poll(bool blocking)
{
	int			rc;
	struct
	{
		long		mtype;
		char		mtext[MSGMAX];
	}			msg;

	while (true)
	{
		rc = msgrcv(msgid, &msg, sizeof(msg), 0,
					(blocking) ? 0 : IPC_NOWAIT);
		if (rc < 0)
		{
			if (errno == ENOMSG)
				return 0;

			fprintf(stderr, "msgrcv() failed in ipc_poll(): %s\n",
					strerror(errno));
			return -1;
		}

		if (msg.mtype == 2)
			shutdown_smart_requested = true;
		else if (msg.mtype == 3)
			shutdown_immed_requested = true;
		else if (msg.mtype == 4)
			wait_for_resume = false;
		else if (msg.mtype == 5)
			logfile_switch_requested = true;
		else if (ipc_add_path(msg.mtext) < 0)
			return -1;

		if (blocking)
			break;
	}

	return 0;
}


/*
 * ipc_add_path() -
 *
 *	Add an archive path to the sorted queue
 */
static int
ipc_add_path(char *path)
{
	queue_elem **elemp;
	queue_elem *elem;

	if ((elem = (queue_elem *) malloc(sizeof(queue_elem))) == NULL)
	{
		fprintf(stderr, "out of memory in ipc_add_path()\n");
		return -1;
	}
	if ((elem->archive_path = strdup(path)) == NULL)
	{
		fprintf(stderr, "out of memory in ipc_add_path()\n");
		return -1;
	}
	elem->next = NULL;

	/*
	 * See if we have to insert it in front of something else
	 */
	for (elemp = &archive_queue_head; *elemp != NULL; elemp = &((*elemp)->next))
	{
		if (strcmp(elem->archive_path, (*elemp)->archive_path) < 0)
		{
			elem->next = *elemp;
			*elemp = elem;
			return 0;
		}
	}


	if (archive_queue_head == NULL)
	{
		archive_queue_head = elem;
		archive_queue_tail = elem;
	}
	else
	{
		archive_queue_tail->next = elem;
		archive_queue_tail = elem;
	}

	return 0;
}


/*
 * ipc_send_path() -
 *
 *	Add an archive path to the daemons archive queue. Done by calling
 *	ipc_add_path() or sending the path to the daemon.
 */
int
ipc_send_path(char *logfname)
{
	struct
	{
		long		mtype;
		char		mtext[MSGMAX];
	}			msg;

	if (strlen(logfname) > (MSGMAX - 1))
	{
		fprintf(stderr, "path exceeds MSGMAX: %s\n", logfname);
		return -1;
	}

	/*
	 * As the creator, we are also the consumer, so we simply add the file to
	 * the queue.
	 */
	if (ipc_creator)
		return ipc_add_path(logfname);

	msg.mtype = 1;
	strcpy(msg.mtext, logfname);
	if (msgsnd(msgid, &msg, strlen(logfname) + 1, 0) < 0)
	{
		fprintf(stderr, "msgsnd() in ipc_send_path() failed: %s\n",
				strerror(errno));
		return -1;
	}

	return 0;
}


/*
 * ipc_recv_path()
 *
 *	Get the next archive file to process from the queue.
 */
int
ipc_recv_path(char *buf)
{
	queue_elem *elem;
	int			rc;
	struct
	{
		long		mtype;
		char		mtext[MSGMAX];
	}			msg;

	while (true)
	{
		if (ipc_poll(false) < 0)
		{
			ipc_finish(true);
			return -1;
		}

		/*
		 * If something requested an immediate shutdown, don't report any more
		 * logfiles back.
		 */
		if (shutdown_immed_requested)
		{
			ipc_finish(true);
			return 0;
		}

		/*
		 * If a smart shutdown was requested, try to close the queue but don't
		 * force it.
		 */
		if (shutdown_smart_requested)
		{
			if ((rc = ipc_finish(false)) == 0)
			{
				return 0;
			}
			if (rc < 0)
			{
				ipc_finish(true);
				return -1;
			}
		}

		/*
		 * If we have something in the queue, return that.
		 */
		if (archive_queue_head != NULL)
		{
			elem = archive_queue_head;
			archive_queue_head = archive_queue_head->next;
			if (archive_queue_head == NULL)
				archive_queue_tail = NULL;

			strcpy(buf, elem->archive_path);
			free(elem->archive_path);
			free(elem);
			return 1;
		}

		/*
		 * Receive one single message blocking for it.
		 */
		rc = msgrcv(msgid, &msg, sizeof(msg), 0, IPC_NOWAIT);
		if (rc < 0)
		{
			if (errno == ENOMSG)
				return -2;
			if (errno == EINTR)
				continue;

			fprintf(stderr, "msgrcv() failed in ipc_recv_path(): %s\n",
					strerror(errno));
			ipc_finish(true);
			return -1;
		}

		if (msg.mtype == 2)
			shutdown_smart_requested = true;
		else if (msg.mtype == 3)
			shutdown_immed_requested = true;
		else if (msg.mtype == 4)
			wait_for_resume = false;
		else if (msg.mtype == 5)
			logfile_switch_requested = true;
		else if (ipc_add_path(msg.mtext) < 0)
		{
			ipc_finish(true);
			return -1;
		}
	}
}


/*
 * ipc_send_term() -
 *
 *	Send a terminate request to the daemon.
 */
int
ipc_send_term(char *archive_dir, bool immediate)
{
	int			rc;

	rc = ipc_send_code(archive_dir, (immediate) ? 3 : 2);
	if (rc != 0)
		return rc;

	return ipc_wait_for_destroy();
}


/*
 * ipc_send_logswitch() -
 *
 *	Send a logswitch code to the daemon.
 */
int
ipc_send_logswitch(char *archive_dir)
{
	return ipc_send_code(archive_dir, 5);
}


/*
 * ipc_send_resume() -
 *
 *	Send a resume (after error) code to the daemon.
 */
int
ipc_send_resume(char *archive_dir)
{
	return ipc_send_code(archive_dir, 4);
}


/*
 * ipc_send_code() -
 *
 *	Support function for ipc_send_term() and ipc_send_resume().
 */
static int
ipc_send_code(char *archive_dir, int code)
{
	struct
	{
		long		mtype;
		char		mtext[1];
	}			msg;

	if (ipc_generate_keys(archive_dir) < 0)
		return -1;

	if ((semid = semget(semkey, 0, 0)) < 0)
	{
		if (!opt_quiet)
			fprintf(stderr, "no logshipper daemon running\n");
		return 2;
	}

	if (ipc_lock() < 0)
		return -1;

	if ((msgid = msgget(msgkey, 0)) < 0)
	{
		fprintf(stderr, "msgget() failed in ipc_send_code(): %s\n",
				strerror(errno));
		ipc_unlock();
		return -1;
	}

	msg.mtype = (long) code;
	if (msgsnd(msgid, &msg, 0, 0) < 0)
	{
		fprintf(stderr, "msgsnd() failed in ipc_send_code(): %s\n",
				strerror(errno));
		ipc_unlock();
		return -1;
	}

	return ipc_unlock();
}


/*
 * ipc_lock()
 *
 *	Lock the semaphore.
 */
int
ipc_lock(void)
{
	struct sembuf sops[1] = {{0, -1, 0}};

	if (semop(semid, sops, 1) < 0)
	{
		fprintf(stderr, "semop() failed in ipc_lock(): %s\n",
				strerror(errno));
		return -1;
	}
	return 0;
}


/*
 * ipc_unlock()
 *
 *	Unlock the semaphore.
 */
int
ipc_unlock(void)
{
	struct sembuf sops[1] = {{0, 1, 0}};

	if (semop(semid, sops, 1) < 0)
	{
		fprintf(stderr, "semop() failed in ipc_lock(): %s\n",
				strerror(errno));
		return -1;
	}
	return 0;
}


/*
 * ipc_wait_for_destroy()
 *
 *	Wait until the semaphore set is destroyed.
 */
int
ipc_wait_for_destroy(void)
{
	struct sembuf sops[1] = {{2, -1, 0}};

	if (semop(semid, sops, 1) < 0)
	{
		if (errno != EIDRM && errno != EINVAL)
		{
			fprintf(stderr, "semop() failed in ipc_wait_for_destroy(): %s\n",
					strerror(errno));
			return -1;
		}
	}
	return 0;
}


/*
 * ipc_cleanup() -
 *
 *	Destroy the semaphore set and message queue. Use with caution, this
 *	code does not check if there is a daemon running.
 */
int
ipc_cleanup(char *archive_dir)
{
	int			rc = 0;

	if (ipc_generate_keys(archive_dir) < 0)
		return -1;

	if ((semid = semget(semkey, 0, 0)) >= 0)
	{
		if (semctl(semid, 0, IPC_RMID) < 0)
		{
			fprintf(stderr, "semctl() failed in ipc_cleanup(): %s\n",
					strerror(errno));
			rc = -1;
		}
		else if (!opt_quiet)
			fprintf(stderr, "semaphore set removed\n");
		rc = 1;
	}
	else if (!opt_quiet)
		fprintf(stderr, "no semaphore set found\n");

	if ((msgid = msgget(msgkey, 0)) >= 0)
	{
		if (msgctl(msgid, IPC_RMID, NULL) < 0)
		{
			fprintf(stderr, "msgctl() failed in ipc_cleanup(): %s\n",
					strerror(errno));
			rc = -1;
		}
		else if (!opt_quiet)
			fprintf(stderr, "message queue removed\n");
		if (rc >= 0)
			rc |= 2;
	}
	else if (!opt_quiet)
		fprintf(stderr, "no message queue found\n");

	return rc;
}


/*
 * ipc_set_shutdown_smart() -
 *
 *	Put the queue into smart shutdown mode. This will cause
 *	ipc_recv_path() to return 0 once the queue is empty.
 */
void
ipc_set_shutdown_smart(void)
{
	shutdown_smart_requested = true;
}


/*
 * ipc_set_shutdown_immed() -
 *
 *	Put the queue into immediate shutdown mode. This will cause
 *	ipc_recv_path() to return 0 at the next call.
 */
void
ipc_set_shutdown_immed(void)
{
	shutdown_immed_requested = true;
}


/*
 * ipc_generate_keys() -
 *
 *	Generate the semkey and msgkey used for the Sys-V IPC objects.
 */
static int
ipc_generate_keys(char *archive_dir)
{
	if (ipc_archive_dir != NULL)
	{
		free(ipc_archive_dir);
		ipc_archive_dir = NULL;
	}

	/* ----
	 * Compute the two IPC keys used for the semaphore set and the
	 * message queue.
	 * ----
	 */
	semkey = ftok(archive_dir, 64);
	if (semkey < 0)
	{
		fprintf(stderr, "ftok(%s, 64): %s\n", archive_dir, strerror(errno));
		return -1;
	}
	msgkey = ftok(archive_dir, 65);
	if (msgkey < 0)
	{
		fprintf(stderr, "ftok(%s, 65): %s\n", archive_dir, strerror(errno));
		return -1;
	}

	ipc_archive_dir = strdup(archive_dir);
	if (ipc_archive_dir == NULL)
	{
		fprintf(stderr, "out of memory in ipc_generate_keys()\n");
		return -1;
	}

	return 0;
}


/*
 * ipc_sighandler()
 *
 *	Called on SIGINT and SIGTERM. Puts the daemon into immediate
 *	shutdown mode by sending ourself a -T message.
 */
static void
ipc_sighandler(int sig)
{
	struct
	{
		long		mtype;
		char		mtext[1];
	}			msg;

	msg.mtype = 3;
	msgsnd(msgid, &msg, 0, 0);
}
