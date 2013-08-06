

struct remote_wal_listener_state {
	int node;
	PGConn * dbconn;
	
};



/**
 * connect to the walsender and INIT a logical WAL slot
 *
 */
void init_wal_slot(struct  remote_wal_listener_state * state,SlonNode * node)
{
	char * query[1024];
	PGresult res;
	
	state->dbconn = slon_raw_connectdb(node->pa_conninfo);
	if ( state->dbconn == 0) 
	{
		/**
		 * connection failed, retry ?
		 */
	}
	snprintf(query,sizeof(query),"INIT_LOGICAL_REPLICATION \"%s\" \"%s\"",
			 node->no_id, "slony1_logical");
	res = PQexec(state->dbconn,query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slon_log(ERROR,"remoteWALListener_%d: could not send replication " \
				 "command \"%s\": %s",node->no_id, query, 
				 PQerrorMessage(conn));
	}
	if (PQntuples(res) != 1 || PQnfields(res) != 4)
	{
		slon_log(ERROR,
				 "remoteWALListener_%d: could not init logical rep: " \
				   "got %d rows and %d fields, expected %d rows and %d " \
				   "fields\n",
				 node->no_id, PQntuples(res), PQnfields(res), 1, 4);
	}
	if (sscanf(PQgetvalue(res, 0, 1), "%X/%X", &hi, &lo) != 2)
	{
		slon_log(ERROR,
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
}

/**
 * Establish the connection to the wal sender and START replication
 */
void start_wal(int node)
{
	

}

/**
 * The main loop
 * 
 * Read a single WAL record 
 *  If the WAL record is for a replicated table
 *          push the WAL record into the connection for that origin
 *  If the WAL record is for a EVENT (sl_event) hand the event over
 *      to the remote worker. The remote worker can then issue a commit
 *      on the transaction.  
 *	   
 */
void process_WAL(int node)
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

void 
