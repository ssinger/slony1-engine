/* ----------------------------------------------------------------------
 * slony1_funcs.c
 *
 *	  The C functions and triggers portion of Slony-I.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 * ----------------------------------------------------------------------
 */

#include "postgres.h"
#ifdef MSVC
#include "config_msvc.h"
#else
#include "config.h"
#endif


#include "avl_tree.c"

#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/keywords.h"
#include "parser/parse_type.h"
#include "executor/spi.h"
#include "commands/trigger.h"
#include "commands/async.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_type.h"
#include "catalog/namespace.h"
#include "access/xact.h"
#include "access/transam.h"
#include "access/hash.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/hsearch.h"
#include "utils/timestamp.h"
#include "utils/int8.h"
#ifdef HAVE_GETACTIVESNAPSHOT
#include "utils/snapmgr.h"
#endif
#ifdef HAVE_TYPCACHE
#include "utils/typcache.h"
#else
#include "parser/parse_oper.h"
#endif
#include "mb/pg_wchar.h"

#include <signal.h>
#include <errno.h>
/*@+matchanyintegral@*/
/*@-compmempass@*/
/*@-immediatetrans@*/

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define FOO 1

#define versionFunc3(funcName,version) _Slony_I_## version##_##funcName
#define versionFunc2(funcName,version) versionFunc3(funcName,version)
#define versionFunc(funcName) versionFunc2(funcName,SLONY_I_FUNC_VERSION_STRING)

PG_FUNCTION_INFO_V1(versionFunc(createEvent));
PG_FUNCTION_INFO_V1(versionFunc(getLocalNodeId));
PG_FUNCTION_INFO_V1(versionFunc(getModuleVersion));

PG_FUNCTION_INFO_V1(versionFunc(logTrigger));
PG_FUNCTION_INFO_V1(versionFunc(denyAccess));
PG_FUNCTION_INFO_V1(versionFunc(logApply));
PG_FUNCTION_INFO_V1(versionFunc(logApplySetCacheSize));
PG_FUNCTION_INFO_V1(versionFunc(logApplySaveStats));
PG_FUNCTION_INFO_V1(versionFunc(lockedSet));
PG_FUNCTION_INFO_V1(versionFunc(killBackend));
PG_FUNCTION_INFO_V1(versionFunc(seqtrack));

PG_FUNCTION_INFO_V1(versionFunc(slon_quote_ident));
PG_FUNCTION_INFO_V1(versionFunc(resetSession));
PG_FUNCTION_INFO_V1(versionFunc(slon_decode_tgargs));



Datum		versionFunc(createEvent) (PG_FUNCTION_ARGS);
Datum		versionFunc(getLocalNodeId) (PG_FUNCTION_ARGS);
Datum		versionFunc(getModuleVersion) (PG_FUNCTION_ARGS);

Datum		versionFunc(logTrigger) (PG_FUNCTION_ARGS);
Datum		versionFunc(denyAccess) (PG_FUNCTION_ARGS);
Datum		versionFunc(logApply) (PG_FUNCTION_ARGS);
Datum		versionFunc(logApplySetCacheSize) (PG_FUNCTION_ARGS);
Datum		versionFunc(logApplySaveStats) (PG_FUNCTION_ARGS);
Datum		versionFunc(lockedSet) (PG_FUNCTION_ARGS);
Datum		versionFunc(killBackend) (PG_FUNCTION_ARGS);
Datum		versionFunc(seqtrack) (PG_FUNCTION_ARGS);

Datum		versionFunc(slon_quote_ident) (PG_FUNCTION_ARGS);
Datum		versionFunc(slon_decode_tgargs) (PG_FUNCTION_ARGS);

Datum		versionFunc(resetSession) (PG_FUNCTION_ARGS);

#ifdef CYGWIN
extern DLLIMPORT Node *newNodeMacroHolder;
#endif

#define PLAN_NONE			0
#define PLAN_INSERT_EVENT	(1 << 1)
#define PLAN_INSERT_LOG_STATUS (1 << 2)
#define PLAN_APPLY_QUERIES	(1 << 3)

/*
 * This OID definition is missing in 8.3, although the data type
 * does exist.
 */
#ifndef TEXTARRAYOID
#define TEXTARRAYOID 1009
#endif


/* ----
 * Slony_I_ClusterStatus -
 *
 *	The per-cluster data to hold for functions and triggers.
 * ----
 */
typedef struct slony_I_cluster_status
{
	NameData	clustername;
	char	   *clusterident;
	int32		localNodeId;
	TransactionId currentXid;
	void	   *plan_active_log;

	int			have_plan;
	void	   *plan_insert_event;
	void	   *plan_insert_log_1;
	void	   *plan_insert_log_2;
	void	   *plan_insert_log_script;
	void	   *plan_record_sequences;
	void	   *plan_get_logstatus;
	void	   *plan_table_info;
	void	   *plan_apply_stats_update;
	void	   *plan_apply_stats_insert;

	text	   *cmdtype_I;
	text	   *cmdtype_U;
	text	   *cmdtype_D;

	struct slony_I_cluster_status *next;
}	Slony_I_ClusterStatus;


/*
 * Defining APPLY_CACHE_VERIFY causes the apply cache to store a second
 * copy of the query hash key and verify it when hash_search() reports "found".
 * The reason for doing this is that it is not entirely clear if using
 * a char pointer as the hash key works the way we are using it.
 */
#define APPLY_CACHE_VERIFY

typedef struct apply_cache_entry
{
	char	   *queryKey;

	void	   *plan;
	bool		forward;
	struct apply_cache_entry *prev;
	struct apply_cache_entry *next;

	FmgrInfo   *finfo_input;
	Oid		   *typioparam;
	int32	   *typmod;

#ifdef APPLY_CACHE_VERIFY
	char	   *verifyKey;
	int			evicted;
#endif
}	ApplyCacheEntry;


static MemoryContext applyCacheContext = NULL;
static HTAB *applyCacheHash = NULL;
static ApplyCacheEntry *applyCacheHead = NULL;
static ApplyCacheEntry *applyCacheTail = NULL;
static int	applyCacheSize = 100;
static int	applyCacheUsed = 0;

static uint32 applyCache_hash(const void *kp, Size ksize);
static int	applyCache_cmp(const void *kp1, const void *kp2, Size ksize);

static char *applyQuery = NULL;
static char *applyQueryPos = NULL;
static int	applyQuerySize = 8192;

static void applyQueryReset(void);
static void applyQueryIncrease(void);

static int64 apply_num_insert;
static int64 apply_num_update;
static int64 apply_num_delete;
static int64 apply_num_truncate;
static int64 apply_num_script;
static int64 apply_num_prepare;
static int64 apply_num_hit;
static int64 apply_num_evict;


/*@null@*/
static Slony_I_ClusterStatus *clusterStatusList = NULL;
static Slony_I_ClusterStatus *
getClusterStatus(Name cluster_name,
				 int need_plan_mask);
static const char *slon_quote_identifier(const char *ident);
static int prepareLogPlan(Slony_I_ClusterStatus * cs,
			   int log_status);

Datum
versionFunc(createEvent) (PG_FUNCTION_ARGS)
{
	TransactionId newXid = GetTopTransactionId();
	Slony_I_ClusterStatus *cs;
	char	   *ev_type_c;
	Datum		argv[9];
	char		nulls[10];
	int			rc;
	int			i;
	int64		retval;
	bool		isnull;

#ifdef HAVE_GETACTIVESNAPSHOT
	if (GetActiveSnapshot() == NULL)
		elog(ERROR, "Slony-I: ActiveSnapshot is NULL in createEvent()");
#else
	if (SerializableSnapshot == NULL)
		elog(ERROR, "Slony-I: SerializableSnapshot is NULL in createEvent()");
#endif

	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in createEvent()");

	/*
	 * Get or create the cluster status information and make sure it has the
	 * SPI plans that we need here.
	 */
	cs = getClusterStatus(PG_GETARG_NAME(0),
						  PLAN_INSERT_EVENT);

	/*
	 * Do the following only once per transaction.
	 */
	if (!TransactionIdEquals(cs->currentXid, newXid))
	{
		cs->currentXid = newXid;
	}

	/*
	 * Call the saved INSERT plan
	 */
	for (i = 1; i < 10; i++)
	{
		if (i >= PG_NARGS() || PG_ARGISNULL(i))
		{
			argv[i - 1] = (Datum) 0;
			nulls[i - 1] = 'n';
		}
		else
		{
			argv[i - 1] = PG_GETARG_DATUM(i);
			nulls[i - 1] = ' ';
		}
	}
	nulls[9] = '\0';

	if ((rc = SPI_execp(cs->plan_insert_event, argv, nulls, 0)) < 0)
		elog(ERROR, "Slony-I: SPI_execp() failed for \"INSERT INTO sl_event ...\"");

	/*
	 * The INSERT plan also contains a SELECT currval('sl_event_seq'), use the
	 * new sequence number as return value.
	 */
	if (SPI_processed != 1)
		elog(ERROR, "Slony-I: INSERT plan did not return 1 result row");
	retval = DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0],
										 SPI_tuptable->tupdesc, 1, &isnull));

	/*
	 * For SYNC and ENABLE_SUBSCRIPTION events, we also remember all current
	 * sequence values.
	 */
	if (PG_NARGS() > 1 && !PG_ARGISNULL(1))
	{
		ev_type_c = DatumGetPointer(DirectFunctionCall1(
											   textout, PG_GETARG_DATUM(1)));
		if (strcmp(ev_type_c, "SYNC") == 0 ||
			strcmp(ev_type_c, "ENABLE_SUBSCRIPTION") == 0)
		{
/*@-nullpass@*/
			if ((rc = SPI_execp(cs->plan_record_sequences, NULL, NULL, 0)) < 0)
				elog(ERROR, "Slony-I: SPI_execp() failed for \"INSERT INTO sl_seqlog ...\"");
/*@+nullpass@*/
		}
	}

	(void) SPI_finish();
/*@-mustfreefresh@*/
	PG_RETURN_INT64(retval);
}

/*@+mustfreefresh@*/


/*
 * versionFunc(getLocalNodeId) -
 *
 *	  SQL callable wrapper for calling getLocalNodeId() in order
 *	  to get the current setting of sequence sl_local_node_id with
 *	  configuration check.
 *
 */
Datum
versionFunc(getLocalNodeId) (PG_FUNCTION_ARGS)
{
	Slony_I_ClusterStatus *cs;
	int			rc;

	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in getLocalNodeId()");

	cs = getClusterStatus(PG_GETARG_NAME(0), PLAN_NONE);

	SPI_finish();

	PG_RETURN_INT32(cs->localNodeId);
}


/*
 * versionFunc(getModuleVersion) -
 *
 *	  SQL callable function to determine the version number
 *	  of this shared object during the startup checks.
 *
 */
Datum
versionFunc(getModuleVersion) (PG_FUNCTION_ARGS)
{
	text	   *retval;
	int			len;

	len = strlen(SLONY_I_VERSION_STRING);
	retval = palloc(VARHDRSZ + len);

	SET_VARSIZE(retval, VARHDRSZ + len);
	memcpy(VARDATA(retval), SLONY_I_VERSION_STRING, len);

	PG_RETURN_TEXT_P(retval);
}


Datum
versionFunc(logTrigger) (PG_FUNCTION_ARGS)
{
	TransactionId newXid = GetTopTransactionId();
	Slony_I_ClusterStatus *cs;
	TriggerData *tg;
	Datum		log_param[6];
	text	   *cmdtype = NULL;
	int32		cmdupdncols = 0;
	int			rc;
	Name		cluster_name;
	int32		tab_id;
	char	   *attkind;
	int			attkind_idx;

	char	   *olddatestyle = NULL;
	Datum	   *cmdargs = NULL;
	Datum	   *cmdargselem = NULL;
	bool	   *cmdnulls = NULL;
	bool	   *cmdnullselem = NULL;
	int			cmddims[1];
	int			cmdlbs[1];

	/*
	 * Don't do any logging if the current session role isn't Origin.
	 */
	if (SessionReplicationRole != SESSION_REPLICATION_ROLE_ORIGIN)
		return PointerGetDatum(NULL);

	/*
	 * Get the trigger call context
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "Slony-I: logTrigger() not called as trigger");
	tg = (TriggerData *) (fcinfo->context);

	/*
	 * Check all logTrigger() calling conventions
	 */
	if (!TRIGGER_FIRED_AFTER(tg->tg_event))
		elog(ERROR, "Slony-I: logTrigger() must be fired AFTER");
	if (!TRIGGER_FIRED_FOR_ROW(tg->tg_event))
		elog(ERROR, "Slony-I: logTrigger() must be fired FOR EACH ROW");
	if (tg->tg_trigger->tgnargs != 3)
		elog(ERROR, "Slony-I: logTrigger() must be defined with 3 args");

	/*
	 * Connect to the SPI manager
	 */
	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in logTrigger()");

	/*
	 * Get all the trigger arguments
	 */
	cluster_name = DatumGetName(DirectFunctionCall1(namein,
								CStringGetDatum(tg->tg_trigger->tgargs[0])));
	tab_id = strtol(tg->tg_trigger->tgargs[1], NULL, 10);
	attkind = tg->tg_trigger->tgargs[2];

	/*
	 * Get or create the cluster status information and make sure it has the
	 * SPI plans that we need here.
	 */
	cs = getClusterStatus(cluster_name, PLAN_INSERT_LOG_STATUS);

	/*
	 * Do the following only once per transaction.
	 */
	if (!TransactionIdEquals(cs->currentXid, newXid))
	{
		int32		log_status;
		bool		isnull;

		/*
		 * Determine the currently active log table
		 */
		if (SPI_execp(cs->plan_get_logstatus, NULL, NULL, 0) < 0)
			elog(ERROR, "Slony-I: cannot determine log status");
		if (SPI_processed != 1)
			elog(ERROR, "Slony-I: cannot determine log status");

		log_status = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[0],
										 SPI_tuptable->tupdesc, 1, &isnull));
		SPI_freetuptable(SPI_tuptable);
		prepareLogPlan(cs, log_status);
		switch (log_status)
		{
			case 0:
			case 2:
				cs->plan_active_log = cs->plan_insert_log_1;
				break;

			case 1:
			case 3:
				cs->plan_active_log = cs->plan_insert_log_2;
				break;

			default:
				elog(ERROR, "Slony-I: illegal log status %d", log_status);
				break;
		}

		cs->currentXid = newXid;
	}

	/*
	 * Save the current datestyle setting and switch to ISO (if not already)
	 */
	olddatestyle = GetConfigOptionByName("DateStyle", NULL);
	if (!strstr(olddatestyle, "ISO"))
	{
#ifdef SETCONFIGOPTION_6
		set_config_option("DateStyle", "ISO", PGC_USERSET, PGC_S_SESSION,
						  true, true);
#elif defined(SETCONFIGOPTION_7)
		set_config_option("DateStyle", "ISO", PGC_USERSET, PGC_S_SESSION,
						  true, true, 0);
#elif defined(SETCONFIGOPTION_8)
		set_config_option("DateStyle", "ISO", PGC_USERSET, PGC_S_SESSION,
						  true, true, 0, 0);
#endif
	}


	/*
	 * Determine cmdtype and cmdargs depending on the command type
	 */
	if (TRIGGER_FIRED_BY_INSERT(tg->tg_event))
	{
		HeapTuple	new_row = tg->tg_trigtuple;
		TupleDesc	tupdesc = tg->tg_relation->rd_att;
		char	   *col_value;

		int			i;

		/*
		 * INSERT
		 *
		 * cmdtype = 'I' cmdargs = colname, newval [, ...]
		 */
		cmdtype = cs->cmdtype_I;

		cmdargselem = cmdargs = (Datum *) palloc(sizeof(Datum) *
								 ((tg->tg_relation->rd_att->natts * 2) + 2));
		cmdnullselem = cmdnulls = (bool *) palloc(sizeof(bool) *
								 ((tg->tg_relation->rd_att->natts * 2) + 2));

		/*
		 * Specify all the columns
		 */
		for (i = 0; i < tg->tg_relation->rd_att->natts; i++)
		{
			/*
			 * Skip dropped columns
			 */
			if (tupdesc->attrs[i]->attisdropped)
				continue;

			/*
			 * Add the column name
			 */
			*cmdargselem++ = DirectFunctionCall1(textin,
								 CStringGetDatum(SPI_fname(tupdesc, i + 1)));
			*cmdnullselem++ = false;

			/*
			 * Add the column value
			 */
			if ((col_value = SPI_getvalue(new_row, tupdesc, i + 1)) == NULL)
			{
				*cmdnullselem++ = true;
				cmdargselem++;
			}
			else
			{
				*cmdargselem++ = DirectFunctionCall1(textin,
												 CStringGetDatum(col_value));
				*cmdnullselem++ = false;
			}
		}

	}
	else if (TRIGGER_FIRED_BY_UPDATE(tg->tg_event))
	{
		HeapTuple	old_row = tg->tg_trigtuple;
		HeapTuple	new_row = tg->tg_newtuple;
		TupleDesc	tupdesc = tg->tg_relation->rd_att;
		Datum		old_value;
		Datum		new_value;
		bool		old_isnull;
		bool		new_isnull;

		char	   *col_ident;
		char	   *col_value;
		int			i;

		/*
		 * UPDATE
		 *
		 * cmdtype = 'U' cmdargs = pkcolname, oldval [, ...] colname, newval
		 * [, ...]
		 */
		cmdtype = cs->cmdtype_U;

		cmdargselem = cmdargs = (Datum *) palloc(sizeof(Datum) *
								 ((tg->tg_relation->rd_att->natts * 4) + 3));
		cmdnullselem = cmdnulls = (bool *) palloc(sizeof(bool) *
								 ((tg->tg_relation->rd_att->natts * 4) + 3));

		/*
		 * For all changed columns, add name+value pairs and count them.
		 */
		for (i = 0; i < tg->tg_relation->rd_att->natts; i++)
		{
			/*
			 * Ignore dropped columns
			 */
			if (tupdesc->attrs[i]->attisdropped)
				continue;

			old_value = SPI_getbinval(old_row, tupdesc, i + 1, &old_isnull);
			new_value = SPI_getbinval(new_row, tupdesc, i + 1, &new_isnull);

			/*
			 * If old and new value are NULL, the column is unchanged
			 */
			if (old_isnull && new_isnull)
				continue;

			/*
			 * If both are NOT NULL, we need to compare the values and skip
			 * setting the column if equal
			 */
			if (!old_isnull && !new_isnull)
			{
				Oid			opr_oid;
				FmgrInfo   *opr_finfo_p;

				/*
				 * Lookup the equal operators function call info using the
				 * typecache if available
				 */
#ifdef HAVE_TYPCACHE
				TypeCacheEntry *type_cache;

				type_cache = lookup_type_cache(
											   SPI_gettypeid(tupdesc, i + 1),
								  TYPECACHE_EQ_OPR | TYPECACHE_EQ_OPR_FINFO);
				opr_oid = type_cache->eq_opr;
				if (opr_oid == ARRAY_EQ_OP)
					opr_oid = InvalidOid;
				else
					opr_finfo_p = &(type_cache->eq_opr_finfo);
#else
				FmgrInfo	opr_finfo;

				opr_oid = compatible_oper_funcid(makeList1(makeString("=")),
											   SPI_gettypeid(tupdesc, i + 1),
										SPI_gettypeid(tupdesc, i + 1), true);
				if (OidIsValid(opr_oid))
				{
					fmgr_info(opr_oid, &opr_finfo);
					opr_finfo_p = &opr_finfo;
				}
#endif

				/*
				 * If we have an equal operator, use that to do binary
				 * comparision. Else get the string representation of both
				 * attributes and do string comparision.
				 */
				if (OidIsValid(opr_oid))
				{
					if (DatumGetBool(FunctionCall2(opr_finfo_p,
												   old_value, new_value)))
						continue;
				}
				else
				{
					char	   *old_strval = SPI_getvalue(old_row, tupdesc, i + 1);
					char	   *new_strval = SPI_getvalue(new_row, tupdesc, i + 1);

					if (strcmp(old_strval, new_strval) == 0)
						continue;
				}
			}

			*cmdargselem++ = DirectFunctionCall1(textin,
								 CStringGetDatum(SPI_fname(tupdesc, i + 1)));
			*cmdnullselem++ = false;
			if (new_isnull)
			{
				*cmdnullselem++ = true;
				cmdargselem++;
			}
			else
			{
				*cmdargselem++ = DirectFunctionCall1(textin,
					 CStringGetDatum(SPI_getvalue(new_row, tupdesc, i + 1)));
				*cmdnullselem++ = false;
			}
			cmdupdncols++;
		}

		/*
		 * Add pairs of PK column names and values
		 */
		for (i = 0, attkind_idx = -1; i < tg->tg_relation->rd_att->natts; i++)
		{
			/*
			 * Ignore dropped columns
			 */
			if (tupdesc->attrs[i]->attisdropped)
				continue;

			attkind_idx++;
			if (!attkind[attkind_idx])
				break;
			if (attkind[attkind_idx] != 'k')
				continue;
			col_ident = SPI_fname(tupdesc, i + 1);
			col_value = SPI_getvalue(old_row, tupdesc, i + 1);
			if (col_value == NULL)
				elog(ERROR, "Slony-I: old key column %s.%s IS NULL on UPDATE",
					 NameStr(tg->tg_relation->rd_rel->relname), col_ident);

			*cmdargselem++ = DirectFunctionCall1(textin,
												 CStringGetDatum(col_ident));
			*cmdnullselem++ = false;

			*cmdargselem++ = DirectFunctionCall1(textin,
												 CStringGetDatum(col_value));
			*cmdnullselem++ = false;
		}

	}
	else if (TRIGGER_FIRED_BY_DELETE(tg->tg_event))
	{
		HeapTuple	old_row = tg->tg_trigtuple;
		TupleDesc	tupdesc = tg->tg_relation->rd_att;
		char	   *col_ident;
		char	   *col_value;
		int			i;

		/*
		 * DELETE
		 *
		 * cmdtype = 'D' cmdargs = pkcolname, oldval [, ...]
		 */
		cmdtype = cs->cmdtype_D;

		cmdargselem = cmdargs = (Datum *) palloc(sizeof(Datum) *
								 ((tg->tg_relation->rd_att->natts * 2) + 2));
		cmdnullselem = cmdnulls = (bool *) palloc(sizeof(bool) *
								 ((tg->tg_relation->rd_att->natts * 2) + 2));

		/*
		 * Add the PK columns
		 */
		for (i = 0, attkind_idx = -1; i < tg->tg_relation->rd_att->natts; i++)
		{
			if (tupdesc->attrs[i]->attisdropped)
				continue;

			attkind_idx++;
			if (!attkind[attkind_idx])
				break;
			if (attkind[attkind_idx] != 'k')
				continue;

			*cmdargselem++ = DirectFunctionCall1(textin,
					 CStringGetDatum(col_ident = SPI_fname(tupdesc, i + 1)));
			*cmdnullselem++ = false;

			col_value = SPI_getvalue(old_row, tupdesc, i + 1);
			if (col_value == NULL)
				elog(ERROR, "Slony-I: old key column %s.%s IS NULL on DELETE",
					 NameStr(tg->tg_relation->rd_rel->relname), col_ident);
			*cmdargselem++ = DirectFunctionCall1(textin,
												 CStringGetDatum(col_value));
			*cmdnullselem++ = false;
		}
	}
	else
		elog(ERROR, "Slony-I: logTrigger() fired for unhandled event");

	/*
	 * Restore the datestyle
	 */
	if (!strstr(olddatestyle, "ISO"))
	{
#ifdef SETCONFIGOPTION_6
		set_config_option("DateStyle", olddatestyle,
						  PGC_USERSET, PGC_S_SESSION, true, true);
#elif defined(SETCONFIGOPTION_7)
		set_config_option("DateStyle", olddatestyle,
						  PGC_USERSET, PGC_S_SESSION, true, true, 0);
#elif defined(SETCONFIGOPTION_8)
		set_config_option("DateStyle", olddatestyle,
						  PGC_USERSET, PGC_S_SESSION, true, true, 0, 0);
#endif
	}

	/*
	 * Construct the parameter array and insert the log row.
	 */
	cmddims[0] = cmdargselem - cmdargs;
	cmdlbs[0] = 1;

	log_param[0] = Int32GetDatum(tab_id);
	log_param[1] = DirectFunctionCall1(textin,
									   CStringGetDatum(get_namespace_name(
									RelationGetNamespace(tg->tg_relation))));
	log_param[2] = DirectFunctionCall1(textin,
				  CStringGetDatum(RelationGetRelationName(tg->tg_relation)));
	log_param[3] = PointerGetDatum(cmdtype);
	log_param[4] = Int32GetDatum(cmdupdncols);
	log_param[5] = PointerGetDatum(construct_md_array(cmdargs, cmdnulls, 1,
								  cmddims, cmdlbs, TEXTOID, -1, false, 'i'));

	SPI_execp(cs->plan_active_log, log_param, NULL, 0);

	SPI_finish();
	return PointerGetDatum(NULL);
}


Datum
versionFunc(denyAccess) (PG_FUNCTION_ARGS)
{
	TriggerData *tg;

	/*
	 * Get the trigger call context
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "Slony-I: denyAccess() not called as trigger");
	tg = (TriggerData *) (fcinfo->context);

	/*
	 * Check all logTrigger() calling conventions
	 */
	if (!TRIGGER_FIRED_BEFORE(tg->tg_event))
		elog(ERROR, "Slony-I: denyAccess() must be fired BEFORE");
	if (!TRIGGER_FIRED_FOR_ROW(tg->tg_event))
		elog(ERROR, "Slony-I: denyAccess() must be fired FOR EACH ROW");
	if (tg->tg_trigger->tgnargs != 1)
		elog(ERROR, "Slony-I: denyAccess() must be defined with 1 arg");

	/*
	 * Connect to the SPI manager
	 */
	if (SPI_connect() < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in denyAccess()");

	/*
	 * If the replication role is: ORIGIN - default role --> FAIL REPLICA -
	 * this trigger will not fire in --> N/A LOCAL - role when running "local
	 * updates" --> PERMIT UPDATE
	 */
	if (SessionReplicationRole == SESSION_REPLICATION_ROLE_ORIGIN)
		elog(ERROR,
			 "Slony-I: Table %s is replicated and cannot be "
			 "modified on a subscriber node - role=%d",
		  NameStr(tg->tg_relation->rd_rel->relname), SessionReplicationRole);

	SPI_finish();
	if (TRIGGER_FIRED_BY_UPDATE(tg->tg_event))
		return PointerGetDatum(tg->tg_newtuple);
	else
		return PointerGetDatum(tg->tg_trigtuple);
}


Datum
versionFunc(logApply) (PG_FUNCTION_ARGS)
{
	TransactionId newXid = GetTopTransactionId();
	Slony_I_ClusterStatus *cs;
	TriggerData *tg;
	HeapTuple	new_row;
	TupleDesc	tupdesc;
	Name		cluster_name;
	int			rc;
	bool		isnull;
	Relation	target_rel;

	Datum		dat;
	char		cmdtype;
	int32		tableid;
	char	   *nspname;
	char	   *relname;
	int32		cmdupdncols;
	Datum	   *cmdargs;
	bool	   *cmdargsnulls;
	int			cmdargsn;
	int			querynvals = 0;
	Datum	   *queryvals = NULL;
	Oid		   *querytypes = NULL;
	char	   *querynulls = NULL;
	char	  **querycolnames = NULL;
	int			i;
	int			spi_rc;

	MemoryContext oldContext;
	ApplyCacheEntry *cacheEnt;
	char	   *cacheKey;
	bool		found;

	/*
	 * Get the trigger call context
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "Slony-I: logApply() not called as trigger");
	tg = (TriggerData *) (fcinfo->context);

	/*
	 * Don't do any applying if the current session role isn't Replica.
	 */
	if (SessionReplicationRole != SESSION_REPLICATION_ROLE_REPLICA)
		return PointerGetDatum(tg->tg_trigtuple);

	/*
	 * Check all logApply() calling conventions
	 */
	if (!TRIGGER_FIRED_BEFORE(tg->tg_event))
		elog(ERROR, "Slony-I: logApply() must be fired BEFORE");
	if (!TRIGGER_FIRED_FOR_ROW(tg->tg_event))
		elog(ERROR, "Slony-I: logApply() must be fired FOR EACH ROW");
	if (tg->tg_trigger->tgnargs != 1)
		elog(ERROR, "Slony-I: logApply() must be defined with 1 arg");

	/*
	 * Connect to the SPI manager
	 */
	if ((rc = SPI_connect()) < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in logApply()");

	/*
	 * Get or create the cluster status information and make sure it has the
	 * SPI plans that we need here.
	 */
	cluster_name = DatumGetName(DirectFunctionCall1(namein,
								CStringGetDatum(tg->tg_trigger->tgargs[0])));
	cs = getClusterStatus(cluster_name, PLAN_APPLY_QUERIES);

	/*
	 * Do the following only once per transaction.
	 */
	if (!TransactionIdEquals(cs->currentXid, newXid))
	{
		HASHCTL		hctl;

		/*
		 * Free all prepared apply queries.
		 */
		for (cacheEnt = applyCacheHead; cacheEnt; cacheEnt = cacheEnt->next)
		{
			if (cacheEnt->plan != NULL)
				SPI_freeplan(cacheEnt->plan);
			cacheEnt->plan = NULL;
		}
		applyCacheHead = NULL;
		applyCacheTail = NULL;
		applyCacheUsed = 0;

		/*
		 * Destroy and recreate the hashtable for the apply cache
		 */
		if (applyCacheHash != NULL)
			hash_destroy(applyCacheHash);
		memset(&hctl, 0, sizeof(hctl));
		hctl.keysize = sizeof(char *);
		hctl.entrysize = sizeof(ApplyCacheEntry);
		hctl.hash = applyCache_hash;
		hctl.match = applyCache_cmp;
		applyCacheHash = hash_create("Slony-I apply cache",
									 50, &hctl,
								   HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

		/*
		 * Reset or create the apply cache key memory context.
		 */
		if (applyCacheContext == NULL)
		{
			applyCacheContext = AllocSetContextCreate(
													  TopMemoryContext,
												  "Slony-I apply query keys",
													ALLOCSET_DEFAULT_MINSIZE,
												   ALLOCSET_DEFAULT_INITSIZE,
												   ALLOCSET_DEFAULT_MAXSIZE);
		}
		else
		{
			MemoryContextReset(applyCacheContext);
		}

		/*
		 * Reset statistic counters.
		 */
		apply_num_insert = 0;
		apply_num_update = 0;
		apply_num_delete = 0;
		apply_num_truncate = 0;
		apply_num_script = 0;
		apply_num_prepare = 0;
		apply_num_hit = 0;
		apply_num_evict = 0;

		cs->currentXid = newXid;
	}

	/*
	 * Get the cmdtype first.
	 */
	new_row = tg->tg_trigtuple;
	tupdesc = tg->tg_relation->rd_att;

	dat = SPI_getbinval(new_row, tupdesc,
						SPI_fnumber(tupdesc, "log_cmdtype"), &isnull);
	if (isnull)
		elog(ERROR, "Slony-I: log_cmdtype is NULL");
	cmdtype = DatumGetChar(dat);

	/*
	 * Rows coming from sl_log_script are handled different from regular data
	 * log rows since they don't have all the columns.
	 */
	if (cmdtype == 'S')
	{
		char	   *ddl_script;
		bool		localNodeFound = true;
		Datum		script_insert_args[5];
		Datum	   *nodeargs;
		bool	   *nodeargsnulls;
		int			nodeargsn;
		Datum	   *seqargs;
		bool	   *seqargsnulls;
		int			seqargsn;
		Datum		array_holder;
		Datum		delim_text;

		apply_num_script++;


		/*
		 * Turn the log_cmdargs into a plain array of Text Datums.
		 */
		dat = SPI_getbinval(new_row, tupdesc,
							SPI_fnumber(tupdesc, "log_cmdargs"), &isnull);
		if (isnull)
			elog(ERROR, "Slony-I: log_cmdargs is NULL");
		deconstruct_array(DatumGetArrayTypeP(dat),
						  TEXTOID, -1, false, 'i',
						  &cmdargs, &cmdargsnulls, &cmdargsn);

		nodeargs = NULL;
		nodeargsn = 0;
		seqargs = NULL;
		seqargsn = 0;
		if (cmdargsn >= 2)
		{
			delim_text = DirectFunctionCall1(textin, CStringGetDatum(","));
			if ((!cmdargsnulls[1]))
			{
				char	   *astr = DatumGetCString(DirectFunctionCall1(textout,
																cmdargs[1]));

				if (strcmp(astr, ""))
				{
					array_holder = DirectFunctionCall2(text_to_array, cmdargs[1],
													   delim_text);
					deconstruct_array(DatumGetArrayTypeP(array_holder),
									  TEXTOID, -1, false, 'i',
									  &nodeargs, &nodeargsnulls, &nodeargsn);
				}
			}
		}
		if (cmdargsn >= 3)
		{
			if ((!cmdargsnulls[2]))
			{
				char	   *astr = DatumGetCString(DirectFunctionCall1(textout,
																cmdargs[2]));

				if (strcmp(astr, ""))
				{
					array_holder = DirectFunctionCall2(text_to_array, cmdargs[2],
													   delim_text);
					deconstruct_array(DatumGetArrayTypeP(array_holder),
									  TEXTOID, -1, false, 'i',
									  &seqargs, &seqargsnulls, &seqargsn);
				}
			}
		}

		/*
		 * The first element is the DDL statement itself.
		 */
		ddl_script = DatumGetCString(DirectFunctionCall1(
													   textout, cmdargs[0]));

		/*
		 * If there is an optional node ID list, check that we are in it.
		 */
		if (nodeargsn > 0)
		{
			localNodeFound = false;
			for (i = 0; i < nodeargsn; i++)
			{
				int32		nodeId = DatumGetInt32(
												   DirectFunctionCall1(int4in,
								 DirectFunctionCall1(textout, nodeargs[i])));

				if (nodeId == cs->localNodeId)
				{
					localNodeFound = true;
					break;
				}
			}
		}

		/*
		 * Execute the DDL statement if the node list is empty or our local
		 * node ID appears in it.
		 */
		if (localNodeFound)
		{

			char		query[1024];
			Oid			argtypes[3];
			void	   *plan = NULL;

			argtypes[0] = INT4OID;
			argtypes[1] = INT4OID;
			argtypes[2] = INT8OID;

			snprintf(query, 1023, "select \"%s\".sequenceSetValue($1," \
					 "$2,NULL,$3,true); ", tg->tg_trigger->tgargs[0]);
			plan = SPI_prepare(query, 3, argtypes);
			if (plan == NULL)
			{

				elog(ERROR, "could not prepare plan to call sequenceSetValue");
			}
			/**
			 * before we execute the DDL we need to update the sequences.
			 */
			if (seqargsn > 0)
			{

				for (i = 0; (i + 2) < seqargsn; i = i + 3)
				{
					Datum		call_args[3];
					bool		call_nulls[3];

					call_args[0] = DirectFunctionCall1(int4in,
								   DirectFunctionCall1(textout, seqargs[i]));
					call_args[1] = DirectFunctionCall1(int4in,
							   DirectFunctionCall1(textout, seqargs[i + 1]));
					call_args[2] = DirectFunctionCall1(int8in,
							   DirectFunctionCall1(textout, seqargs[i + 2]));

					call_nulls[0] = 0;
					call_nulls[1] = 0;
					call_nulls[2] = 0;

					if (SPI_execp(plan, call_args, call_nulls, 0) < 0)
					{
						elog(ERROR, "error executing sequenceSetValue plan");

					}

				}


			}

			sprintf(query, "set session_replication_role to local;");
			if (SPI_exec(query, 0) < 0)
			{
				elog(ERROR, "SPI_exec() failed for statement '%s'",
					 query);
			}


			if (SPI_exec(ddl_script, 0) < 0)
			{
				elog(ERROR, "SPI_exec() failed for DDL statement '%s'",
					 ddl_script);
			}

			sprintf(query, "set session_replication_role to replica;");
			if (SPI_exec(query, 0) < 0)
			{
				elog(ERROR, "SPI_exec() failed for statement '%s'",
					 query);
			}

			/*
			 * Set the currentXid to invalid to flush the apply query cache.
			 */
			cs->currentXid = InvalidTransactionId;
		}

		/*
		 * Build the parameters for the insert into sl_log_script and execute
		 * the query.
		 */
		script_insert_args[0] = SPI_getbinval(new_row, tupdesc,
								SPI_fnumber(tupdesc, "log_origin"), &isnull);
		script_insert_args[1] = SPI_getbinval(new_row, tupdesc,
								  SPI_fnumber(tupdesc, "log_txid"), &isnull);
		script_insert_args[2] = SPI_getbinval(new_row, tupdesc,
							 SPI_fnumber(tupdesc, "log_actionseq"), &isnull);
		script_insert_args[3] = SPI_getbinval(new_row, tupdesc,
							   SPI_fnumber(tupdesc, "log_cmdtype"), &isnull);
		script_insert_args[4] = SPI_getbinval(new_row, tupdesc,
							   SPI_fnumber(tupdesc, "log_cmdargs"), &isnull);
		if (SPI_execp(cs->plan_insert_log_script, script_insert_args, NULL, 0) < 0)
			elog(ERROR, "Execution of sl_log_script insert plan failed");

		/*
		 * Return NULL to suppress the insert into the original sl_log_N.
		 */
		SPI_finish();
		return PointerGetDatum(NULL);
	}

	/*
	 * Process the special ddlScript_complete row.
	 */
	if (cmdtype == 's')
	{
		bool		localNodeFound = true;
		Datum		script_insert_args[5];

		apply_num_script++;

		/*
		 * Turn the log_cmdargs into a plain array of Text Datums.
		 */
		dat = SPI_getbinval(new_row, tupdesc,
							SPI_fnumber(tupdesc, "log_cmdargs"), &isnull);
		if (isnull)
			elog(ERROR, "Slony-I: log_cmdargs is NULL");

		deconstruct_array(DatumGetArrayTypeP(dat),
						  TEXTOID, -1, false, 'i',
						  &cmdargs, &cmdargsnulls, &cmdargsn);

		/*
		 * If there is an optional node ID list, check that we are in it.
		 */
		if (cmdargsn > 1)
		{
			localNodeFound = false;
			for (i = 1; i < cmdargsn; i++)
			{
				int32		nodeId = DatumGetInt32(
												   DirectFunctionCall1(int4in,
								  DirectFunctionCall1(textout, cmdargs[i])));

				if (nodeId == cs->localNodeId)
				{
					localNodeFound = true;
					break;
				}
			}
		}

		/*
		 * Execute the ddlScript_complete_int() call if we have to.
		 */
		if (localNodeFound)
		{
			char		query[1024];

			snprintf(query, sizeof(query),
					 "select %s.ddlScript_complete_int();",
					 cs->clusterident);

			if (SPI_exec(query, 0) < 0)
			{
				elog(ERROR, "SPI_exec() failed for statement '%s'",
					 query);
			}


			/*
			 * Set the currentXid to invalid to flush the apply query cache.
			 */
			cs->currentXid = InvalidTransactionId;
		}

		/*
		 * Build the parameters for the insert into sl_log_script and execute
		 * the query.
		 */
		script_insert_args[0] = SPI_getbinval(new_row, tupdesc,
								SPI_fnumber(tupdesc, "log_origin"), &isnull);
		script_insert_args[1] = SPI_getbinval(new_row, tupdesc,
								  SPI_fnumber(tupdesc, "log_txid"), &isnull);
		script_insert_args[2] = SPI_getbinval(new_row, tupdesc,
							 SPI_fnumber(tupdesc, "log_actionseq"), &isnull);
		script_insert_args[3] = SPI_getbinval(new_row, tupdesc,
							   SPI_fnumber(tupdesc, "log_cmdtype"), &isnull);
		script_insert_args[4] = SPI_getbinval(new_row, tupdesc,
							   SPI_fnumber(tupdesc, "log_cmdargs"), &isnull);
		if (SPI_execp(cs->plan_insert_log_script, script_insert_args, NULL, 0) < 0)
			elog(ERROR, "Execution of sl_log_script insert plan failed");

		/*
		 * Return NULL to suppress the insert into the original sl_log_N.
		 */
		SPI_finish();
		return PointerGetDatum(NULL);
	}

	/*
	 * Normal data log row. Get all the relevant data from the log row.
	 */
	dat = SPI_getbinval(new_row, tupdesc,
						SPI_fnumber(tupdesc, "log_tableid"), &isnull);
	if (isnull)
		elog(ERROR, "Slony-I: log_tableid is NULL");
	tableid = DatumGetInt32(dat);
	nspname = SPI_getvalue(new_row, tupdesc,
						   SPI_fnumber(tupdesc, "log_tablenspname"));
	if (nspname == NULL)
		elog(ERROR, "Slony-I: log_tablenspname is NULL on INSERT/UPDATE/DELETE");

	relname = SPI_getvalue(new_row, tupdesc,
						   SPI_fnumber(tupdesc, "log_tablerelname"));
	if (relname == NULL)
		elog(ERROR, "Slony-I: log_tablerelname is NULL on INSERT/UPDATE/DELETE");

	dat = SPI_getbinval(new_row, tupdesc,
						SPI_fnumber(tupdesc, "log_cmdupdncols"), &isnull);
	if (isnull && cmdtype == 'U')
		elog(ERROR, "Slony-I: log_cmdupdncols is NULL on UPDATE");
	cmdupdncols = DatumGetInt32(dat);

	dat = SPI_getbinval(new_row, tupdesc,
						SPI_fnumber(tupdesc, "log_cmdargs"), &isnull);
	if (isnull)
		elog(ERROR, "Slony-I: log_cmdargs is NULL");

	/*
	 * Turn the log_cmdargs into a plain array of Text Datums.
	 */
	deconstruct_array(DatumGetArrayTypeP(dat),
					  TEXTOID, -1, false, 'i',
					  &cmdargs, &cmdargsnulls, &cmdargsn);

	/*
	 * Build the query cache key. This is for insert, update and truncate just
	 * the operation type and the table ID. For update we also append the
	 * fully quoted names of updated columns.
	 */
	applyQueryReset();
	sprintf(applyQueryPos, "%c,%d", cmdtype, tableid);
	applyQueryPos += strlen(applyQueryPos);

	if (cmdtype == 'U')
	{
		char	   *colname;

		for (i = 0; i < cmdupdncols * 2; i += 2)
		{
			applyQueryIncrease();

			colname = DatumGetCString(DirectFunctionCall1(
													   textout, cmdargs[i]));
			snprintf(applyQueryPos, applyQuerySize - (applyQueryPos - applyQuery),
					 ",%s", slon_quote_identifier(colname));
			applyQueryPos += strlen(applyQueryPos);
		}
	}

	/*
	 * We now need to copy this cache key into the cache context because the
	 * hash_search() call will eventually create the hash entry pointing to
	 * this string.
	 */
	oldContext = MemoryContextSwitchTo(applyCacheContext);
	cacheKey = pstrdup(applyQuery);
	MemoryContextSwitchTo(oldContext);

/*	elog(NOTICE, "looking for key=%s", cacheKey); */
	cacheEnt = hash_search(applyCacheHash, &cacheKey, HASH_ENTER, &found);
	if (found)
	{
		apply_num_hit++;

		/* elog(NOTICE, "cache entry for %s found", cacheKey); */

		/*
		 * Free the cacheKey copy.
		 */
		oldContext = MemoryContextSwitchTo(applyCacheContext);
#ifdef APPLY_CACHE_VERIFY
		if (cacheEnt->evicted)
			elog(ERROR, "Slony-I: query cache returned evicted entry for '%s'",
				 cacheKey);
		if (strcmp(cacheEnt->verifyKey, cacheKey) != 0)
			elog(ERROR, "Slony-I: query cache key verification failed - "
				 "searched='%s' found='%s'", cacheKey,
				 cacheEnt->verifyKey);
#endif
		pfree(cacheKey);
		MemoryContextSwitchTo(oldContext);

		/*
		 * We are reusing an existing query plan. Just move it to the end of
		 * the list.
		 */
		if (cacheEnt != applyCacheTail)
		{
			/*
			 * Remove the entry from the list
			 */
			if (cacheEnt->prev == NULL)
				applyCacheHead = cacheEnt->next;
			else
				cacheEnt->prev->next = cacheEnt->next;
			if (cacheEnt->next == NULL)
				applyCacheTail = cacheEnt->prev;
			else
				cacheEnt->next->prev = cacheEnt->prev;

			/*
			 * Put the entry back at the end of the list.
			 */
			if (applyCacheHead == NULL)
			{
				cacheEnt->prev = NULL;
				cacheEnt->next = NULL;
				applyCacheHead = cacheEnt;
				applyCacheTail = cacheEnt;
			}
			else
			{
				cacheEnt->prev = applyCacheTail;
				cacheEnt->next = NULL;
				applyCacheTail->next = cacheEnt;
				applyCacheTail = cacheEnt;
			}
		}
	}
	else
	{
		Datum		query_args[2];

		apply_num_prepare++;

		/* elog(NOTICE, "cache entry for %s NOT found", cacheKey); */

		/*
		 * Allocate memory for the function call info to cast all datums from
		 * TEXT to the required Datum type.
		 */
		oldContext = MemoryContextSwitchTo(applyCacheContext);
		cacheEnt->finfo_input = (FmgrInfo *) palloc(sizeof(FmgrInfo) * (cmdargsn / 2));
		cacheEnt->typioparam = (Oid *) palloc(sizeof(Oid) * (cmdargsn / 2));
		cacheEnt->typmod = (int32 *) palloc(sizeof(int32) * (cmdargsn / 2));
		MemoryContextSwitchTo(oldContext);

		if (cacheEnt->finfo_input == NULL || cacheEnt->typioparam == NULL ||
			cacheEnt->typmod == NULL)
			elog(ERROR, "Slony-I: out of memory in logApply()");

#ifdef APPLY_CACHE_VERIFY

		/*
		 * Save a second copy of the query key for verification/debugging
		 */
		oldContext = MemoryContextSwitchTo(applyCacheContext);
		cacheEnt->verifyKey = pstrdup(cacheKey);
		MemoryContextSwitchTo(oldContext);
		cacheEnt->evicted = 0;
#endif

		/*
		 * Find the target relation in the system cache. We need this to find
		 * the data types of the target columns for casting.
		 */
#ifdef HAS_LOOKUPEXPLICITNAMESPACE_2
		target_rel = RelationIdGetRelation(
										   get_relname_relid(relname,
								   LookupExplicitNamespace(nspname, false)));
#else
		target_rel = RelationIdGetRelation(
			   get_relname_relid(relname, LookupExplicitNamespace(nspname)));
#endif
		if (target_rel == NULL)
			elog(ERROR, "Slony-I: cannot find table %s.%s in logApply()",
				 slon_quote_identifier(nspname),
				 slon_quote_identifier(relname));

		/*
		 * Create the saved SPI plan for this query
		 */
		applyQueryReset();

		/*
		 * Build the query string and parameter type array for the
		 * SPI_prepare() call.
		 */
		switch (cmdtype)
		{
			case 'I':

				/*
				 * INSERT
				 */
				querycolnames = (char **) palloc(sizeof(char *) * cmdargsn / 2);
				querytypes = (Oid *) palloc(sizeof(Oid) * cmdargsn / 2);

				sprintf(applyQueryPos, "INSERT INTO %s.%s (",
						slon_quote_identifier(nspname),
						slon_quote_identifier(relname));
				applyQueryPos += strlen(applyQueryPos);

				/*
				 * Construct the list of quoted column names.
				 */
				for (i = 0; i < cmdargsn; i += 2)
				{
					char	   *colname;

					applyQueryIncrease();

					if (i > 0)
					{
						strcpy(applyQueryPos, ", ");
						applyQueryPos += 2;
					}

					if (cmdargsnulls[i])
						elog(ERROR, "Slony-I: column name in log_cmdargs is NULL");
					querycolnames[i / 2] = DatumGetCString(DirectFunctionCall1(
													   textout, cmdargs[i]));
					colname = (char *) slon_quote_identifier(querycolnames[i / 2]);
					strcpy(applyQueryPos, colname);
					applyQueryPos += strlen(applyQueryPos);
				}

				/*
				 * Add ") VALUES ("
				 */
				strcpy(applyQueryPos, ") VALUES (");
				applyQueryPos += strlen(applyQueryPos);

				/*
				 * Add $n::<coltype> placeholders for all the values.
				 */
				for (i = 0; i < cmdargsn; i += 2)
				{
					int			colnum;
					Oid			coltype;
					Oid			typinput;

					applyQueryIncrease();

					/*
					 * Lookup the column data type in the target relation and
					 * remember everything we need to know later to cast TEXT
					 * to the correct column Datum.
					 */
					colnum = SPI_fnumber(target_rel->rd_att, querycolnames[i / 2]);
					coltype = SPI_gettypeid(target_rel->rd_att, colnum);
					if (coltype == InvalidOid)
						elog(ERROR, "Slony-I: type lookup for column %s failed in logApply()",
							 querycolnames[i / 2]);
					getTypeInputInfo(coltype, &typinput,
									 &(cacheEnt->typioparam[i / 2]));
					oldContext = MemoryContextSwitchTo(applyCacheContext);
					fmgr_info(typinput, &(cacheEnt->finfo_input[i / 2]));
					MemoryContextSwitchTo(oldContext);
					cacheEnt->typmod[i / 2] =
						target_rel->rd_att->attrs[colnum - 1]->atttypmod;

					/*
					 * Add the parameter to the query string
					 */
					sprintf(applyQueryPos, "%s$%d", (i == 0) ? "" : ", ",
							i / 2 + 1);
					applyQueryPos += strlen(applyQueryPos);

					querytypes[i / 2] = coltype;
				}

				/*
				 * Finish the query string
				 */
				strcpy(applyQueryPos, ");");
				applyQueryPos += 2;
				querynvals = cmdargsn / 2;

				break;

			case 'U':

				/*
				 * UPDATE
				 */
				querycolnames = (char **) palloc(sizeof(char *) * cmdargsn / 2);
				querytypes = (Oid *) palloc(sizeof(Oid) * cmdargsn / 2);

				sprintf(applyQueryPos, "UPDATE ONLY %s.%s SET ",
						slon_quote_identifier(nspname),
						slon_quote_identifier(relname));
				applyQueryPos += strlen(applyQueryPos);

				/*
				 * This can all be done in one pass over the cmdargs array. We
				 * just have to switch the behavior slightly between the SET
				 * clause and the WHERE clause.
				 */
				for (i = 0; i < cmdargsn; i += 2)
				{
					char	   *colname;
					int			colnum;
					Oid			coltype;
					Oid			typinput;

					applyQueryIncrease();

					/*
					 * Get the column name and data type as well as everything
					 * needed later to cast TEXT to the correct input Datum.
					 */
					if (cmdargsnulls[i])
						elog(ERROR, "Slony-I: column name in log_cmdargs is NULL");
					colname = DatumGetCString(DirectFunctionCall1(
													   textout, cmdargs[i]));
					colnum = SPI_fnumber(target_rel->rd_att, colname);
					coltype = SPI_gettypeid(target_rel->rd_att, colnum);
					if (coltype == InvalidOid)
						elog(ERROR, "Slony-I: type lookup for column %s failed in logApply()",
							 colname);
					getTypeInputInfo(coltype, &typinput,
									 &(cacheEnt->typioparam[i / 2]));
					oldContext = MemoryContextSwitchTo(applyCacheContext);
					fmgr_info(typinput, &(cacheEnt->finfo_input[i / 2]));
					MemoryContextSwitchTo(oldContext);
					cacheEnt->typmod[i / 2] =
						target_rel->rd_att->attrs[colnum - 1]->atttypmod;

					/*
					 * Special case if there were no columns updated. We tell
					 * it to set the first PK column to itself.
					 */
					if (cmdupdncols == 0 && i == 0)
					{
						sprintf(applyQueryPos, "%s = %s",
								slon_quote_identifier(colname),
								slon_quote_identifier(colname));
						applyQueryPos += strlen(applyQueryPos);
					}

					/*
					 * If we are at the transition point from SET to WHERE,
					 * add the WHERE keyword.
					 */
					if (i == cmdupdncols * 2)
					{
						strcpy(applyQueryPos, " WHERE ");
						applyQueryPos += 7;
					}

					if (i < cmdupdncols * 2)
					{
						/*
						 * This is inside the SET clause. Add the <colname> =
						 * $n::<coltype> separated by comma.
						 */
						sprintf(applyQueryPos, "%s%s = $%d",
								(i > 0) ? ", " : "",
								slon_quote_identifier(colname),
								i / 2 + 1);
					}
					else
					{
						/*
						 * This is in the WHERE clause. Same as above but
						 * separated by AND.
						 */
						sprintf(applyQueryPos, "%s%s = $%d",
								(i > cmdupdncols * 2) ? " AND " : "",
								slon_quote_identifier(colname),
								i / 2 + 1);
					}
					applyQueryPos += strlen(applyQueryPos);

					querytypes[i / 2] = coltype;
				}

				strcpy(applyQueryPos, ";");
				applyQueryPos += 1;
				querynvals = cmdargsn / 2;

				break;

			case 'D':

				/*
				 * DELETE
				 */
				querycolnames = (char **) palloc(sizeof(char *) * cmdargsn / 2);
				querytypes = (Oid *) palloc(sizeof(Oid) * cmdargsn / 2);

				sprintf(applyQueryPos, "DELETE FROM ONLY %s.%s WHERE ",
						slon_quote_identifier(nspname),
						slon_quote_identifier(relname));
				applyQueryPos += strlen(applyQueryPos);

				for (i = 0; i < cmdargsn; i += 2)
				{
					int			colnum;
					char	   *colname;
					Oid			coltype;
					Oid			typinput;

					applyQueryIncrease();

					/*
					 * Add <colname> = $n separated by comma.
					 */
					if (cmdargsnulls[i])
						elog(ERROR, "Slony-I: column name in log_cmdargs is NULL");
					colname = DatumGetCString(DirectFunctionCall1(
													   textout, cmdargs[i]));
					colnum = SPI_fnumber(target_rel->rd_att, colname);
					coltype = SPI_gettypeid(target_rel->rd_att, colnum);
					if (coltype == InvalidOid)
						elog(ERROR, "Slony-I: type lookup for column %s failed in logApply()",
							 colname);
					getTypeInputInfo(coltype, &typinput,
									 &(cacheEnt->typioparam[i / 2]));
					oldContext = MemoryContextSwitchTo(applyCacheContext);
					fmgr_info(typinput, &(cacheEnt->finfo_input[i / 2]));
					MemoryContextSwitchTo(oldContext);
					cacheEnt->typmod[i / 2] =
						target_rel->rd_att->attrs[colnum - 1]->atttypmod;
					sprintf(applyQueryPos, "%s%s = $%d",
							(i > 0) ? " AND " : "",
							slon_quote_identifier(colname),
							i / 2 + 1);

					applyQueryPos += strlen(applyQueryPos);

					querytypes[i / 2] = coltype;
				}

				strcpy(applyQueryPos, ";");
				applyQueryPos += 1;

				querynvals = cmdargsn / 2;

				break;

			case 'T':

				/*
				 * TRUNCATE
				 */
				querytypes = (Oid *) palloc(sizeof(Oid) * 2);

				sprintf(applyQueryPos, "TRUNCATE ONLY %s.%s CASCADE ;",
						slon_quote_identifier(nspname),
						slon_quote_identifier(relname));

				querynvals = 0;

				break;

			default:
				elog(ERROR, "Slony-I: unhandled log cmdtype '%c' in logApply()",
					 cmdtype);
				break;
		}

		/*
		 * Close the target relation.
		 */
		RelationClose(target_rel);

		/*
		 * Prepare the saved SPI query plan.
		 */
		cacheEnt->plan = SPI_saveplan(
							SPI_prepare(applyQuery, querynvals, querytypes));
		if (cacheEnt->plan == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed for query '%s'",
				 applyQuery);
/*	elog(NOTICE, "key=%s nvals=%d query=%s ", cacheEnt->verifyKey, querynvals, applyQuery); */

		/*
		 * Add the plan to the double linked LRU list
		 */
		if (applyCacheHead == NULL)
		{
			cacheEnt->prev = NULL;
			cacheEnt->next = NULL;
			applyCacheHead = cacheEnt;
			applyCacheTail = cacheEnt;
		}
		else
		{
			cacheEnt->prev = applyCacheTail;
			cacheEnt->next = NULL;
			applyCacheTail->next = cacheEnt;
			applyCacheTail = cacheEnt;
		}
		applyCacheUsed++;

		/*
		 * If that pushes us over the maximum allowed cached plans, evict the
		 * one that wasn't used the longest.
		 */
		if (applyCacheUsed > applyCacheSize)
		{
			ApplyCacheEntry *evict = applyCacheHead;

			apply_num_evict++;

			SPI_freeplan(evict->plan);
			oldContext = MemoryContextSwitchTo(applyCacheContext);
			pfree(evict->finfo_input);
			pfree(evict->typioparam);
			pfree(evict->typmod);
			MemoryContextSwitchTo(oldContext);
			evict->finfo_input = NULL;
			evict->typioparam = NULL;
			evict->typmod = NULL;
			evict->plan = NULL;
#ifdef APPLY_CACHE_VERIFY
			evict->evicted = 1;
#endif

			if (evict->prev == NULL)
				applyCacheHead = evict->next;
			else
				evict->prev->next = evict->next;
			if (evict->next == NULL)
				applyCacheTail = evict->prev;
			else
				evict->next->prev = evict->prev;

			hash_search(applyCacheHash, &(evict->queryKey), HASH_REMOVE, &found);
			if (!found)
				elog(ERROR, "Slony-I: cached queries hash entry not found "
					 "on evict");

			applyCacheUsed--;
		}

		/*
		 * We also need to determine if this table belongs to a set, that we
		 * are a forwarder of.
		 */
		query_args[0] = SPI_getbinval(new_row, tupdesc,
							   SPI_fnumber(tupdesc, "log_tableid"), &isnull);
		query_args[1] = Int32GetDatum(cs->localNodeId);

		if (SPI_execp(cs->plan_table_info, query_args, NULL, 0) < 0)
			elog(ERROR, "SPI_execp() failed for table forward lookup");

		if (SPI_processed != 1)
			elog(ERROR, "forwarding lookup for table %d failed",
				 DatumGetInt32(query_args[1]));

		cacheEnt->forward = DatumGetBool(
				  SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc,
				SPI_fnumber(SPI_tuptable->tupdesc, "sub_forward"), &isnull));
	}

	/*
	 * We now have a cached SPI plan. Construct the call parameter and null
	 * flag arrays.
	 */
	switch (cmdtype)
	{
		case 'I':
		case 'U':
		case 'D':

			/*
			 * INSERT, UPDATE and DELETE
			 */
			queryvals = (Datum *) palloc(sizeof(Datum) * cmdargsn / 2);
			querynulls = (char *) palloc(cmdargsn / 2 + 1);

			for (i = 0; i < cmdargsn; i += 2)
			{
				char	   *tmpval;

				if (cmdargsnulls[i + 1])
				{
					queryvals[i / 2] = (Datum) 0;
					querynulls[i / 2] = 'n';
				}
				else
				{
					tmpval = DatumGetCString(DirectFunctionCall1(textout,
															cmdargs[i + 1]));
					queryvals[i / 2] = InputFunctionCall(
											 &(cacheEnt->finfo_input[i / 2]),
														 tmpval,
												 cacheEnt->typioparam[i / 2],
													cacheEnt->typmod[i / 2]);
					pfree(tmpval);
					querynulls[i / 2] = ' ';
				}
			}
			querynulls[cmdargsn / 2] = '\0';

			break;

		case 'T':

			/*
			 * TRUNCATE
			 */

			break;

		default:
			elog(ERROR, "Slony-I: unhandled log cmdtype '%c' in logApply()",
				 cmdtype);
			break;
	}

/*	elog(NOTICE, "using key=%s nvals=%d", cacheEnt->verifyKey, cmdargsn / 2); */

	/*
	 * Execute the query.
	 */
	if ((spi_rc = SPI_execp(cacheEnt->plan, queryvals, querynulls, 0)) < 0)
		elog(ERROR, "Slony-I: SPI_execp() failed - rc=%d", spi_rc);

	/*
	 * Count operations
	 */
	switch (cmdtype)
	{
		case 'I':
			apply_num_insert++;
			break;
		case 'U':
			apply_num_update++;
			break;
		case 'D':
			apply_num_delete++;
			break;
		case 'T':
			apply_num_truncate++;
			break;
		default:
			break;
	}

	/*
	 * Disconnect from SPI manager and return either the new tuple or NULL
	 * according to the forwarding of log data.
	 */
	SPI_finish();
	if (cacheEnt->forward)
		return PointerGetDatum(tg->tg_trigtuple);
	else
		return PointerGetDatum(NULL);
}


/*
 * versionFunc(logApplySetCacheSize)()
 *
 *	Called by slon during startup to set the size of the log apply
 *	query cache according to the config parameter apply_cache_size.
 */
Datum
versionFunc(logApplySetCacheSize) (PG_FUNCTION_ARGS)
{
	int32		newSize;
	int32		oldSize = applyCacheSize;

	if (!superuser())
		elog(ERROR, "Slony-I: insufficient privilege logApplySetCacheSize");

	newSize = PG_GETARG_INT32(0);

	if (newSize <= 0)
		PG_RETURN_INT32(oldSize);

	if (newSize < 10 || newSize > 2000)
		elog(ERROR, "Slony-I: logApplySetCacheSize(): illegal size");

	applyCacheSize = newSize;
	PG_RETURN_INT32(oldSize);
}


/*
 * versionFunc(logApplySaveStats)()
 *
 *	Save statistics at the end of SYNC processing.
 */
Datum
versionFunc(logApplySaveStats) (PG_FUNCTION_ARGS)
{
	Slony_I_ClusterStatus *cs;
	Datum		params[11];
	char	   *nulls = "           ";
	int32		rc = 0;
	int			spi_rc;

	if (!superuser())
		elog(ERROR, "Slony-I: insufficient privilege logApplySetCacheSize");

	/*
	 * Connect to the SPI manager
	 */
	if (SPI_connect() < 0)
		elog(ERROR, "Slony-I: SPI_connect() failed in logApply()");

	/*
	 * Get or create the cluster status information and make sure it has the
	 * SPI plans that we need here.
	 */
	cs = getClusterStatus(PG_GETARG_NAME(0), PLAN_APPLY_QUERIES);

	/*
	 * Setup the parameter array. Note that both queries use the same
	 * parameters in exactly the same order.
	 */
	params[0] = Int32GetDatum(PG_GETARG_INT32(1));

	params[1] = Int64GetDatum(apply_num_insert);
	params[2] = Int64GetDatum(apply_num_update);
	params[3] = Int64GetDatum(apply_num_delete);
	params[4] = Int64GetDatum(apply_num_truncate);
	params[5] = Int64GetDatum(apply_num_script);
	params[6] = Int64GetDatum(apply_num_insert + apply_num_update +
				   apply_num_delete + apply_num_truncate * apply_num_script);
	params[7] = PointerGetDatum(PG_GETARG_INTERVAL_P(2));
	params[8] = Int64GetDatum(apply_num_prepare);
	params[9] = Int64GetDatum(apply_num_hit);
	params[10] = Int64GetDatum(apply_num_evict);

	/*
	 * Perform the UPDATE of sl_apply_stats. If that doesn't update any
	 * row(s), try to INSERT one.
	 */
	if ((spi_rc = SPI_execp(cs->plan_apply_stats_update, params, nulls, 0)) < 0)
		elog(ERROR, "Slony-I: SPI_execp() to update apply stats failed"
			 " - rc=%d", spi_rc);
	if (SPI_processed > 0)
	{
		rc = 2;
	}
	else
	{
		if ((spi_rc = SPI_execp(cs->plan_apply_stats_insert, params, nulls, 0)) < 0)
			elog(ERROR, "Slony-I: SPI_execp() to insert apply stats failed"
				 " - rc=%d", spi_rc);
		if (SPI_processed > 0)
			rc = 1;
	}

	/*
	 * Reset statistic counters.
	 */
	apply_num_insert = 0;
	apply_num_update = 0;
	apply_num_delete = 0;
	apply_num_truncate = 0;
	apply_num_script = 0;
	apply_num_prepare = 0;
	apply_num_hit = 0;
	apply_num_evict = 0;

	/*
	 * That's it.
	 */
	SPI_finish();
	PG_RETURN_INT32(rc);
}


static uint32
applyCache_hash(const void *kp, Size ksize)
{
	char	   *key = *((char **) kp);

	return hash_any((void *) key, strlen(key));
}


static int
applyCache_cmp(const void *kp1, const void *kp2, Size ksize)
{
	char	   *key1 = *((char **) kp1);
	char	   *key2 = *((char **) kp2);

	return strcmp(key1, key2);
}


static void
applyQueryReset(void)
{
	if (applyQuery == NULL)
	{
		applyQuery = malloc(applyQuerySize);
		if (applyQuery == NULL)
			elog(ERROR, "Slony-I: applyQueryReset(): out of memory");
	}

	applyQueryPos = applyQuery;
}


static void
applyQueryIncrease(void)
{
	if (applyQueryPos - applyQuery + 1024 > applyQuerySize)
	{
		size_t		offset = applyQueryPos - applyQuery;

		applyQuerySize *= 2;
		applyQuery = realloc(applyQuery, applyQuerySize);
		if (applyQuery == NULL)
			elog(ERROR, "Slony-I: applyQueryIncrease(): out of memory");
		applyQueryPos = applyQuery + offset;
	}
}


Datum
versionFunc(lockedSet) (PG_FUNCTION_ARGS)
{
	TriggerData *tg;

	/*
	 * Get the trigger call context
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "Slony-I: lockedSet() not called as trigger");
	tg = (TriggerData *) (fcinfo->context);

	/*
	 * Check all logTrigger() calling conventions
	 */
	if (!TRIGGER_FIRED_BEFORE(tg->tg_event))
		elog(ERROR, "Slony-I: denyAccess() must be fired BEFORE");
	if (!TRIGGER_FIRED_FOR_ROW(tg->tg_event))
		elog(ERROR, "Slony-I: denyAccess() must be fired FOR EACH ROW");
	if (tg->tg_trigger->tgnargs != 1)
		elog(ERROR, "Slony-I: denyAccess() must be defined with 1 arg");

	elog(ERROR,
		 "Slony-I: Table %s is currently locked against updates "
		 "because of MOVE_SET operation in progress",
		 NameStr(tg->tg_relation->rd_rel->relname));

	return (Datum) 0;
}


Datum
versionFunc(killBackend) (PG_FUNCTION_ARGS)
{
	int32		pid;
	int32		signo;
	text	   *signame;

	if (!superuser())
		elog(ERROR, "Slony-I: insufficient privilege for killBackend");

	pid = PG_GETARG_INT32(0);
	signame = PG_GETARG_TEXT_P(1);

	if (VARSIZE(signame) == VARHDRSZ + 4 &&
		memcmp(VARDATA(signame), "NULL", 0) == 0)
	{
		signo = 0;
	}
	else if (VARSIZE(signame) == VARHDRSZ + 4 &&
			 memcmp(VARDATA(signame), "TERM", 0) == 0)
	{
		signo = SIGTERM;
	}
	else
	{
		signo = 0;
		elog(ERROR, "Slony-I: unsupported signal");
	}

	if (kill(pid, signo) < 0)
		PG_RETURN_INT32(-1);

	PG_RETURN_INT32(0);
}


typedef struct
{
	int32		seqid;
	int64		seqval;
}	SeqTrack_elem;

static int
seqtrack_cmp(void *seq1, void *seq2)
{
	return (((SeqTrack_elem *) seq1)->seqid - ((SeqTrack_elem *) seq2)->seqid);
}

static void
seqtrack_free(void *seq)
{
	free(seq);
}

Datum
versionFunc(seqtrack) (PG_FUNCTION_ARGS)
{
	static AVLtree seqmem = AVL_INITIALIZER(seqtrack_cmp, seqtrack_free);
	AVLnode    *node;
	SeqTrack_elem *elem;
	int32		seqid;
	int64		seqval;

	seqid = PG_GETARG_INT32(0);
	seqval = PG_GETARG_INT64(1);

	/*
	 * Try to insert the sequence id into the AVL tree.
	 */
	if ((node = avl_insert(&seqmem, &seqid)) == NULL)
		elog(ERROR, "Slony-I: unexpected NULL return from avl_insert()");

	if (AVL_DATA(node) == NULL)
	{
		/*
		 * This is a new (not seen before) sequence. Create the element,
		 * remember the current lastval and return it to the caller.
		 */
		elem = (SeqTrack_elem *) malloc(sizeof(SeqTrack_elem));
		elem->seqid = seqid;
		elem->seqval = seqval;
		AVL_SETDATA(node, elem);

		PG_RETURN_INT64(seqval);
	}

	/*
	 * This is a sequence seen before. If the value has changed remember and
	 * return it. If it did not, return NULL.
	 */
	elem = AVL_DATA(node);

	if (elem->seqval == seqval)
		PG_RETURN_NULL();
	else
		elem->seqval = seqval;

	PG_RETURN_INT64(seqval);
}


/*
 * slon_quote_identifier					 - Quote an identifier only if needed
 *
 * When quotes are needed, we palloc the required space; slightly
 * space-wasteful but well worth it for notational simplicity.
 *
 * Version: pgsql/src/backend/utils/adt/ruleutils.c,v 1.188 2005/01/13 17:19:10
 */
static const char *
slon_quote_identifier(const char *ident)
{
	/*
	 * Can avoid quoting if ident starts with a lowercase letter or underscore
	 * and contains only lowercase letters, digits, and underscores, *and* is
	 * not any SQL keyword.  Otherwise, supply quotes.
	 */
	int			nquotes = 0;
	bool		safe;
	const char *ptr;
	char	   *result;
	char	   *optr;

	/*
	 * would like to use <ctype.h> macros here, but they might yield unwanted
	 * locale-specific results...
	 */
	safe = ((ident[0] >= 'a' && ident[0] <= 'z') || ident[0] == '_');

	for (ptr = ident; *ptr; ptr++)
	{
		char		ch = *ptr;

		if ((ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') ||
			(ch == '_'))
		{
			/* okay */
		}
		else
		{
			safe = false;
			if (ch == '"')
				nquotes++;
		}
	}

	if (safe)
	{
		/*
		 * Check for keyword.  This test is overly strong, since many of the
		 * "keywords" known to the parser are usable as column names, but the
		 * parser doesn't provide any easy way to test for whether an
		 * identifier is safe or not... so be safe not sorry.
		 *
		 * Note: ScanKeywordLookup() does case-insensitive comparison, but
		 * that's fine, since we already know we have all-lower-case.
		 */

#ifdef SCANKEYWORDLOOKUP_1
		if (ScanKeywordLookup(ident) != NULL)
#endif
#ifdef SCANKEYWORDLOOKUP_3
			if (ScanKeywordLookup(ident, ScanKeywords, NumScanKeywords) != NULL)
#endif
				safe = false;
	}

	if (safe)
		return ident;			/* no change needed */

	result = (char *) palloc(strlen(ident) + nquotes + 2 + 1);

	optr = result;
	*optr++ = '"';
	for (ptr = ident; *ptr; ptr++)
	{
		char		ch = *ptr;

		if (ch == '"')
			*optr++ = '"';
		*optr++ = ch;
	}
	*optr++ = '"';
	*optr = '\0';

	return result;
}



/*
 * _slon_quote_ident -
 *		  returns a properly quoted identifier
 *
 * Version: pgsql/src/backend/utils/adt/quote.c,v 1.14.4.1 2005/03/21 16:29:31
 */
Datum
versionFunc(slon_quote_ident) (PG_FUNCTION_ARGS)
{
	text	   *t = PG_GETARG_TEXT_P(0);
	text	   *result;
	const char *qstr;
	char	   *str;
	int			len;

	/* We have to convert to a C string to use quote_identifier */
	len = VARSIZE(t) - VARHDRSZ;
	str = (char *) palloc(len + 1);
	memcpy(str, VARDATA(t), len);
	str[len] = '\0';

	qstr = slon_quote_identifier(str);

	len = strlen(qstr);
	result = (text *) palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), qstr, len);

	PG_RETURN_TEXT_P(result);
}



static Slony_I_ClusterStatus *
getClusterStatus(Name cluster_name, int need_plan_mask)
{
	Slony_I_ClusterStatus *cs;
	int			rc;
	char		query[1024];
	bool		isnull;
	Oid			plan_types[16];
	TypeName   *txid_snapshot_typname;

	/*
	 * Find an existing cs row for this cluster
	 */
	for (cs = clusterStatusList; cs; cs = cs->next)
	{
		if ((bool) DirectFunctionCall2(nameeq,
									   NameGetDatum(&(cs->clustername)),
									   NameGetDatum(cluster_name)) == true)
		{
			/*
			 * Return it if all the requested SPI plans are prepared already.
			 */
			if ((cs->have_plan & need_plan_mask) == need_plan_mask)
				return cs;

			/*
			 * Create more SPI plans below.
			 */
			break;
		}
	}

	if (cs == NULL)
	{
		/*
		 * No existing cs found ... create a new one
		 */
		cs = (Slony_I_ClusterStatus *) malloc(sizeof(Slony_I_ClusterStatus));
		memset(cs, 0, sizeof(Slony_I_ClusterStatus));

		/*
		 * We remember the plain cluster name for fast lookup
		 */
		strncpy(NameStr(cs->clustername), NameStr(*cluster_name), NAMEDATALEN);

		/*
		 * ... and the quoted identifier of it for building queries
		 */
		cs->clusterident = strdup(DatumGetCString(DirectFunctionCall1(textout,
											 DirectFunctionCall1(quote_ident,
																 DirectFunctionCall1(textin, CStringGetDatum(NameStr(*cluster_name)))))));

		/*
		 * Get our local node ID
		 */
		snprintf(query, 1024, "select last_value::int4 from %s.sl_local_node_id",
				 cs->clusterident);
		rc = SPI_exec(query, 0);
		if (rc < 0 || SPI_processed != 1)
			elog(ERROR, "Slony-I: failed to read sl_local_node_id");
		cs->localNodeId = DatumGetInt32(
										SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull));
		SPI_freetuptable(SPI_tuptable);
		if (cs->localNodeId < 0)
			elog(ERROR, "Slony-I: Node is uninitialized - cluster %s", DatumGetCString(cluster_name));

		/*
		 * Initialize the currentXid to invalid
		 */
		cs->currentXid = InvalidTransactionId;

		/*
		 * Insert the new control block into the list
		 */
		cs->next = clusterStatusList;
		clusterStatusList = cs;
	}

	/*
	 * Prepare and save the PLAN_INSERT_EVENT
	 */
	if ((need_plan_mask & PLAN_INSERT_EVENT) != 0 &&
		(cs->have_plan & PLAN_INSERT_EVENT) == 0)
	{
		/*
		 * Lookup the oid of the txid_snapshot type
		 */
		txid_snapshot_typname = makeNode(TypeName);
		txid_snapshot_typname->names =
			lappend(lappend(NIL, makeString("pg_catalog")),
					makeString("txid_snapshot"));

		/*
		 * Create the saved plan. We lock the sl_event table in exclusive mode
		 * in order to ensure that all events are really assigned sequence
		 * numbers in the order they get committed.
		 */
		sprintf(query,
				"INSERT INTO %s.sl_event "
				"(ev_origin, ev_seqno, "
				"ev_timestamp, ev_snapshot, "
				"ev_type, ev_data1, ev_data2, ev_data3, ev_data4, "
				"ev_data5, ev_data6, ev_data7, ev_data8) "
				"VALUES ('%d', nextval('%s.sl_event_seq'), "
				"now(), \"pg_catalog\".txid_current_snapshot(), $1, $2, "
				"$3, $4, $5, $6, $7, $8, $9); "
				"SELECT currval('%s.sl_event_seq');",
				cs->clusterident, cs->localNodeId, cs->clusterident,
				cs->clusterident);
		plan_types[0] = TEXTOID;
		plan_types[1] = TEXTOID;
		plan_types[2] = TEXTOID;
		plan_types[3] = TEXTOID;
		plan_types[4] = TEXTOID;
		plan_types[5] = TEXTOID;
		plan_types[6] = TEXTOID;
		plan_types[7] = TEXTOID;
		plan_types[8] = TEXTOID;

		cs->plan_insert_event = SPI_saveplan(SPI_prepare(query, 9, plan_types));
		if (cs->plan_insert_event == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		/*
		 * Also prepare the plan to remember sequence numbers on certain
		 * events.
		 */
		sprintf(query,
				"insert into %s.sl_seqlog "
				"(seql_seqid, seql_origin, seql_ev_seqno, seql_last_value) "
				"select * from ("
			 "select seq_id, %d, currval('%s.sl_event_seq'), seq_last_value "
				"from %s.sl_seqlastvalue "
				"where seq_origin = '%d') as FOO "
				"where NOT %s.seqtrack(seq_id, seq_last_value) IS NULL; ",
				cs->clusterident,
				cs->localNodeId, cs->clusterident,
				cs->clusterident, cs->localNodeId,
				cs->clusterident);

		cs->plan_record_sequences = SPI_saveplan(SPI_prepare(query, 0, NULL));
		if (cs->plan_record_sequences == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		cs->have_plan |= PLAN_INSERT_EVENT;
	}

	/*
	 * Prepare and save the PLAN_INSERT_LOG_STATUS
	 */
	if ((need_plan_mask & PLAN_INSERT_LOG_STATUS) != 0 &&
		(cs->have_plan & PLAN_INSERT_LOG_STATUS) == 0)
	{
		/* @-nullderef@ */

		/*
		 * Also create the 3 rather static text values for the log_cmdtype
		 * parameter.
		 */
		cs->cmdtype_I = malloc(VARHDRSZ + 1);
		SET_VARSIZE(cs->cmdtype_I, VARHDRSZ + 1);
		*VARDATA(cs->cmdtype_I) = 'I';
		cs->cmdtype_U = malloc(VARHDRSZ + 1);
		SET_VARSIZE(cs->cmdtype_U, VARHDRSZ + 1);
		*VARDATA(cs->cmdtype_U) = 'U';
		cs->cmdtype_D = malloc(VARHDRSZ + 1);
		SET_VARSIZE(cs->cmdtype_D, VARHDRSZ + 1);
		*VARDATA(cs->cmdtype_D) = 'D';

		/*
		 * And the plan to read the current log_status.
		 */
		sprintf(query, "SELECT last_value::int4 FROM %s.sl_log_status",
				cs->clusterident);
		cs->plan_get_logstatus = SPI_saveplan(SPI_prepare(query, 0, NULL));
		if (cs->plan_get_logstatus == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		cs->have_plan |= PLAN_INSERT_LOG_STATUS;
	}

	/*
	 * Prepare and save the PLAN_APPLY_QUERIES
	 */
	if ((need_plan_mask & PLAN_APPLY_QUERIES) != 0 &&
		(cs->have_plan & PLAN_APPLY_QUERIES) == 0)
	{
		/* @-nullderef@ */

		/*
		 * The plan to insert into sl_log_script.
		 */
		sprintf(query, "insert into %s.sl_log_script "
		   "(log_origin, log_txid, log_actionseq, log_cmdtype, log_cmdargs) "
				"values ($1, $2, $3, $4, $5);",
				slon_quote_identifier(NameStr(*cluster_name)));
		plan_types[0] = INT4OID;
		plan_types[1] = INT8OID;
		plan_types[2] = INT8OID;
		plan_types[3] = CHAROID;
		plan_types[4] = TEXTARRAYOID;

		cs->plan_insert_log_script = SPI_saveplan(
										  SPI_prepare(query, 5, plan_types));
		if (cs->plan_insert_log_script == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		/*
		 * The plan to lookup table forwarding info
		 */
		sprintf(query,
				"select sub_forward from "
				" %s.sl_subscribe, %s.sl_table "
				" where tab_id = $1 and tab_set = sub_set "
				" and sub_receiver = $2;",
				slon_quote_identifier(NameStr(*cluster_name)),
				slon_quote_identifier(NameStr(*cluster_name)));

		plan_types[0] = INT4OID;
		plan_types[1] = INT4OID;

		cs->plan_table_info = SPI_saveplan(
										   SPI_prepare(query, 2, plan_types));
		if (cs->plan_table_info == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		/*
		 * The plan to update the apply stats
		 */
		sprintf(query,
				"update %s.sl_apply_stats set "
				" as_num_insert = as_num_insert + $2, "
				" as_num_update = as_num_update + $3, "
				" as_num_delete = as_num_delete + $4, "
				" as_num_truncate = as_num_truncate + $5, "
				" as_num_script = as_num_script + $6, "
				" as_num_total = as_num_total + $7, "
				" as_duration = as_duration + $8, "
				" as_apply_last = \"pg_catalog\".timeofday()::timestamptz, "
				" as_cache_prepare = as_cache_prepare + $9, "
				" as_cache_hit = as_cache_hit + $10, "
				" as_cache_evict = as_cache_evict + $11, "
				" as_cache_prepare_max = case "
				"     when $9 > as_cache_prepare_max then $9 "
				"     else as_cache_prepare_max end "
				" where as_origin = $1;",
				slon_quote_identifier(NameStr(*cluster_name)));

		plan_types[0] = INT4OID;
		plan_types[1] = INT8OID;
		plan_types[2] = INT8OID;
		plan_types[3] = INT8OID;
		plan_types[4] = INT8OID;
		plan_types[5] = INT8OID;
		plan_types[6] = INT8OID;
		plan_types[7] = INTERVALOID;
		plan_types[8] = INT8OID;
		plan_types[9] = INT8OID;
		plan_types[10] = INT8OID;

		cs->plan_apply_stats_update = SPI_saveplan(
										 SPI_prepare(query, 11, plan_types));
		if (cs->plan_apply_stats_update == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		/*
		 * The plan to insert the apply stats, if update misses
		 */
		sprintf(query,
				"insert into %s.sl_apply_stats ("
				" as_origin, as_num_insert, as_num_update, as_num_delete, "
				" as_num_truncate, as_num_script, as_num_total, "
				" as_duration, as_apply_first, as_apply_last, "
				" as_cache_prepare, as_cache_hit, as_cache_evict, "
				" as_cache_prepare_max) "
				"values "
				"($1, $2, $3, $4, $5, $6, $7, $8, "
				"\"pg_catalog\".timeofday()::timestamptz, "
				"\"pg_catalog\".timeofday()::timestamptz, "
				"$9, $10, $11, $9);",
				slon_quote_identifier(NameStr(*cluster_name)));

		plan_types[0] = INT4OID;
		plan_types[1] = INT8OID;
		plan_types[2] = INT8OID;
		plan_types[3] = INT8OID;
		plan_types[4] = INT8OID;
		plan_types[5] = INT8OID;
		plan_types[6] = INT8OID;
		plan_types[7] = INTERVALOID;
		plan_types[8] = INT8OID;
		plan_types[9] = INT8OID;
		plan_types[10] = INT8OID;

		cs->plan_apply_stats_insert = SPI_saveplan(
										 SPI_prepare(query, 11, plan_types));
		if (cs->plan_apply_stats_insert == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");

		cs->have_plan |= PLAN_APPLY_QUERIES;
	}

	return cs;
	/* @+nullderef@ */
}

/**
 * prepare the plan for the curren sl_log_x insert query.
 *
 */

int
prepareLogPlan(Slony_I_ClusterStatus * cs,
			   int log_status)
{
	char		query[1024];
	Oid			plan_types[9];

	if ((log_status == 0 ||
		 log_status == 2) &&
		cs->plan_insert_log_1 == NULL)
	{
		/*
		 * Create the saved plan's
		 */
		sprintf(query, "INSERT INTO %s.sl_log_1 "
				"(log_origin, log_txid, log_tableid, log_actionseq,"
				" log_tablenspname, log_tablerelname, "
				" log_cmdtype, log_cmdupdncols, log_cmdargs) "
				"VALUES (%d, \"pg_catalog\".txid_current(), $1, "
				"nextval('%s.sl_action_seq'), $2, $3, $4, $5, $6); ",
				cs->clusterident, cs->localNodeId, cs->clusterident);
		plan_types[0] = INT4OID;
		plan_types[1] = TEXTOID;
		plan_types[2] = TEXTOID;
		plan_types[3] = TEXTOID;
		plan_types[4] = INT4OID;
		plan_types[5] = TEXTARRAYOID;

		cs->plan_insert_log_1 = SPI_saveplan(SPI_prepare(query, 6, plan_types));
		if (cs->plan_insert_log_1 == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");
	}
	else if ((log_status == 1 ||
			  log_status == 3) &&
			 cs->plan_insert_log_2 == NULL)
	{
		sprintf(query, "INSERT INTO %s.sl_log_2 "
				"(log_origin, log_txid, log_tableid, log_actionseq,"
				" log_tablenspname, log_tablerelname, "
				" log_cmdtype, log_cmdupdncols, log_cmdargs) "
				"VALUES (%d, \"pg_catalog\".txid_current(), $1, "
				"nextval('%s.sl_action_seq'), $2, $3, $4, $5, $6); ",
				cs->clusterident, cs->localNodeId, cs->clusterident);
		plan_types[0] = INT4OID;
		plan_types[1] = TEXTOID;
		plan_types[2] = TEXTOID;
		plan_types[3] = TEXTOID;
		plan_types[4] = INT4OID;
		plan_types[5] = TEXTARRAYOID;

		cs->plan_insert_log_2 = SPI_saveplan(SPI_prepare(query, 6, plan_types));
		if (cs->plan_insert_log_2 == NULL)
			elog(ERROR, "Slony-I: SPI_prepare() failed");
	}

	return 0;
}

/* Provide a way to reset the per-session data structure that stores
   the cluster status in the C functions.

 * This is used to rectify the case where CLONE NODE updates the node
 * ID, but calls to getLocalNodeId() could continue to return the old
 * value.
 */
Datum
versionFunc(resetSession) (PG_FUNCTION_ARGS)
{
	Slony_I_ClusterStatus *cs;

	cs = clusterStatusList;
	while (cs != NULL)
	{
		Slony_I_ClusterStatus *previous;

		if (cs->cmdtype_I)
			free(cs->cmdtype_I);
		if (cs->cmdtype_D)
			free(cs->cmdtype_D);
		if (cs->cmdtype_U)
			free(cs->cmdtype_D);
		free(cs->clusterident);
		if (cs->plan_insert_event)
			SPI_freeplan(cs->plan_insert_event);
		if (cs->plan_insert_log_1)
			SPI_freeplan(cs->plan_insert_log_1);
		if (cs->plan_insert_log_2)
			SPI_freeplan(cs->plan_insert_log_2);
		if (cs->plan_record_sequences)
			SPI_freeplan(cs->plan_record_sequences);
		if (cs->plan_get_logstatus)
			SPI_freeplan(cs->plan_get_logstatus);
		previous = cs;
		cs = cs->next;
		free(previous);


	}
	clusterStatusList = NULL;
	PG_RETURN_NULL();

}

/**
 * A function to decode the tgargs column of pg_trigger
 * and return an array of text objects with each trigger
 * argument.
 */
Datum
versionFunc(slon_decode_tgargs) (PG_FUNCTION_ARGS)
{
	const char *arg;
	size_t		elem_size = 0;
	ArrayType  *out_array;
	int			idx;
	bytea	   *t = PG_GETARG_BYTEA_P(0);

	int			arg_size = VARSIZE(t) - VARHDRSZ;
	const char *in_args = VARDATA(t);
	int			array_size = 0;

	out_array = construct_empty_array(TEXTOID);
	arg = in_args;

	for (idx = 0; idx < arg_size; idx++)
	{

		if (in_args[idx] == '\0')
		{
			text	   *one_arg = palloc(elem_size + VARHDRSZ);

			SET_VARSIZE(one_arg, elem_size + VARHDRSZ);
			memcpy(VARDATA(one_arg), arg, elem_size);
			out_array = array_set(out_array,
								  1, &array_size,
								  PointerGetDatum(one_arg),
								  false,
								  -1,
								  -1,
								  false,		/* typbyval for TEXT */
								  'i'	/* typalign for TEXT */
				);
			elem_size = 0;
			array_size++;
			arg = &in_args[idx + 1];
		}
		else
		{
			elem_size++;
		}
	}


	PG_RETURN_ARRAYTYPE_P(out_array);
}



/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
