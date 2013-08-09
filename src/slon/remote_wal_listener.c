

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

#include<pg_config_manual.h>

#include <pthread.h>
#include "slon.h"

typedef struct  {
	PGconn * dbconn;
	uint64 startpos;

} SlonWALState;

static void init_wal_slot(SlonWALState * state,SlonNode * node);

static int extract_row_metadata(SlonNode * node,char * schema_name,
								char * table_name,
	                            char * xid_str,
								int * origin_id,
	                            char * row);

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
	state->startpos = ((uint64) hi) << 32 | lo;

	/**
	 * startpos needs to be stored in the local database.
	 * We will need to pass this to START REPLICATION.
	 * 
	 */
	
	slot = strdup(PQgetvalue(res, 0, 0));
	slon_log(SLON_INFO,"remoteWALListenerThread_%d: replication slot initialized %lld %s",node->no_id,state->startpos,slot);
	PQfinish(state->dbconn);
	state->dbconn=0;
}

/**
 * Establish the connection to the wal sender and START replication
 */
void start_wal(SlonNode * node, SlonWALState * state,const char * row)
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
	snprintf(query,sizeof(query),"START_LOGICAL_REPLICATION \"%d\" %X/%X ",
			 node->no_id, (uint32) ((state->startpos) >>32), 
			 (uint32)state->startpos);
	
	res = PQexec(state->dbconn,query);
	if (PQresultStatus(res) != PGRES_COPY_BOTH)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: could not send " \
				 "START REPLICATION command:%s %s",
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
int process_WAL(SlonNode * node, SlonWALState * state, char * row)
{
	
	/**
	 * variables we parse from the row
	 */
	 char schema_name[NAMEDATALEN];
	 char table_name[NAMEDATALEN];
	 char xid_str[64];
	SlonListen * listener;
	/**
	 * variables we lookup on the local node
	 */
	int origin_id;
	
	/**
	 * working variables;
	 */
	int rc;

	rc = extract_row_metadata(node,schema_name,table_name,xid_str,&origin_id
							  ,row);
	if( rc < 0 )
	{
		return rc;
	}
	
	/**
	 * check to see if origin_id is an origin that we are listening from
	 * if not we ignore this row.
	 */
	for(listener = node->listen_head; listener != NULL; 
		listener=listener->next) 
	{

		if( listener->li_origin == origin_id )
			break;

	}
	if(listener == NULL)
		return 0;

	/**
	 * are we subscribed to the set ?
	 * TODO: Figure out the best way of determing if we are
	 * subscribed to the set this table is in.  
	 * Options include:  1. Making the plugin dow this for us and
	 *                      filter out unsubscribed tables
	 *                   2. Lookup the set and subscription status
	 *                      from a in memory structure 
	 *                   3. Have the plugin pass the set_id along and then
	 *                      check the subscription status of this.
	 *                   4. Pass all rows to the apply trigger and let it filter
	 */
	
	if(strcmp(schema_name, rtcfg_namespace)==0 &&
	   strcmp(table_name,  "sl_event")==0)
	{
		/**
		 * sl_event updates need to be processed.
		 * TODO
		 *
		 * We need to parse this structure and turn it into
		 * data that can be passed to the remoteWorker_event function.
		 * This means parsing cmdargs.
		 *   1. I could manually write parse code
		 *   2. I could write a SET returning function in plpgsql that did
		 *      this, or invoke a select ARRAY_TO_SOMETHING
		 *   3. I could make the COPY stream be different from the 
		 *      walsender/plugin.
		 */
		
		
	}
	else if (strcmp(schema_name, rtcfg_namespace)==0 &&
			 strcmp(table_name,  "sl_confirm")==0)
	{
		/**
		 * confirms need to be processed ? 
		 * TODO ? Or can we just pass this to the apply trigger.
		 */
	}
	
	/**
	 * have we already processed this row? 
	 * We need a way to track this?
	 * One option might be to apply everything and then
	 * ROLLBACK instead of COMMIT on the next sl_event
	 * if it has already been applied.
	 */

	/**
	 * COPY the row to the connection for the origin.
	 * 1. Get the remoteWorker COPY connection
	 *    and push this row onto it
	 * 2. Release the connection back to the remoteWorker
	 */
	push_copy_row(node,origin_id,row);
	return 0;
	
}

static int extract_row_metadata(SlonNode * node,
								char * schema_name,
								char * table_name,
	                            char * xid_str,
								int * origin_id,								
	                            char * row)
{
	/**
	 * working/local usage variables.
	 */
	char * saveptr;

	char * field = strtok_r ( row, "," , &saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	*origin_id = strtol(field,NULL, 10 );
	field = strtok_r(NULL,",",&saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	strncpy(xid_str,field,64);
	field = strtok_r(NULL,",",&saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	/* ignore table id ? */
	field = strtok_r(NULL,",",&saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	/* ignore actionseq */
	field = strtok_r(NULL,",",&saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	strncpy(schema_name,field,NAMEDATALEN);
	field = strtok_r(NULL,",",&saveptr);
	if(field == NULL)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: parse error in row:%s"
				 , node->no_id, row);
		slon_retry();
	}
	strncpy(table_name,field,10);
	
	return 0;
	

}


static void push_copy_row(SlonNode * listening_node, int origin_id, char * row)
{
	SlonNode * workerNode;
	int active_log_table;
	SlonDString copy_in;
	PGresult   *res1;
	int rc;

	rtcfg_lock();
	workerNode = rtcfg_findNode(origin_id);
	if (workerNode == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWALListenerThread_%d: unknown origin %d",
				 listening_node->no_id, origin_id);

		return;
	}
	if (!listening_node->no_active)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWALListenerThread_%d: worker  %d is not active ",
				 listening_node->no_id, origin_id);
		return;
	}
	pthread_mutex_lock(&(workerNode->worker_con_lock));
	switch(workerNode->worker_con_status)
	{

	case SLON_WCON_IDLE:
	case SLON_WCON_INTXN:
		dstring_init(&copy_in);
		active_log_table = get_active_log_table(workerNode,
											workerNode->worker_dbconn);
		slon_mkquery(&copy_in, "COPY %s.\"sl_log_%d\" ( log_origin, "	\
				 "log_txid,log_tableid,log_actionseq,log_tablenspname, " \
				 "log_tablerelname, log_cmdtype, log_cmdupdncols," \
				 "log_cmdargs) FROM STDIN",
				 rtcfg_namespace, active_log_table);
		res1 = PQexec(workerNode->worker_dbconn,dstring_data(&copy_in));
		if(PQresultStatus(res1) != PGRES_COPY_IN)
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d_%d: error " \
					 "executing COPY IN:%s",listening_node->no_id,
					 origin_id, PQresultErrorMessage(res1));
			dstring_free(&copy_in);
			PQclear(res1);
			slon_retry();
		}
		PQclear(res1);
		dstring_free(&copy_in);
	case SLON_WCON_INCOPY:
		rc = PQputCopyData(workerNode->worker_dbconn, row, 0);
		if (rc < 0)
		{
			/**
			 * deal with the error
			 */
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d_%d: error writing"\
					 " to sl_log:%s",listening_node->no_id, origin_id,
					 PQerrorMessage(workerNode->worker_dbconn));
			
			slon_retry();
		}
		break;
		

	}				   
	
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
