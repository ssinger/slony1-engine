/* ----------------------------------------------------------------------
 * slony1_funcs.c
 *
 *	  The C functions and triggers portion of Slony-I.
 *
 * Copyright (c) 2003, PostgreSQL Global Development Group
 * Author: Jan Wieck, Afilias USA LLC.
 *
 *	$Id: slony1_funcs.c,v 1.1 2003-11-28 14:59:44 wieck Exp $
 * ----------------------------------------------------------------------
 */

#include "postgres.h"

#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "executor/spi.h"
#include "access/xact.h"


PG_FUNCTION_INFO_V1(_Slony_I_createEvent);
PG_FUNCTION_INFO_V1(_Slony_I_getLocalNodeId);

Datum           _Slony_I_createEvent(PG_FUNCTION_ARGS);
Datum           _Slony_I_getLocalNodeId(PG_FUNCTION_ARGS);


/* ----
 * Slony_I_ClusterStatus -
 *
 *	The per-cluster data to hold for functions and triggers.
 * ----
 */
typedef struct slony_I_cluster_status {
	NameData		clustername;
	char		   *clusterident;
	int32			localNodeId;
	TransactionId	currentXid;

	void		   *plan_insert_event;
	void		   *plan_notify_event;

	struct slony_I_cluster_status *next;
} Slony_I_ClusterStatus;


static Slony_I_ClusterStatus   *clusterStatusList = NULL;
static Slony_I_ClusterStatus   *getClusterStatus(Name cluster_name);


Datum
_Slony_I_createEvent(PG_FUNCTION_ARGS)
{
	TransactionId	newXid = GetCurrentTransactionId();
	Slony_I_ClusterStatus *cs;
	text	   *ev_xip;
	Datum		argv[12];
	char		nulls[13];
	char	   *buf;
	size_t		buf_size;
	int			rc;
	int			xcnt;
	char	   *cp;
	int			i;

	if (SerializableSnapshot == NULL)
		elog(ERROR, "Slony-I: SerializableSnapshot is NULL in createEvent()");

	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in createEvent()");

	cs = getClusterStatus(PG_GETARG_NAME(0));

	buf_size = 8192;
	buf = palloc(buf_size);

	/*
	 * Do the following only once per transaction.
	 */
	if (!TransactionIdEquals(cs->currentXid, newXid))
	{
		/*
		 * On first call create the saved plan for event insertion
		 * and notification.
		 */
		if (cs->plan_insert_event == NULL)
		{
			Oid			plan_types[12];
			Oid			xxid_typid;
			TypeName   *xxid_typename;

			/*
			 * Prepare and save the plan to create the event notification
			 */
			sprintf(buf, "NOTIFY %s_event;", NameStr(cs->clustername));
			cs->plan_notify_event = SPI_saveplan(SPI_prepare(buf, 0, NULL));
			if (cs->plan_notify_event == NULL)
				elog(ERROR, "Slony-I: SPI_prepare() failed");

			/*
			 * Lookup the oid of our special xxid type
			 */
			xxid_typename = makeNode(TypeName);
			xxid_typename->names = 
				lappend(lappend(NIL, makeString(NameStr(cs->clustername))),
									 makeString("xxid"));
			xxid_typid = typenameTypeId(xxid_typename);

			/*
			 * Prepare and save the INSERT plan
			 */
			sprintf(buf, "INSERT INTO %s.sl_event "
				"(ev_origin, ev_seqno, "
				"ev_timestamp, ev_minxid, ev_maxxid, ev_xip, "
				"ev_type, ev_data1, ev_data2, ev_data3, ev_data4, "
				"ev_data5, ev_data6, ev_data7, ev_data8) "
				"VALUES ('%d', nextval('%s.sl_event_seq'), "
				"now(), $1, $2, $3,
				$4, $5, $6, $7, $8, $9, $10, $11, $12);", 
				cs->clusterident, cs->localNodeId, cs->clusterident);
			plan_types[0] = xxid_typid;
			plan_types[1] = xxid_typid;
			plan_types[2] = TEXTOID;
			plan_types[3] = TEXTOID;
			plan_types[4] = TEXTOID;
			plan_types[5] = TEXTOID;
			plan_types[6] = TEXTOID;
			plan_types[7] = TEXTOID;
			plan_types[8] = TEXTOID;
			plan_types[9] = TEXTOID;
			plan_types[10] = TEXTOID;
			plan_types[11] = TEXTOID;
			
			cs->plan_insert_event = SPI_saveplan(SPI_prepare(buf, 12, plan_types));
			if (cs->plan_insert_event == NULL)
				elog(ERROR, "Slony-I: SPI_prepare() failed");
		}

		/*
		 * Once per transaction notify on the sl_event relation
		 */
		if ((rc = SPI_execp(cs->plan_notify_event, NULL, NULL, 0)) < 0)
			elog(ERROR, "Slony-I: SPI_execp() failed for \"NOTIFY event\"");

		cs->currentXid = newXid;
	}

	/*
	 * Build the comma separated list of transactions in progress
	 * as Text datum.
	 */
	*(cp = buf) = '\0';
	for (xcnt = 0; xcnt < SerializableSnapshot->xcnt; xcnt++)
	{
		if ((cp + 30) >= (buf + buf_size))
		{
			buf_size *= 2;
			buf = repalloc(buf, buf_size);
			cp = buf+ strlen(buf);
		}
		sprintf(cp, "%s'%u'", (xcnt > 0) ? "," : "",
				SerializableSnapshot->xip[xcnt]);
		cp += strlen(cp);
	}
	ev_xip = DatumGetTextP(DirectFunctionCall1(textin, PointerGetDatum(buf)));

	/*
	 * Call the saved INSERT plan
	 */
	argv[0] = TransactionIdGetDatum(SerializableSnapshot->xmin);
	argv[1] = TransactionIdGetDatum(SerializableSnapshot->xmax);
	argv[2] = PointerGetDatum(ev_xip);
	nulls[0] = ' ';
	nulls[1] = ' ';
	nulls[2] = ' ';
	for (i = 1; i < 10; i++)
	{
		if (i >= PG_NARGS() || PG_ARGISNULL(i))
		{
			argv[i + 2] = (Datum)0;
			nulls[i + 2] = 'n';
		}
		else
		{
			argv[i + 2] = PG_GETARG_DATUM(i);
			nulls[i + 2] = ' ';
		}
	}
	nulls[12] = '\0';

	if ((rc = SPI_execp(cs->plan_insert_event, argv, nulls, 0)) < 0)
		elog(ERROR, "Slony-I: SPI_execp() failed for \"INSERT INTO sl_event ...\"");

	SPI_finish();

	PG_RETURN_INT32(0);
}


/*
 * _Slony_I_getLocalNodeId -
 *    
 *    SQL callable wrapper for calling getLocalNodeId() in order
 *    to get the current setting of sequence sl_local_node_id with
 *    configuration check.
 *
 */
Datum
_Slony_I_getLocalNodeId(PG_FUNCTION_ARGS)
{
	Slony_I_ClusterStatus *cs;
	int		rc;

	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in createEvent()");

	cs = getClusterStatus(PG_GETARG_NAME(0));

	SPI_finish();

	PG_RETURN_INT32(cs->localNodeId);
}


static Slony_I_ClusterStatus *
getClusterStatus(Name cluster_name)
{
	Slony_I_ClusterStatus  *status;
	int						rc;
	char					query[1024];
	bool					isnull;

	/*
	 * Find an existing status row for this cluster
	 */
	for (status = clusterStatusList; status; status = status->next)
	{
		if ((bool)DirectFunctionCall2(nameeq, 
				NameGetDatum(&(status->clustername)),
				NameGetDatum(cluster_name)) == true)
			return status;
	}

	/*
	 * No existing status found ... create a new one
	 */
	status = (Slony_I_ClusterStatus *)malloc(sizeof(Slony_I_ClusterStatus));
	memset(status, 0, sizeof(Slony_I_ClusterStatus));

	/*
	 * We remember the plain cluster name for fast lookup
	 */
	strncpy(NameStr(status->clustername), NameStr(*cluster_name), NAMEDATALEN);

	/*
	 * ... and the quoted identifier of it for building queries
	 */
	status->clusterident = strdup(DatumGetCString(DirectFunctionCall1(textout,
			DirectFunctionCall1(quote_ident,
			DirectFunctionCall1(textin, CStringGetDatum(NameStr(*cluster_name)))))));

	/*
	 * Get our local node ID
	 */
	snprintf(query, 1024, "select last_value::int4 from %s.sl_local_node_id",
		status->clusterident);
	rc = SPI_exec(query, 0);
	if (rc < 0 || SPI_processed != 1)
		elog(ERROR, "Slony-I: failed to read sl_local_node_id");
	status->localNodeId = DatumGetInt32(
		SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull));
	SPI_freetuptable(SPI_tuptable);
	if (status->localNodeId < 0)
		elog(ERROR, "Slony-I: Node is uninitialized");

	/*
	 * Initialize the currentXid to invalid
	 */
	status->currentXid = InvalidTransactionId;

	/*
	 * Insert the new control block into the list
	 */
	status->next = clusterStatusList;
	clusterStatusList = status;

	return status;
}


