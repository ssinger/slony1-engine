

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif


#include <pthread.h>
#include "slon.h"

typedef struct  {
	PGconn * dbconn;

} SlonWALState;

static void init_wal_slot(SlonWALState * state,SlonNode * node);

/**
 * connect to the walsender and INIT a logical WAL slot
 *
 */
static void init_wal_slot(SlonWALState * state, SlonNode * node)
{
	char query[1024];
	PGresult * res;
	uint32 hi;
	uint32 lo;
	uint64 startpos;
	char * slot;

	state->dbconn = slon_raw_connectdb(node->pa_conninfo);
	if ( state->dbconn == 0) 
	{
		/**
		 * connection failed, retry ?
		 */
	}
	snprintf(query,sizeof(query),"INIT_LOGICAL_REPLICATION \"%d\" \"%s\"",
			 node->no_id, "slony1_funcs.2.2.0.b2");
	res = PQexec(state->dbconn,query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(SLON_ERROR,"remoteWALListener_%d: could not send replication " \
				 "command \"%s\": %s",node->no_id, query, 
				 PQerrorMessage(state->dbconn));
	}
	if (PQntuples(res) != 1 || PQnfields(res) != 4)
	{
		slon_log(SLON_ERROR,
				 "remoteWALListener_%d: could not init logical rep: " \
				   "got %d rows and %d fields, expected %d rows and %d " \
				   "fields\n",
				 node->no_id, PQntuples(res), PQnfields(res), 1, 4);
	}
	if (sscanf(PQgetvalue(res, 0, 1), "%X/%X", &hi, &lo) != 2)
	{
		slon_log(SLON_ERROR,
				 "remoteWALListener_%d: could not parse log location " \
				   "\"%s\"\n",
				 node->no_id, PQgetvalue(res, 0, 1));
	}
	startpos = ((uint64) hi) << 32 | lo;

	/**
	 * startpos needs to be stored in the local database.
	 * We will need to pass this to START REPLICATION.
	 * 
	 */
	
	slot = strdup(PQgetvalue(res, 0, 0));
	slon_log(SLON_INFO,"remoteWALListenerThread_%d: replication slot initialized %lld %s",node->no_id,startpos,slot);
	PGfinish(state->dbconn);
	state->dbconn=0;
}

/**
 * Establish the connection to the wal sender and START replication
 */
void start_wal(int node)
{
	char query[1024];
	PGresult * res;
	bool time_to_abort=false;
	int copy_res;


	state->dbconn = slon_raw_connectdb(node->pa_conninfo);
	if ( state->dbconn == 0) 
	{
		/**
		 * connection failed, retry ?
		 */
	}
	snprintf(query,sizeof(query),"START_LOGICAL_REPLICATION \"%d\" %X/%X ()"
			 node->no_id, (uint32) (startpos>>32), uint32(startpos));
	
	res = PQexec(state->dbconn,query);
	if (PQresultStatus(res) != PGRES_COPY_BOTH)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: could not send " \
				 "START REPLICATION command:%s %s"
				 query, PQresultErrorMessage(res));
		PQclear(res);
		slon_retry();
	}
	PQclear(res);

	while(!time_to_abort)
	{
		char * copybuf;
		/**
		 * can we send feedback?
		 */
		
		
		/**
		 *  Read a row
		 */
		copy_res=PQgetCopyData(state->dbconn, &copybuf,1);
		
		/**
		 * 
		 */
		if (copy_res > 0 )
		{
			/**
			 * process the row
			 */
			
			
		}
		else if (copy_res == 0)
		{
			/**
			 *
			 * nothing available
			 */
		}
		else if (copy_res == -1 )
		{
			/**
			 * copy finished.  What does this mean?
			 */
		}
		else 
		{
			/**
			 * error ?
			 */
		}
	}


}

/**
 * process a single WAL record 
 *  If the WAL record is for a replicated table
 *          push the WAL record into the connection for that origin
 *  If the WAL record is for a EVENT (sl_event) hand the event over
 *      to the remote worker. The remote worker can then issue a commit
 *      on the transaction.  
 *	   
 */
void process_WAL(SlonNode * node, SlonWALState * state,char * row)
{
	
}

/**
 *  tell the WAL sender that we have received up until a certain XLOG
 *  id. We can confirm a XLOG segment when all remote worker DB connections
 *      have committed at least up until that segment.
 */
void ack_WAL(int node)
{
	
	
}



void *
remoteWALListenThread_main(void *cdata)
{
	
	SlonNode * node = (SlonNode*) cdata;
	SlonWALState state;

	
	slon_log(SLON_INFO,
			 "remoteWALListenThread_%d: thread starts\n",
			 node->no_id);
	init_wal_slot(&state,node);
	return 0;
}
