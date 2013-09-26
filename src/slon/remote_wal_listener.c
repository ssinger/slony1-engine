

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#define POSTGRES_EPOCH_JDATE 2451545
#define UNIX_EPOCH_JDATE		2440588 /* == date2j(1970, 1, 1) */
#define SECS_PER_DAY	86400
#define USECS_PER_DAY	INT64CONST(86400000000)
#define USECS_PER_SEC	INT64CONST(1000000)

#include<pg_config_manual.h>

#include <pthread.h>
#include "slon.h"

struct SlonOriginList_t;
typedef struct  {
	PGconn * dbconn;
	XlogRecPtr last_committed_pos;
	pthread_mutex_t position_lock;
	struct SlonOriginList_t * origin_list;

} SlonWALState;



struct SlonOriginList_t {
	int no_id;
	XlogRecPtr last_event_lsn;
	struct SlonOriginList_t * next;
};

typedef struct SlonOriginList_t SlonOriginList;

struct SlonWALState_list_t;

struct SlonWALState_list_t {
	int no_id;
	SlonWALState * state;
	struct SlonWALState_list_t * next;

};


typedef struct SlonWALState_list_t SlonWALState_list;

static SlonWALState_list * state_list=NULL;
static pthread_mutex_t state_list_lock = PTHREAD_MUTEX_INITIALIZER;




static XlogRecPtr init_wal_slot(SlonWALState * state,SlonNode * node);




static void push_copy_row(SlonNode * listening_node, SlonWALState * state,
						  int origin_id, char * row, char * xid);

static void 
parseEvent(SlonNode * node, char * cmdargs,SlonWALState * state,XlogRecPtr walptr);

static void sendint64(int64 i, char *buf);
static int64 recvint64(char *buf);

static bool sendFeedback(SlonNode * node,
						 SlonWALState * state,
						 XlogRecPtr confirmedPos, int64 now, bool replyRequested);

static int process_WAL(SlonNode * node, SlonWALState * state, char * row,XlogRecPtr walptr);

static void start_wal(SlonNode * node, SlonWALState * state);

static int extract_row_metadata(SlonNode * node,
								char * schema_name,
								char * table_name,
	                            char * xid_str,
								int * origin_id,								
								char * operation,
								char ** cmdargs,
	                            char * row);


static bool xid_in_snapshot(SlonNode * node,
							char * xid_c,
							char * snapshot_p);

/**
 * connect to the walsender and INIT a logical WAL slot
 *
 */
static XlogRecPtr init_wal_slot(SlonWALState * state, SlonNode * node)
{
	char query[1024];
	PGresult * res;
	uint32 hi;
	uint32 lo;
	char * slot;
	char * conn_info;
	XlogRecPtr result;
	const char * replication = " replication=database";

	conn_info = malloc(strlen(node->pa_conninfo) + strlen(replication)+1);
	sprintf(conn_info,"%s %s",node->pa_conninfo,replication);
	state->dbconn = slon_raw_connectdb(conn_info);	
	free(conn_info);
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
	pthread_mutex_lock(&state->position_lock);

	state->last_committed_pos = ((uint64) hi) << 32 | lo;

	/**
	 * last_committed_pos needs to be stored in the local database.
	 * We will need to pass this to START REPLICATION.
	 * 
	 */
	slon_log(SLON_INFO,"remoteWALListenerThread_%d compare start is %s\n",node->no_id,PQgetvalue(res,0,1));
	slot = strdup(PQgetvalue(res, 0, 0));
	slon_log(SLON_INFO,"remoteWALListenerThread_%d: replication slot initialized %X/%X %s\n",node->no_id,
			 (uint32)(state->last_committed_pos>>32), (uint32)state->last_committed_pos,slot);

	/**
	 * update sl_node with the position returned.
	 */
	
	result = state->last_committed_pos;
	pthread_mutex_unlock(&state->position_lock);
	PQfinish(state->dbconn);
	state->dbconn=0;
	return result;
}

/**
 * Establish the connection to the wal sender and START replication
 */
static void start_wal(SlonNode * node, SlonWALState * state)
{
	char query[1024];
	PGresult * res;
	bool time_to_abort=false;
	int copy_res;
	char * conn_info;
	const char * replication = " replication=database";
	conn_info = malloc(strlen(node->pa_conninfo) + strlen(replication)+1);
	sprintf(conn_info,"%s %s",node->pa_conninfo,replication);
	state->dbconn = slon_raw_connectdb(conn_info);	

	state->dbconn = slon_raw_connectdb(conn_info);
	free(conn_info);
	if ( state->dbconn == 0) 
	{
		/**
		 * connection failed, retry ?
		 */
	}
	snprintf(query,sizeof(query),"START_LOGICAL_REPLICATION \"%d\" %X/%X (\"cluster\" '%s') ",
			 node->no_id, 
			 (uint32)(state->last_committed_pos>>32), 
			 (uint32)state->last_committed_pos,
			 rtcfg_cluster_name);


	slon_log(SLON_INFO,"remoteWALListenerThread_%d: %s",node->no_id,query);
	res = PQexec(state->dbconn,query);
	if (PQresultStatus(res) != PGRES_COPY_BOTH)
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: could not send " \
				 "START REPLICATION command:%s %s",
				 query, PQresultErrorMessage(res));
		PQclear(res);
		slon_retry();
	}

	slon_log(SLON_DEBUG2,"sent start logical replication");
	while(!time_to_abort)
	{
		char * copybuf;
		/**
		 * can we send feedback?
		 */
		
		/**
		 *  Read a row
		 */
		copy_res=PQgetCopyData(state->dbconn, &copybuf,0);		
		/**
		 * 
		 */
		if (copy_res > 0 )
		{
			if(copybuf[0] == 'k')
			{
				int64 position;
				/**
				 * keepalive message
				 */
				int64 now;
				struct timeval tp;
				gettimeofday(&tp,NULL);
				now  = (int64) (tp.tv_sec -
					((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
								* USECS_PER_SEC) + tp.tv_usec;
				pthread_mutex_lock(&state->position_lock);
				position = state->last_committed_pos;
				pthread_mutex_unlock(&state->position_lock);
				sendFeedback(node,state,position , now,false);

			}
			else if (copybuf[0]=='w')
			{
				size_t hdr_len;
				XlogRecPtr temp;
				XlogRecPtr position;
				int64 now;
				struct timeval tp;

				hdr_len = 1;			/* msgtype 'w' */
				hdr_len += 8;			/* dataStart */
				hdr_len += 8;			/* walEnd */
				hdr_len += 8;			/* sendTime */
				
				temp = recvint64(&copybuf[1]);

				slon_log(SLON_INFO,"remoteWALListenerThread_%d: %s\n",
						 node->no_id,
						 copybuf+hdr_len);

				
				process_WAL(node,state,copybuf+hdr_len,temp);
				/**
				 * process the row
				 */
				PQfreemem(copybuf);

				
				slon_log(SLON_INFO,
						 "remoteWALListenerThread_%d: new ptr is %X/%X\n", node->no_id,
						 (uint32)(temp>>32),(uint32)temp);

				
	
				gettimeofday(&tp,NULL);
				now  = (int64) (tp.tv_sec -
					((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
								* USECS_PER_SEC) + tp.tv_usec;
				pthread_mutex_lock(&state->position_lock);
				position = state->last_committed_pos;
				pthread_mutex_unlock(&state->position_lock);
				/**
				 * send feedback if required with the last confirmed position
				 * which should be BEFORE the position of the WAL just read.
				 * we don't confirm a record until the remoteWorkerThread has processed it.
				 */
				sendFeedback(node,state,position , 0,false);

			}			
		
		}
		else if (copy_res == 0)
		{

		}
		else if (copy_res == -1 )
		{
			/**
			 * copy finished.  What does this mean?
			 */
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: received -1 from COPY\n",node->no_id);
		}
		else 
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: an unknown error(%d)"\
					 " was received while reading data\n",node->no_id,copy_res);
			/**
			 * error ?
			 */
			slon_retry();
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
static int process_WAL(SlonNode * node, SlonWALState * state, char * row,XlogRecPtr walptr)
{
	
	/**
	 * variables we parse from the row
	 */
	 char schema_name[NAMEDATALEN];
	 char table_name[NAMEDATALEN];
	 char schema_name_quoted[NAMEDATALEN+2];
	 char xid_str[64];
	SlonListen * listener;
	/**
	 * variables we lookup on the local node
	 */
	int origin_id;
	
	char operation[2];
	
	char * cmdargs;

	/**
	 * working variables;
	 */
	int rc;
   
	rc = extract_row_metadata(node,schema_name,table_name,xid_str,&origin_id,
							  &operation,&cmdargs,row);
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
	sprintf(schema_name_quoted,"\"%s\"",schema_name);
	if( (strcmp(schema_name, rtcfg_namespace)==0 ||
		 strcmp(schema_name_quoted,rtcfg_namespace)==0)
	   &&
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
		parseEvent(node,cmdargs,state,walptr);
	}
	else if (strcmp(schema_name_quoted, rtcfg_namespace)==0 &&
			 strcmp(table_name,  "sl_confirm")==0)
	{
		/**
		 * confirms need to be processed ? 
		 * TODO ? Or can we just pass this to the apply trigger.
		 */
		push_copy_row(node,state,origin_id,row,xid_str);
	
	}
	else
	{
		/**
		 * We need the snapshot value from sl_setsync 
		 * we then need to compare the XID of this row against
		 * the snapshot in sl_setsync so we can decide if this
		 * row has already been processed.
		 *
		 * This exposes us to a race condition. 
		 *        sl_setsync won't be populated until the ENABLE_SUBSCRIPTION
		 *        event is finished (ie the copy_set) is finished. 
		 *  but we are making this decision now, ie too early.
		 * 
		 *  
		 */
		
		
		/**
		 * COPY the row to the connection for the origin.
		 * 1. Get the remoteWorker COPY connection
		 *    and push this row onto it
		 * 2. Release the connection back to the remoteWorker
		 */		
		push_copy_row(node,state,origin_id,row,xid_str);
	}
	return 0;
		
}

static void 
parseEvent(SlonNode * node, char * cmdargs,SlonWALState * state,
		   XlogRecPtr walptr)
{
	char * saveptr;
	char * column;
	char * value;
	size_t value_len;
	int64 ev_seqno=0;
	char *ev_timestamp=NULL;
	char *ev_snapshot=NULL;
	char *ev_mintxid=NULL;
	char *ev_maxtxid=NULL;
	char *ev_type=NULL;
	char *ev_data1=NULL; 
	char *ev_data2=NULL;
	char *ev_data3=NULL;
	char *ev_data4=NULL;
	char *ev_data5=NULL;
	char *ev_data6=NULL;
	char *ev_data7=NULL; 
	char *ev_data8=NULL;
	int ev_origin=0;
	SlonListen	* listenPtr=NULL;


	column=strtok_r(cmdargs+1,",",&saveptr);
	do
		 
	{

		value = strtok_r(NULL,",",&saveptr);
		if(value == NULL)
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: " \
					 "unexpected end of row encountered: last column %s : %s\n",
					 node->no_id,column , cmdargs);
			slon_retry();
		}	
		value_len = strlen(value);
		if(value[value_len-1]=='}')
		{
			value[value_len-1]='\0';
		}
		
		if(strcmp(value,"NULL") == 0)
		{
			value = NULL;
		}
		if(strcmp(column,"ev_origin")==0)
		{
			ev_origin = strtol(value,NULL,10);
		}
		else if (strcmp(column,"ev_seqno")==0)
		{
			ev_seqno=strtoll(value,NULL,10);
			if(ev_seqno == 0) 
				slon_log(SLON_ERROR,"error parsing sequence number\n");
		}
		else if (strcmp(column,"ev_timestamp")==0)
		{
			ev_timestamp = malloc(strlen(value)+1);
			strcpy(ev_timestamp,value);
		}
		else if (strcmp(column,"ev_snapshot")==0)
		{
			char * saveptr2;
			char * value2;

			ev_snapshot = malloc(strlen(value)+1);
			strcpy(ev_snapshot,value);
			value2 = strtok_r(value,":",&saveptr2);
			ev_mintxid = malloc(strlen(value2)+1);
			strcpy(ev_mintxid,value2);
			value2 = strtok_r(NULL,":",&saveptr2);
			ev_maxtxid = malloc(strlen(value2)+1);
			strcpy(ev_maxtxid,value2);
		
		}
		else if (strcmp(column,"ev_type")==0)
		{
			ev_type = malloc(strlen(value)+1);
			strcpy(ev_type,value);
		}
		else if (strcmp(column,"ev_data1")==0)
		{
			if(value != NULL) 
			{
				ev_data1 = malloc(strlen(value)+1);
				strcpy(ev_data1,value);
			}
			else
				ev_data1=NULL;
		}
		else if (strcmp(column,"ev_data2")==0)
		{
			if(value != NULL) 
			{
				ev_data2 = malloc(strlen(value)+1);
				strcpy(ev_data2,value);
			}
			else
				ev_data2=NULL;
		}
		else if (strcmp(column,"ev_data3")==0)
		{
			if(value != NULL)
			{
				ev_data3 = malloc(strlen(value)+1);
				strcpy(ev_data3,value);
			}
			else
				ev_data3=NULL;
		}
		else if (strcmp(column,"ev_data4")==0)
		{
			if(value != NULL)
			{
				ev_data4 = malloc(strlen(value)+1);
				strcpy(ev_data4,value);
			}
			else
				ev_data4=NULL;
		}
		else if(strcmp(column,"ev_data5")==0)
		{
			if(value != NULL)
			{
				ev_data5 = malloc(strlen(value)+1);
				strcpy(ev_data5,value);
			}
			else
				ev_data5=NULL;
			
		}
		else if (strcmp(column,"ev_data6")==0)
		{
			if(value != NULL)
			{
				ev_data6 = malloc(strlen(value)+1);
				strcpy(ev_data6, value);
			}
			else
				ev_data6 = NULL;
		}
		else if (strcmp(column,"ev_data7")==0)
		{
			if(value != NULL)
			{
				ev_data7 = malloc(strlen(value)+1);
				strcpy(ev_data7,value);
			}
			else
				ev_data7=NULL;
			
		}
		else if (strcmp(column,"ev_data8")==0)
		{
			if(value != NULL)
			{
				ev_data8 = malloc(strlen(value)+1);
				strcpy(ev_data8, value);
			}
			ev_data8 = NULL;
		}

		
		
	}while ( (column = strtok_r(NULL,",",&saveptr)) != NULL);
	
	slon_log(SLON_DEBUG2,"remoteWALListenerThread_%d: adding event %lld %d to queue snapshot %s \n", node->no_id,ev_seqno,ev_origin,ev_snapshot);

	/**
	 * do we listen for events from this origin via the provider?
	 */
	for(listenPtr = node->listen_head; listenPtr != NULL; listenPtr = listenPtr->next)
	{
		if (listenPtr->li_origin == ev_origin)
		{
			bool wal_established=false;

			/**
			 * make sure this is not the first event from the provider for the origin.
			 */
			SlonOriginList * origin=NULL;
			
			for(origin = state->origin_list; origin != NULL; origin = origin->next)
			{
				if (origin->no_id == ev_origin)
				{
					/**
					 * THIS provider has not yet seen an event from ev_origin.
					 * it might mean that there is an event in sl_event on the
					 * provider that has not caught in the WAL stream.
					 *
					 * 
					 */
					if ( origin->last_event_lsn > 0)
					{
						break;
					}
					wal_established=true;
					origin->last_event_lsn = walptr;
				}
			}
			if ( ! wal_established )
			{
				
				struct listat list;
				SlonConn * conn;
				char conn_string[32];
				
				if ( origin == NULL)
				{
					origin = malloc ( sizeof(SlonOriginList));
					memset(origin,0,sizeof(SlonOriginList));
					origin->no_id = ev_origin;
					if(state->origin_list == NULL)
					{
						state->origin_list = origin;
					}
					else
					{
						origin->next = state->origin_list;
						state->origin_list = origin;
					}
					origin->last_event_lsn = walptr;	   
				}

				memset(&list,0,sizeof(list));
				list.li_origin=ev_origin;
				snprintf(conn_string,32,"remote_wal_listener_%d.seed",node->no_id);
				conn = slon_connectdb(node->pa_conninfo,conn_string);
				remoteListen_receive_events(node, conn, &list,ev_seqno);
				slon_disconnectdb(conn);
			}

			remoteWorker_event(node->no_id,ev_origin,ev_seqno,
							   ev_timestamp,ev_snapshot,ev_mintxid,ev_maxtxid,
							   ev_type,ev_data1,ev_data2,ev_data3,ev_data4
							   ,ev_data5,ev_data6,ev_data7,ev_data8,true,walptr);
			break;
		}
	}


					   
	if(ev_timestamp != NULL)
		free(ev_timestamp);
	if(ev_snapshot != NULL)
		free(ev_snapshot);
	if(ev_mintxid != NULL)
		free(ev_mintxid);
	if(ev_maxtxid != NULL)
		free(ev_maxtxid);
	if(ev_type != NULL)
		free(ev_type);
	if(ev_data1 != NULL)
		free(ev_data1);
	if(ev_data2 != NULL)
		free(ev_data2);
	if(ev_data3 != NULL)
		free(ev_data3);
	if(ev_data4 != NULL)
		free(ev_data4);
	if(ev_data5 != NULL)
		free(ev_data5);
	if(ev_data6 != NULL)
		free(ev_data6);
	if(ev_data7 != NULL)
		free(ev_data7);
	if(ev_data8 != NULL)
		free(ev_data8);



}

static int extract_row_metadata(SlonNode * node,
								char * schema_name,
								char * table_name,
	                            char * xid_str,
								int * origin_id,								
								char * operation,
								char ** cmdargs,
	                            char * row)
{
	/**
	 * working/local usage variables.
	 */
	char * curptr;
	char * prev_start;
	bool start=true;
	char table_id[64];
	char action_seq[64];
	char cmdncols[64];
	char * field_array[] = {
		xid_str,
		table_id,
		action_seq,
		schema_name,
		table_name,
		operation,
		cmdncols,
		NULL

	};
	size_t field_idx=0;

	prev_start=row;
	for(curptr = row; *curptr != '\0' && field_array[field_idx] != NULL; curptr++)
	{
		if(*curptr == '\t')
		{
			if(start)
			{
				/**
				 * origin_id is the first column.
				 */
				char * tmp_buf = malloc(curptr - prev_start );
				strncpy(tmp_buf,prev_start, curptr-prev_start);
				*origin_id = strtol(tmp_buf,NULL, 10 );				
				free(tmp_buf);
				start=false;
			}
			else
			{
				/**
				 * a column from the array.
				 */ 
				strncpy(field_array[field_idx],prev_start,curptr - prev_start);
				field_array[field_idx][curptr-prev_start]='\0';
				field_idx++;		
			}
			prev_start = curptr+1;
			
		}

	}
		/**
		 * Copy the final cmdargs group if available.
		 */
		if(*curptr != '\0')
		{
			*cmdargs = malloc(strlen(curptr)+1);
			strcpy(*cmdargs,curptr);
		}
	
	return 0;
	

}


static void push_copy_row(SlonNode * listening_node, SlonWALState * state, int origin_id, char * row,
						  char * xid)
{
	SlonNode * workerNode;
	int active_log_table;
	SlonDString copy_in;
	PGresult * res1;
	size_t rowlen;

	int rc;
	rtcfg_lock();
	workerNode = rtcfg_findNode(origin_id);
	if (workerNode == NULL)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWALListenerThread_%d: unknown origin %d\n",
				 listening_node->no_id, origin_id);

		return;
	}
	if (!listening_node->no_active)
	{
		rtcfg_unlock();
		slon_log(SLON_WARN,
				 "remoteWALListenerThread_%d: worker  %d is not active\n",
				 listening_node->no_id, origin_id);
		return;
	}

	/**
	 * Wait until the message queue is empty.
	 * We do this so we can get a good guess at 
	 * the sl_setsync value to compare this row against
	 * to figure out if it has already been applied.
	 */
	pthread_mutex_lock(&(workerNode->message_lock));
	while ( workerNode->message_head != NULL)
	{
		struct timespec timeval;
		int64 now;
		struct timeval tp;		

		timeval.tv_sec = 0;
		timeval.tv_sec = 1000;
		pthread_cond_timedwait(&(workerNode->message_cond),&(workerNode->message_lock),&timeval);
	
		gettimeofday(&tp,NULL);
		now  = (int64) (tp.tv_sec -
						((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
						* USECS_PER_SEC) + tp.tv_usec;
		/** 
		 * have the lock.
		 * release it , send feedback and re-aquire it.
		 * we don't want to hold the lock while we send a network message
		 */
		pthread_mutex_unlock(&(workerNode->message_lock));
		sendFeedback(listening_node,state,state->last_committed_pos , now,false);
		pthread_mutex_lock(&(workerNode->message_lock));
		
	}

	/**
	 * at this stage the workerNode message queue is empty.
	 * Check to see if the XID for this row has already been processed
	 */

	if(  xid_in_snapshot(listening_node,xid,workerNode->last_snapshot))
	{
		/**
		 *
		 * This row has already been committed. We can ignore it
		 */
		 
		slon_log(SLON_DEBUG2,"remoteWALListenerThread_%d: ignoring row - already applied\n",
				 listening_node->no_id);
		pthread_mutex_unlock(&(workerNode->message_lock));
		return;
	}
	pthread_mutex_unlock(&(workerNode->message_lock));
	

	/**
	 * the problem we have is what if obtaining the mutex will 
	 * require blocking.  We don't want to be blocking for this lock
	 * so long that we don't send feedback to the WAL sender.
	 * This can happen if the remoteWorkerThread is in a copy.
	 * 
	 */
	while( pthread_mutex_trylock(&workerNode->worker_con_lock) != 0)
	{

		/**
		 * we could not get the lock.
		 * sendFeedback, sleep and try again.
		 */
		int64 now;
		struct timeval tp;		
		gettimeofday(&tp,NULL);
		now  = (int64) (tp.tv_sec -
						((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
						* USECS_PER_SEC) + tp.tv_usec;
		slon_log(SLON_INFO,"remoteWALListenerThread_%d: sending feedback while waiting on lock\n",
				 listening_node->no_id);
		sendFeedback(listening_node,state,state->last_committed_pos , now,false);
		sleep( 1 );
		
	}

	switch(workerNode->worker_con_status)
	{

	case SLON_WCON_IDLE:
	case SLON_WCON_INTXN:
		dstring_init(&copy_in);
		slon_log(SLON_DEBUG4,"remoteWALListenerThread_%d_%d: starting COPY\n",listening_node->no_id,workerNode->no_id);
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
					 "executing COPY IN:%s\n",listening_node->no_id,
					 origin_id, PQresultErrorMessage(res1 ));
			dstring_free(&copy_in);
			PQclear(res1);
			slon_retry();
		}
		PQclear(res1);
		dstring_free(&copy_in);
		workerNode->worker_con_status = SLON_WCON_INCOPY;
	case SLON_WCON_INCOPY:
		slon_log(SLON_DEBUG4,"remoteWALListenerThread_%d: COPY IN ROW: %s",listening_node->no_id,row);
		/**
		 * replace the NULL at the end of row (PQgetCopyData promises this to be true) 
		 * with a \n to mark the end of the row. 
		 */
		rowlen = strlen(row);
		row[rowlen] = '\n';
		rc = PQputCopyData(workerNode->worker_dbconn, row, rowlen+1);
		if (rc != 1)
		{
			/**
			 * deal with the error
			 */
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d_%d: error writing"\
					 " to sl_log:%s\n",listening_node->no_id, origin_id,
					 PQerrorMessage(workerNode->worker_dbconn));
			
			slon_retry();
		}
		break;
		

	}				   
	pthread_mutex_unlock(&(workerNode->worker_con_lock));
	rtcfg_unlock();	
	
	
}





void *
remoteWALListenThread_main(void *cdata)
{
	
	/**
	 * node is the Node structure for the provider
	 */
	SlonNode * node = (SlonNode*) cdata;
	SlonWALState state;
	SlonWALState_list * statePtr;
	SlonWALState_list * prevStatePtr;
	SlonDString query;
	SlonConn * conn;
	PGresult * res1;


	slon_log(SLON_INFO,
			 "remoteWALListenThread_%d: thread starts\n",
			 node->no_id);
	pthread_mutex_lock(&state_list_lock);
	
	if (state_list == NULL)
	{
		state_list = malloc(sizeof(SlonWALState_list));
		memset(state_list,0,sizeof(SlonWALState_list));
		state_list->no_id = node->no_id;
		statePtr = state_list;
	}
	else
	{
		for(statePtr = state_list; statePtr->next != NULL; statePtr = statePtr->next);
		statePtr->next = malloc(sizeof(SlonWALState_list));
		memset(state_list,0,sizeof(SlonWALState_list));
		statePtr->next->no_id=node->no_id;
		statePtr = statePtr->next;
	}
	statePtr->state=&state;
	pthread_mutex_unlock(&state_list_lock);

	pthread_mutex_init(&state.position_lock,0);

	/**
	 * check if this slot has been established on the remote end
	 */
	
	dstring_init(&query);
	slon_mkquery(&query,"select no_last_xlog_rec from %s.sl_node where no_id=%d",
				 rtcfg_namespace,node->no_id);

	/**
	 * connect to the LOCAL database to query the position(if any) of the replica.
	 */
	if ((conn = slon_connectdb(rtcfg_conninfo, "remote_wal_listener.local")) == NULL)
		slon_retry();
	res1 = PQexec(conn->dbconn,dstring_data(&query));
	if ( PQresultStatus(res1) != PGRES_TUPLES_OK) {
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: %s - %s\n",node->no_id,
				 dstring_data(&query),PQresultErrorMessage(res1));
		slon_retry();
	}
	if( PQntuples(res1) > 0 && PQgetisnull(res1,0,0)==false)
	{	
		uint32 hi;
		uint32 lo;
		char * last_xlog;

		last_xlog = PQgetvalue(res1,0,0);
		if(sscanf(last_xlog,"%X/%X",&hi,&lo) != 2)
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error parsing WAL position %s",
					 node->no_id,last_xlog);
			slon_retry();
		}
		state.last_committed_pos = ((uint64) hi<<32) | lo;
	}
	else
	{
		PGresult * res2;
		XlogRecPtr loc = init_wal_slot(&state,node);
		
		dstring_reset(&query);
		slon_mkquery(&query,"update %s.sl_node set no_last_xlog_rec = '%X/%X' where no_id=%d",
					 rtcfg_namespace, (uint32)(loc >>32), (uint32)loc, node->no_id);
		res2 = PQexec(conn->dbconn, dstring_data(&query));
		if(PQresultStatus(res2) != PGRES_COMMAND_OK) 
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error updating node position\n",
					 node->no_id);
			slon_retry();
		}
		PQclear(res2);
	}
	PQclear(res1);
	dstring_terminate(&query);
	slon_disconnectdb(conn);


	
	start_wal(node,&state);

	/**
	 * the thread is complete.  Remove it from the state list.
	 */
	pthread_mutex_lock(&state_list_lock);	
	prevStatePtr = state_list;
	if(state_list != NULL && state_list->state == &state)
	{
		/*
		 * first item in list
		 */
		statePtr = state_list;
		state_list = statePtr->next;
		free(statePtr);
	}
	else
	{
		for(statePtr = state_list; statePtr != NULL && statePtr->next != NULL ; statePtr = statePtr->next)
		{
			if (statePtr->next->state == &state)
				break;
		}
		
		if(statePtr != NULL &&  statePtr->next == NULL && statePtr->next->state == &state)
		{
			prevStatePtr = statePtr->next->next;
			free(statePtr->next);
			statePtr->next = prevStatePtr;
		}
			
	}
	pthread_mutex_destroy(&state.position_lock);
	pthread_mutex_unlock(&state_list_lock);
	return 0;
}


static bool
sendFeedback(SlonNode * node,
			 SlonWALState * state,
			  XlogRecPtr blockpos, int64 now, bool replyRequested)
{
	char		replybuf[1 + 8 + 8 + 8 + 8 + 1];
	int			len = 0;

#if 0 
	if (blockpos == state->startpos)
		return true;
#endif
	slon_log(SLON_DEBUG2, "remoteWALListenerThread_%d: sending feedback %X/%X\n",node->no_id,
			 ((uint32)(blockpos>>32)),(uint32)blockpos);
	replybuf[len] = 'r';
	len += 1;
	sendint64(blockpos, &replybuf[len]);		/* write */
	len += 8;
	sendint64(blockpos, &replybuf[len]);		/* flush */
	len += 8;
	sendint64(0, &replybuf[len]);		/* apply */
	len += 8;
	sendint64(now, &replybuf[len]);		/* sendTime */
	len += 8;
	replybuf[len] = replyRequested ? 1 : 0;		/* replyRequested */
	len += 1;

	if (PQputCopyData(state->dbconn, replybuf, len) <= 0 || PQflush(state->dbconn))
	{
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error sending feedback",node->no_id);
		return false;
	}

	return true;
}

/*
 * Converts an int64 to network byte order.
 */
static void
sendint64(int64 i, char *buf)
{
	uint32		n32;

	/* High order half first, since we're doing MSB-first */
	n32 = (uint32) (i >> 32);
	n32 = htonl(n32);
	memcpy(&buf[0], &n32, 4);

	/* Now the low order half */
	n32 = (uint32) i;
	n32 = htonl(n32);
	memcpy(&buf[4], &n32, 4);
}
/*
 * Converts an int64 from network byte order to native format.
 */
static int64
recvint64(char *buf)
{
	int64		result;
	uint32		h32;
	uint32		l32;

	memcpy(&h32, buf, 4);
	memcpy(&l32, buf + 4, 4);
	h32 = ntohl(h32);
	l32 = ntohl(l32);

	result = h32;
	result <<= 32;
	result |= l32;

	return result;
}

void remote_wal_processed(XlogRecPtr confirmed, int no_id)
{
	SlonWALState_list * statePtr;

	pthread_mutex_lock(&state_list_lock);
	for(statePtr = state_list; statePtr != NULL; statePtr = statePtr->next)
	{
		if (no_id == statePtr->no_id)
			break;
		
	}
	if ( statePtr == 0)
	{
		slon_log(SLON_ERROR,"remoteWALListener_%d thread was not found",no_id);
		slon_retry();
	}
	slon_log(SLON_DEBUG2,"remoteWorkerThread_%d processed until %X/%X\n",no_id, (uint32) (confirmed>>32),(uint32)confirmed);
	pthread_mutex_lock(&statePtr->state->position_lock);
	statePtr->state->last_committed_pos=confirmed;
	pthread_mutex_unlock(&statePtr->state->position_lock);
	pthread_mutex_unlock(&state_list_lock);
}

/**
 * is the XID committed at the time of the snaphsot.
 * 
 *  snapshot is of the form   minxip:maxxipactive_xips
 *  
 *  If xid < minxip the transaction is committed
 *  If xid > maxip The transaction is not committed
 *  If xid is listed in the active list then it is not commited
 *  else it is committed
 */
static bool xid_in_snapshot(SlonNode * node,
							char * xid_c,
							char * snapshot_p)
{
	char * state;
	char * minxid_c;
	char * ip_list;
	char * maxxid_c;
	char * elem;
	int64 xid;
	int64 minxid;
	int64 maxxid;
	char * snapshot;

	/**
	 * copy snapshot so strtok doesn't change it
	 */
	snapshot = malloc(strlen(snapshot_p)+1);
	strcpy(snapshot,snapshot_p);
	

	minxid_c = strtok_r(snapshot,":",&state);
	if ( minxid_c == NULL)
	{
		/**
		 * parse error
		 */
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error parsing snapshot %s",node->no_id,snapshot);
		slon_retry();
	}
	
	xid = strtoll(xid_c,NULL,10);
	minxid = strtoll(minxid_c,NULL,10);
	if ( xid < minxid )
	{
		free(snapshot);
		return true;
	}

	maxxid_c = strtok_r(NULL,":", &state);
	if (maxxid_c == NULL)
	{
		/**
		 * parse error
		 */
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error parsing snapshot %s",node->no_id,snapshot);
		slon_retry();
	}
	maxxid = strtoll(maxxid_c,NULL,10);
	if ( xid > maxxid ) {
		free(snapshot);
		return false;
	}
	

	ip_list = strtok_r(NULL,":",&state);
	
	if ( ip_list == NULL) 
	{
		/**
		 * parse error
		 */
		slon_log(SLON_ERROR,"remoteWALListenerThread_%d: error parsing snapshot %s",node->no_id,snapshot);
		slon_retry();
	}

	for( elem = strtok_r(ip_list,",",&state); elem != NULL; elem = strtok_r(NULL,",",&state))
	{
		if (strcmp(elem,xid_c)==0)
		{
			/**
			 * this transaction is in progress.
			 */
			free(snapshot);
			return false;
		}
		
	}
	free(snapshot);
	return true;
}
