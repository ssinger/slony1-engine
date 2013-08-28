

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
#define POSTGRES_EPOCH_JDATE 2451545
#define UNIX_EPOCH_JDATE		2440588 /* == date2j(1970, 1, 1) */
#define SECS_PER_DAY	86400
#define USECS_PER_DAY	INT64CONST(86400000000)
#define USECS_PER_SEC	INT64CONST(1000000)

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
								char * operation,
								char ** cmdargs,
	                            char * row);



static void push_copy_row(SlonNode * listening_node, int origin_id, char * row);

static void 
parseEvent(SlonNode * node, char * cmdargs);

static void sendint64(int64 i, char *buf);
static int64 recvint64(char *buf);

static bool sendFeedback(SlonNode * node,
						 SlonWALState * state,
						 uint64 blockpos, int64 now, bool replyRequested);
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
void start_wal(SlonNode * node, SlonWALState * state)
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
	snprintf(query,sizeof(query),"START_LOGICAL_REPLICATION \"%d\" %X/%X (\"cluster\" 'test') ",
			 node->no_id, 
#if 0 
(uint32) ((state->startpos) >>32), 
			 (uint32)state->startpos,
rtcfg_namespace);
#else
	0,0);
#endif

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
				/**
				 * keepalive message
				 */
				int64 now;
				struct timeval tp;
				gettimeofday(&tp,NULL);
				now  = (int64) (tp.tv_sec -
					((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
								* USECS_PER_SEC) + tp.tv_usec;

				sendFeedback(node,state,state->startpos , now,false);
			}
			else if (copybuf[0]=='w')
			{
				size_t hdr_len;
				int64 temp;
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

				/**
				 * process the row
				 */
				
				PQfreemem(copybuf);
				
				slon_log(SLON_INFO,
						 "remoteWALListenerThread_%d: new ptr is %lld",temp);

				
	
				gettimeofday(&tp,NULL);
				now  = (int64) (tp.tv_sec -
					((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY)
								* USECS_PER_SEC) + tp.tv_usec;
				sendFeedback(node,state,temp , 0,false);
				state->startpos=temp;
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
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: received -1 from COPY?",node->no_id);
		}
		else 
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: an unknown error"\
					 " was received while reading data",node->no_id);
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
	
	char operation;
	
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
		parseEvent(node,cmdargs);
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

static void 
parseEvent(SlonNode * node, char * cmdargs)
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

	for( (column=strtok_r(cmdargs+1,",",&saveptr)); 
		 (column = strtok_r(NULL,",",&saveptr)) != NULL; )
	{
		
		value = strtok_r(NULL,",",&saveptr);
		if(value == NULL)
		{
			slon_log(SLON_ERROR,"remoteWALListenerThread_%d: " \
					 "unexpected end of row encountered: %s",
					 node->no_id,cmdargs);
			slon_retry();
		}
		value_len = strlen(value);
		if(value[value_len]=='}')
		{
			value[value_len]=NULL;
		}
		if(strcmp(column,"ev_origin")==0)
		{
			ev_origin = strtol(value,NULL,10);
		}
		else if (strcmp(column,"ev_seqno")==0)
		{
			ev_seqno=strtoll(value,NULL,10);
		}
		else if (strcmp(column,"ev_timestamp")==0)
		{
			ev_timestamp = malloc(strlen(value));
			strncpy(ev_timestamp,value,strlen(value));
		}
		else if (strcmp(column,"ev_snapshot")==0)
		{
			char * saveptr2;
			char * value2 = strtok_r(value,":",&saveptr2);
			ev_mintxid = malloc(strlen(value2));
			strncpy(ev_mintxid,value2,strlen(value2));
			value2 = strtok_r(NULL,":",&saveptr2);
			ev_maxtxid = malloc(strlen(value2));
			strncpy(ev_maxtxid,value2,strlen(value2));
			ev_snapshot = malloc(strlen(value));
			strncpy(ev_snapshot,value,strlen(value));
		}
		else if (strcmp(column,"ev_type")==0)
		{
			ev_type = malloc(strlen(value));
			strncpy(ev_type,value,strlen(value));
		}
		else if (strcmp(column,"ev_data1")==0)
		{
			ev_data1 = malloc(strlen(value));
			strncpy(ev_data1,value,strlen(value));
		}
		else if (strcmp(column,"ev_data2")==0)
		{
			ev_data2 = malloc(strlen(value));
			strncpy(ev_data2,value,strlen(value));
		}
		else if (strcmp(column,"ev_data3")==0)
		{
			ev_data3 = malloc(strlen(value));
			strncpy(ev_data3,value,strlen(value));
		}
		else if (strcmp(column,"ev_data4")==0)
		{
			ev_data4 = malloc(strlen(value));
			strncpy(ev_data4,value,strlen(value));
		}
		else if(strcmp(column,"ev_data5")==0)
		{
			ev_data5 = malloc(strlen(value));
			strncpy(ev_data5,value,strlen(value));
			
		}
		else if (strcmp(column,"ev_data6")==0)
		{
			ev_data6 = malloc(strlen(value));
			strncpy(ev_data6, value,strlen(value));
		}
		else if (strcmp(column,"ev_data7")==0)
		{
			ev_data7 = malloc(strlen(value));
			strncpy(ev_data7,value,strlen(value));
			
		}
		else if (strcmp(column,"ev_data8")==0)
		{
			ev_data8 = malloc(strlen(value));
			strncpy(ev_data8, value, strlen(value));
		}

		
		
	}


	remoteWorker_event(node->no_id,ev_origin,ev_seqno,
					   ev_timestamp,ev_snapshot,ev_mintxid,ev_maxtxid,
					   ev_type,ev_data1,ev_data2,ev_data3,ev_data4
					   ,ev_data5,ev_data6,ev_data7,ev_data8);
					   
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
	field = strtok_r(NULL,",",&saveptr);
	*operation = field[0];
   
	/**
	 * skip cmdupdncols
	 */
	field = strtok_r(NULL,",",&saveptr);
	
	field = strtok_r(NULL,",",&saveptr);
	*cmdargs = malloc(strlen(field));
	strncpy(*cmdargs,field,strlen(field));
	


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
	start_wal(node,&state);
	return 0;
}


static bool
sendFeedback(SlonNode * node,
			 SlonWALState * state,
			  uint64 blockpos, int64 now, bool replyRequested)
{
	char		replybuf[1 + 8 + 8 + 8 + 8 + 1];
	int			len = 0;

	if (blockpos == state->startpos)
		return true;

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

	state->startpos = blockpos;
	slon_log(SLON_INFO,"sending feedback");
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
