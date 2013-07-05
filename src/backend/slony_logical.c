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
#include "utils/array.h"
#include "utils/builtins.h"
#include "fmgr.h"
#include "access/hash.h"
#include "replication/logical.h"
#include "nodes/makefuncs.h"
#include "commands/defrem.h"

#if 0 
PG_MODULE_MAGIC;
#endif
void _PG_init(void);


extern void pg_decode_init(LogicalDecodingContext * ctx, bool is_init);

extern bool pg_decode_begin_txn(LogicalDecodingContext * ctx, 
								ReorderBufferTXN* txn);
extern bool pg_decode_commit_txn(LogicalDecodingContext * ctx,
								 ReorderBufferTXN* txn, XLogRecPtr commit_lsn);
extern bool pg_decode_change(LogicalDecodingContext * ctx, 
							 ReorderBufferTXN* txn,
							 Relation relation, ReorderBufferChange *change);

extern bool pg_decode_clean(LogicalDecodingContext * ctx);

char * columnAsText(TupleDesc tupdesc, HeapTuple tuple,int idx);

unsigned int local_id=0;

HTAB * replicated_tables=NULL;

typedef struct  {
	const char * key;
	const char * namespace;
	const char * table_name;
	int set;

} replicated_table;

static uint32
replicated_table_hash(const void *kp, Size ksize)
{
	char	   *key = *((char **) kp);
	return hash_any((void *) key, strlen(key));
}
static int
replicated_table_cmp(const void *kp1, const void *kp2, Size ksize)
{
	char	   *key1 = *((char **) kp1);
	char	   *key2 = *((char **) kp2);
	return strcmp(key1, key2);
}

bool is_replicated(const char * namespace,const char * table);


void
_PG_init(void)
{
}




void
pg_decode_init(LogicalDecodingContext * ctx, bool is_init)
{	
	char * table;
	bool found;
	HASHCTL hctl;
	ListCell * option;
	

	ctx->output_plugin_private = AllocSetContextCreate(TopMemoryContext,
									 "slony logical  context",
									 ALLOCSET_DEFAULT_MINSIZE,
									 ALLOCSET_DEFAULT_INITSIZE,
									 ALLOCSET_DEFAULT_MAXSIZE);


	AssertVariableIsOfType(&pg_decode_init, LogicalDecodeInitCB);
	MemoryContext context = (MemoryContext)ctx->output_plugin_private;
	MemoryContext old = MemoryContextSwitchTo(context);
											

	/**
	 * query the local database to find
	 * 1. the local_id
	 */
	elog(NOTICE,"inside of pg_decode_init");
	hctl.keysize = sizeof(char*);
	hctl.entrysize = sizeof(replicated_table);
	hctl.hash = replicated_table_hash;
	hctl.match = replicated_table_cmp;


	/**
	 * build a hash table with information on all replicated tables.
	 */
	replicated_tables=hash_create("replicated_tables",10,&hctl,
								  HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);
	if (ctx->output_plugin_options != NULL) 
	{


	
		option=ctx->output_plugin_options->head;
	
		while(option != NULL && option->next != NULL)
		{

			DefElem * def_schema = (DefElem * ) option->data.ptr_value;
			DefElem * def_table = (DefElem *) option->next->data.ptr_value;
			
			const char * schema= defGetString(def_schema);
			const char * table_name = defGetString(def_table);
			
			table = palloc(strlen(schema) + strlen(table_name)+2);
			sprintf(table,"%s.%s",schema,table_name);
			replicated_table * entry=hash_search(replicated_tables,
												 &table,HASH_ENTER,&found);
			entry->key=table;
			entry->namespace=pstrdup(schema);
			entry->table_name=pstrdup(table_name);
			entry->set=1;
			option=option->next->next;
		}
	}

	MemoryContextSwitchTo(old);
}


bool
pg_decode_begin_txn(LogicalDecodingContext * ctx, ReorderBufferTXN* txn)
{
	AssertVariableIsOfType(&pg_decode_begin_txn, LogicalDecodeBeginCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
	elog(NOTICE,"inside of begin");
	ctx->prepare_write(ctx,txn->lsn,txn->xid);
	appendStringInfo(ctx->out, "BEGIN %d", txn->xid);
	ctx->write(ctx,txn->lsn,txn->xid);
	return true;
}

bool
pg_decode_commit_txn( LogicalDecodingContext * ctx,
					 ReorderBufferTXN* txn, XLogRecPtr commit_lsn)
{
	AssertVariableIsOfType(&pg_decode_commit_txn, LogicalDecodeCommitCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
	elog(NOTICE,"inside of commit");
	ctx->prepare_write(ctx,txn->lsn,txn->xid);
	appendStringInfo(ctx->out, "COMMIT %d", txn->xid);
	ctx->write(ctx,txn->lsn,txn->xid);
	return true;
}



bool
pg_decode_change(LogicalDecodingContext * ctx, ReorderBufferTXN* txn,
				 Relation relation, ReorderBufferChange *change)
{

	
	TupleDesc	tupdesc = RelationGetDescr(relation);
	Form_pg_class class_form = RelationGetForm(relation);
	MemoryContext context = (MemoryContext)ctx->output_plugin_private;
	MemoryContext old;
	int i;
	HeapTuple tuple;
	/**
	 * we build up an array of Datum's so we can convert this
	 * to an array and use array_out to get a text representation.
	 * it might be more efficient to leave everything as 
	 * cstrings and write our own quoting/escaping code
	 */
	Datum		*cmdargs = NULL;
	Datum		*cmdargselem = NULL;
	bool	   *cmdnulls = NULL;
	bool	   *cmdnullselem = NULL;
	int        cmdnupdates=0;
	int		   cmddims[1];
	int        cmdlbs[1];
	ArrayType  *outvalues;
	const char * array_text;
	Oid arraytypeoutput;
	bool  arraytypeisvarlena;
	HeapTuple array_type_tuple;
	FmgrInfo flinfo;
	char action='?';
	int update_cols=0;
	int table_id=0;
	const char * table_name;
	const char * namespace;

	old = MemoryContextSwitchTo(context);
	ctx->prepare_write(ctx,txn->lsn,txn->xid);
	elog(NOTICE,"inside og pg_decode_change");

	
	namespace=get_namespace_name(class_form->relnamespace);
	table_name=NameStr(class_form->relname);

	if( ! is_replicated(namespace,table_name) ) 
	{
		MemoryContextSwitchTo(old);
		return false;
	}

	if(change->action == REORDER_BUFFER_CHANGE_INSERT)
	{		
		/**
		 * convert all columns to a pair of arrays (columns and values)
		 */
		tuple=&change->newtuple->tuple;
		action='I';
		cmdargs = cmdargselem = palloc( (relation->rd_att->natts * 2 +2) 
										* sizeof(Datum)  );
		cmdnulls = cmdnullselem = palloc( (relation->rd_att->natts *2 + 2) 
										  * sizeof(bool));
		
		

		for(i = 0; i < relation->rd_att->natts; i++)
		{
			const char * column;			
			const char * value;

			if(tupdesc->attrs[i]->attisdropped)
				continue;
			if(tupdesc->attrs[i]->attnum < 0)
				continue;
			column= NameStr(tupdesc->attrs[i]->attname);
			*cmdargselem++=PointerGetDatum(cstring_to_text(column));
			*cmdnullselem++=false;
			
			value = columnAsText(tupdesc,tuple,i);
			if (value == NULL) 
			{
			  *cmdnullselem++=true;
			  cmdargselem++;
			}
			else
			{
				*cmdnullselem++=false;
				*cmdargselem++=PointerGetDatum(cstring_to_text(value));
			}			
			
		}   
		
		
	}
	else if (change->action == REORDER_BUFFER_CHANGE_UPDATE)
	{

		Relation indexrel;
		TupleDesc indexdesc;
		/**
		 * convert all columns into two pairs of arrays.
		 * one for key columns, one for non-key columns
		 */
		action='U';
		
		cmdargs = cmdargselem = palloc( (relation->rd_att->natts * 4 +2) 
										* sizeof(Datum)  );
		cmdnulls = cmdnullselem = palloc( (relation->rd_att->natts *4 + 2) 
										  * sizeof(bool));			
	  
		/**
		 * compare the old and new tuples looking at each column.
		 * If the value of the column has changed then we add that
		 * to the list.
		 */

		for(i = 0; i < relation->rd_att->natts; i++)
		{
			const char * column;			
			const char * value_old;
			const char * value_new;

			if(tupdesc->attrs[i]->attisdropped)
				continue;
			if(tupdesc->attrs[i]->attnum < 0)
				continue;
			column= NameStr(tupdesc->attrs[i]->attname);

			
			value_new = columnAsText(tupdesc,&change->newtuple->tuple,i);		
			*cmdargselem++=PointerGetDatum(cstring_to_text(column));
			*cmdnullselem++=false;
			
			if (value_new == NULL) 
			{
				*cmdnullselem++=true;
				cmdargselem++;
			}
			else
			{
				*cmdnullselem++=false;
				*cmdargselem++=PointerGetDatum(cstring_to_text(value_new));
			}			
			cmdnupdates++;					
		}
		
		/**
		 * populate relation->rd_primary with the primary or candidate
		 * index used to WAL values that specify which row is being updated.
		 */
		RelationGetIndexList(relation);
				
		indexrel = RelationIdGetRelation(relation->rd_primary);
		indexdesc = RelationGetDescr(indexrel);
	    for(i = 0; i < indexrel->rd_att->natts; i++)
		{
			const char * column;
			const char * value;
			
			if(indexdesc->attrs[i]->attisdropped)
				/** you can't drop a column from an index, something is wrong */
				continue;
			if(indexdesc->attrs[i]->attnum < 0)
				continue;

			column = NameStr(indexdesc->attrs[i]->attname);
			*cmdargselem++= PointerGetDatum(cstring_to_text(column));
			*cmdnullselem++=false;

			if(change->oldtuple != NULL )
				value = columnAsText(indexdesc,&change->oldtuple->tuple,i);
			else
				value = columnAsText(indexdesc,&change->newtuple->tuple,i);
			*cmdnullselem++=false;
			*cmdargselem++=PointerGetDatum(cstring_to_text(value));
		}
		RelationClose(indexrel);
	}
	else if (change->action == REORDER_BUFFER_CHANGE_DELETE)
	{
	  Relation indexrel;
	  TupleDesc indexdesc;
		/**
		 * convert the key columns to a pair of arrays.
		 */
	  action='D';
	  tuple=&change->oldtuple->tuple;
	  
	  /**
	   * populate relation->rd_primary with the primary or candidate
	   * index used to WAL values that specify which row is being deleted.
	   */
	  RelationGetIndexList(relation);

	  indexrel = RelationIdGetRelation(relation->rd_primary);
	  indexdesc = RelationGetDescr(indexrel);
	  

	  cmdargs = cmdargselem = palloc( (indexrel->rd_att->natts * 2 + 2)
									  * sizeof (Datum));
	  cmdnulls = cmdnullselem = palloc( (indexrel->rd_att->natts * 2 + 2)
										* sizeof(bool));
	  for(i = 0; i < indexrel->rd_att->natts; i++)
	  {
		  const char * column;
		  const char * value;
		  
		  if(indexdesc->attrs[i]->attisdropped)
			  /** you can't drop a column from an index, something is wrong */
			  continue;
		  if(indexdesc->attrs[i]->attnum < 0)
			  continue;
		  column = NameStr(indexdesc->attrs[i]->attname);
		  *cmdargselem++= PointerGetDatum(cstring_to_text(column));
		  *cmdnullselem++=false;
		  value = columnAsText(indexdesc,tuple,i);
		  *cmdnullselem++=false;
		  *cmdargselem++=PointerGetDatum(cstring_to_text(value));
	  }
	  RelationClose(indexrel);
	}
	else
	{
		/**
		 * what else?
		 */
	}   



	cmddims[0] = cmdargselem - cmdargs;
	cmdlbs[0] = 1;
	outvalues= construct_md_array(cmdargs,cmdnulls,1,cmddims,cmdlbs,
								 TEXTOID,-1,false,'i');
	array_type_tuple = SearchSysCache1(TYPEOID,TEXTARRAYOID);
	array_type_tuple->t_tableOid = InvalidOid;
	getTypeOutputInfo(TEXTARRAYOID,&arraytypeoutput,&arraytypeisvarlena);
	fmgr_info(arraytypeoutput,&flinfo);
	array_text = DatumGetCString(FunctionCall1Coll(&flinfo,InvalidOid,
												   PointerGetDatum(outvalues)));
	ReleaseSysCache(array_type_tuple);
	appendStringInfo(ctx->out,"%d,%d,%d,NULL,%s,%s,%c,%d,%s"
					 ,local_id
					 ,txn->xid
					 ,table_id
					 ,namespace
					 ,table_name
					 ,action
					 ,update_cols
					 ,array_text);


	
	
	
	MemoryContextSwitchTo(old);
	ctx->write(ctx,txn->lsn,txn->xid);
	elog(NOTICE,"leaving og pg_decode_change:");
	return true;
}


bool pg_decode_clean(LogicalDecodingContext * ctx)
{
	return true;
}

/**
 * converts the value stored in the attribute/column specified
 * to a text string.  If the value is NULL then a NULL is returned.
 */
char * columnAsText(TupleDesc tupdesc, HeapTuple tuple,int idx)
{
	Oid typid,typeoutput;
	bool		typisvarlena;
	Form_pg_type ftype;
	bool isnull;
	HeapTuple typeTuple;
	Datum origval,val;
	char * outputstr=NULL;

	typid = tupdesc->attrs[idx]->atttypid;

	typeTuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(typid));
	
	if(!HeapTupleIsValid(typeTuple)) 
		elog(ERROR, "cache lookup failed for type %u", typid);
	ftype = (Form_pg_type) GETSTRUCT(typeTuple);
	
	getTypeOutputInfo(typid,
					  &typeoutput, &typisvarlena);

	ReleaseSysCache(typeTuple);
	
	origval = fastgetattr(tuple, idx + 1, tupdesc, &isnull);
	if(typisvarlena && !isnull) 
		val = PointerGetDatum(PG_DETOAST_DATUM(origval));
	else
		val = origval;
	if (isnull)
		return NULL;
	outputstr = OidOutputFunctionCall(typeoutput, val);
	return outputstr;
}

/**
 * checks to see if the table described by class_form is
 * replicated from this origin/provider to the recevier
 * we are running for.
 */
bool is_replicated(const char * namespace,const char * table)
{
	char * search_key = palloc(strlen(namespace) + strlen(table)+2);
	replicated_table * entry;
	bool found;

	sprintf(search_key,"%s.%s",namespace,table);
    entry=hash_search(replicated_tables,
					  &search_key,HASH_FIND,&found);
	
	return found;
	
}
