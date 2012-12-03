/*-------------------------------------------------------------------------
 *
 * 
 *
 * Copyright (c) 2012, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "catalog/index.h"

#include "replication/output_plugin.h"
#include "replication/snapbuild.h"

#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/syscache.h"
#include "utils/typcache.h"


PG_MODULE_MAGIC;

void _PG_init(void);


extern void pg_decode_init(void **private);

extern bool pg_decode_begin_txn(void *private, StringInfo out, ReorderBufferTXN* txn);
extern bool pg_decode_commit_txn(void *private, StringInfo out, ReorderBufferTXN* txn, XLogRecPtr commit_lsn);
extern bool pg_decode_change(void *private, StringInfo out, ReorderBufferTXN* txn, Oid tableoid, ReorderBufferChange *change);


void
_PG_init(void)
{
}

void
pg_decode_init(void **private)
{
	AssertVariableIsOfType(&pg_decode_init, LogicalDecodeInitCB);
	*private = AllocSetContextCreate(TopMemoryContext,
									 "text conversion context",
									 ALLOCSET_DEFAULT_MINSIZE,
									 ALLOCSET_DEFAULT_INITSIZE,
									 ALLOCSET_DEFAULT_MAXSIZE);
	elog(NOTICE,"inside of pg_decode_init");
}


bool
pg_decode_begin_txn(void *private, StringInfo out, ReorderBufferTXN* txn)
{
	AssertVariableIsOfType(&pg_decode_begin_txn, LogicalDecodeBeginCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
appendStringInfo(out, "BEGIN %d", txn->xid);
	return true;
}

bool
pg_decode_commit_txn(void *private, StringInfo out, ReorderBufferTXN* txn, XLogRecPtr commit_lsn)
{
	AssertVariableIsOfType(&pg_decode_commit_txn, LogicalDecodeCommitCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
	appendStringInfo(out, "COMMIT %d", txn->xid);
	return true;
}



bool
pg_decode_change(void *private, StringInfo out, ReorderBufferTXN* txn,
				 Oid tableoid, ReorderBufferChange *change)
{

  
	Relation relation = RelationIdGetRelation(tableoid);
	Form_pg_class class_form = RelationGetForm(relation);
	TupleDesc	tupdesc = RelationGetDescr(relation);
	MemoryContext context = (MemoryContext)private;
	MemoryContext old = MemoryContextSwitchTo(context);
	elog(NOTICE,"inside og pg_decode_change");
	
	RelationClose(relation);

	MemoryContextSwitchTo(old);
	MemoryContextReset(context);
appendStringInfo(out, "DO %d", txn->xid);
	return true;
}


