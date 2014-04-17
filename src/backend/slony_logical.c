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
#include "catalog/namespace.h"
#include "access/sysattr.h"

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
#include "utils/fmgroids.h"
#include "fmgr.h"
#include "access/hash.h"
#include "replication/logical.h"
#include "replication/snapbuild.h"
#include "nodes/makefuncs.h"
#include "commands/defrem.h"
#include "utils/snapmgr.h"

#if 0 
PG_MODULE_MAGIC;
#endif

extern void pg_decode_init(LogicalDecodingContext * ctx, OutputPluginOptions * options, bool is_init);

extern void pg_decode_begin_txn(LogicalDecodingContext * ctx, 
								ReorderBufferTXN* txn);
extern void pg_decode_commit_txn(LogicalDecodingContext * ctx,
								 ReorderBufferTXN* txn, XLogRecPtr commit_lsn);
extern void pg_decode_change(LogicalDecodingContext * ctx, 
							 ReorderBufferTXN* txn,
							 Relation relation, ReorderBufferChange *change);

extern bool pg_decode_clean(LogicalDecodingContext * ctx);
extern void
_PG_output_plugin_init(OutputPluginCallbacks * cb);
extern void pg_decode_shutdown(LogicalDecodingContext * ctx);

char * columnAsText(TupleDesc tupdesc, HeapTuple tuple,int idx);

static int 
lookupSlonyInfo(Oid tableOid,LogicalDecodingContext * ctx,
				int * origin_id, int * table_id,int * set_id);


unsigned int local_id=0;
char * cluster_name=NULL;

HTAB * replicated_tables=NULL;

typedef struct  {
	const char * key;
	const char * namespace;
	const char * table_name;
	int set;

} replicated_table;

#if 0 
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

static bool is_replicated(const char * namespace,const char * table);
static int lookup_origin(const char * namespace, const char * table);
#endif

void
_PG_output_plugin_init(OutputPluginCallbacks * cb)
{
	cb->startup_cb = pg_decode_init;
	cb->begin_cb = pg_decode_begin_txn;
	cb->commit_cb = pg_decode_commit_txn;
	cb->change_cb = pg_decode_change;
	cb->shutdown_cb = pg_decode_shutdown;
}


void pg_decode_shutdown(LogicalDecodingContext * ctx)
{
	/**
	 * free  /ctx->output_plugin_private
	 */
}



void
pg_decode_init(LogicalDecodingContext * ctx,OutputPluginOptions *options, bool is_init)
{	
	ListCell * option;
	elog(NOTICE,"is_init is %d",is_init==1);

	ctx->output_plugin_private = AllocSetContextCreate(TopMemoryContext,
									 "slony logical  context",
									 ALLOCSET_DEFAULT_MINSIZE,
									 ALLOCSET_DEFAULT_INITSIZE,
									 ALLOCSET_DEFAULT_MAXSIZE);


       	AssertVariableIsOfType(&pg_decode_init, LogicalDecodeStartupCB);
	MemoryContext context = (MemoryContext)ctx->output_plugin_private;
	MemoryContext old = MemoryContextSwitchTo(context);
											

	if (ctx->output_plugin_options != NULL) 
	{
	
		option=ctx->output_plugin_options->head;
	
		while(option != NULL )
		{

			DefElem * def_option = (DefElem * ) option->data.ptr_value;

			
			if( strcmp(def_option->defname,"cluster") == 0)
			{
				const char * value = defGetString(def_option);
				cluster_name = palloc(strlen(value)+1);
				strncpy(cluster_name,value,strlen(value));
			}
			option=option->next;
		}
	}

	if( is_init==false && cluster_name==NULL )
	{				
		
		elog(ERROR,"cluster name must be specified");

	}

	MemoryContextSwitchTo(old);
}


void
pg_decode_begin_txn(LogicalDecodingContext * ctx, ReorderBufferTXN* txn)
{
	AssertVariableIsOfType(&pg_decode_begin_txn, LogicalDecodeBeginCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */

}

void
pg_decode_commit_txn( LogicalDecodingContext * ctx,
					 ReorderBufferTXN* txn, XLogRecPtr commit_lsn)
{
	AssertVariableIsOfType(&pg_decode_commit_txn, LogicalDecodeCommitCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
}



void
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
	const char * table_name;
	const char * namespace;
	int origin_id=0;
	int table_id=0;
	int set_id=0;

	old = MemoryContextSwitchTo(context);
	OutputPluginPrepareWrite(ctx,true);
	

	namespace=quote_identifier(get_namespace_name(class_form->relnamespace));
	table_name=quote_identifier(NameStr(class_form->relname));
	lookupSlonyInfo(relation->rd_id,ctx, &origin_id,&table_id,&set_id);
	if( origin_id <= 0 ) 
	{

		Oid slony_namespace;
		Oid slevent_oid;
		Oid slconfirm_oid;
		Oid slseqlog_oid;
		AttrNumber origin_attnum = InvalidAttrNumber;
		char * schema_name;

		/**
		 * The table is not replicated but
		 * we send along changes to sl_event
		 * and sl_confirm.
		 */
		schema_name = palloc(strlen(cluster_name)+4);
		sprintf(schema_name,"_%s",cluster_name);
		slony_namespace = get_namespace_oid(schema_name,false);

		/**
		 * open 
		 *
		 */
		slevent_oid = get_relname_relid("sl_event",slony_namespace);
		slconfirm_oid = get_relname_relid("sl_confirm",slony_namespace);
		slseqlog_oid = get_relname_relid("sl_seqlog",slony_namespace);

		if(slevent_oid == relation->rd_id &&
		   change->action == REORDER_BUFFER_CHANGE_INSERT)
		{

			/**
			 * extract ev_origin from the tuple.
			 */
			origin_attnum = get_attnum(relation->rd_id,"ev_origin");
			
		}
#if 0 
		else if (slconfirm_oid == relation->rd_id &&
				 change->action == REORDER_BUFFER_CHANGE_INSERT)
		{
			/**
			 * extract con_origin from the tuple
			 */
			origin_attnum = get_attnum(relation->rd_id,"con_received");			
		}
#endif
		else if (slseqlog_oid == relation->rd_id &&
				 change->action == REORDER_BUFFER_CHANGE_INSERT)
		{
			/**
			 * 
			 */
			
			origin_attnum = get_attnum(relation->rd_id,"seql_origin");	
		}
		if(origin_attnum != InvalidAttrNumber)
		{
			bool isnull;
			Datum storedValue = fastgetattr(&change->data.tp.newtuple->tuple,
											origin_attnum,tupdesc,&isnull);
			if( ! isnull )
				origin_id  = DatumGetInt32(storedValue);
		}
	}

	if( origin_id <= 0 )
	{

		MemoryContextSwitchTo(old);
		return;
	}
	if(change->action == REORDER_BUFFER_CHANGE_INSERT)
	{		
		/**
		 * convert all columns to a pair of arrays (columns and values)
		 */
		tuple=&change->data.tp.newtuple->tuple;
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
			const char * value_new;

			if(tupdesc->attrs[i]->attisdropped)
				continue;
			if(tupdesc->attrs[i]->attnum < 0)
				continue;
			column= NameStr(tupdesc->attrs[i]->attname);

			
			value_new = columnAsText(tupdesc,&change->data.tp.newtuple->tuple,i);		
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
		
		if(change->data.tp.oldtuple == NULL)
		{
			/**
			 * If oldtuple is NULL then no columns
			 * that are in the REPLICA IDENTITY have changed.
			 * Get the columns in the replica identity from the relation and
			 * use the values in newtuple to make up identity values.
			 */
			Bitmapset * id_attrs;
			const char * column;			
			const char * value;
			id_attrs = RelationGetIndexAttrBitmap(relation,
												  INDEX_ATTR_BITMAP_IDENTITY_KEY);
			for(i = 0; i < relation->rd_att->natts; i++)
			{
				if( bms_is_member(i-FirstLowInvalidHeapAttributeNumber+1,id_attrs))
				{
					column= NameStr(tupdesc->attrs[i]->attname);
					value = columnAsText(tupdesc,&change->data.tp.newtuple->tuple,i);		
					if (value == NULL) 
					{
						continue;
					}
					*cmdargselem++=PointerGetDatum(cstring_to_text(column));
					*cmdnullselem++=false;			
					*cmdnullselem++=false;
					*cmdargselem++=PointerGetDatum(cstring_to_text(value));																	
					
				}			
			}
		}
		else 
		{
			/**
			 * At least one of the columns that make  up the replica
			 * identity has changed.
			 * Use the columns in oldtuple as the replica identity.
			 */
			for(i = 0; i < relation->rd_att->natts; i++)
			{
				const char * column;			
				const char * value_old;
				
				if(tupdesc->attrs[i]->attisdropped)
					continue;
				if(tupdesc->attrs[i]->attnum < 0)
					continue;
				column= NameStr(tupdesc->attrs[i]->attname);
				
				value_old = columnAsText(tupdesc,&change->data.tp.oldtuple->tuple,i);		
				if (value_old == NULL) 
				{
					continue;
				}
				*cmdargselem++=PointerGetDatum(cstring_to_text(column));
				*cmdnullselem++=false;			
				*cmdnullselem++=false;
				*cmdargselem++=PointerGetDatum(cstring_to_text(value_old));
		   			
				
				
				
			}
		}

	}
	else if (change->action == REORDER_BUFFER_CHANGE_DELETE)
	{
	  /**
	   * convert the key columns to a pair of arrays.
	   */
	  action='D';
	  tuple=&change->data.tp.oldtuple->tuple;
	  
	  

	  cmdargs = cmdargselem = palloc( (relation->rd_att->natts * 2 + 2)
									  * sizeof (Datum));
	  cmdnulls = cmdnullselem = palloc( (relation->rd_att->natts * 2 + 2)
										* sizeof(bool));
	  for(i = 0; i < relation->rd_att->natts; i++)
	  {
		  const char * column;
		  const char * value;
		  
		  if(tupdesc->attrs[i]->attisdropped)
			  /** you can't drop a column from an index, something is wrong */
			  continue;
		  if(tupdesc->attrs[i]->attnum < 0)
			  continue;
		  value = columnAsText(tupdesc,tuple,i);
		  if ( value == NULL)
			  continue;
		  column = NameStr(tupdesc->attrs[i]->attname);
		  *cmdargselem++= PointerGetDatum(cstring_to_text(column));
		  *cmdnullselem++=false;
		
		  *cmdnullselem++=false;
		  *cmdargselem++=PointerGetDatum(cstring_to_text(value));
	  }
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
	appendStringInfo(ctx->out,"%d\t%u\t%d\t%u\t%s\t%s\t%c\t%d\t%s"
					 ,origin_id
					 ,txn->xid
					 ,table_id
					 ,0 /*actionseq*/
					 ,namespace
					 ,table_name
					 ,action
					 ,cmdnupdates
					 ,array_text);


	
	
	
	MemoryContextSwitchTo(old);
	OutputPluginWrite(ctx,true);
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
	bool isnull;
	HeapTuple typeTuple;
	Datum origval,val;
	char * outputstr=NULL;

	typid = tupdesc->attrs[idx]->atttypid;

	typeTuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(typid));
	
	if(!HeapTupleIsValid(typeTuple)) 
		elog(ERROR, "cache lookup failed for type %u", typid);
	
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


#if 0 
/**
 * checks to see if the table described by class_form is
 * replicated from this origin/provider to the recevier
 * we are running for.
 */
bool is_replicated(const char * namespace,const char * table)
{
	char * search_key = palloc(strlen(namespace) + strlen(table)+2);
	bool found;

	sprintf(search_key,"%s.%s",namespace,table);
    entry=hash_search(replicated_tables,
					  &search_key,HASH_FIND,&found);
	
	return found;
	
}
#endif

static int 
lookupSlonyInfo(Oid tableOid,LogicalDecodingContext * ctx, int * origin_id,
				int * table_id,int * set_id)
{
  Oid sltable_oid;
  Oid slony_namespace;
  Relation sltable_rel;
  HeapScanDesc scandesc;
  HeapTuple tuple;
  TupleDesc tupdesc;
  int cnt = 0;
  int retval=-1;
  AttrNumber reloid_attnum;
  AttrNumber set_attnum;
  AttrNumber tableid_attnum;
  char * schema_name;

  /**
   * search for the table in sl_table based on the tables
   * oid.
   *
   * We don't use the tables name because the previous SQL command
   * might have been a ALTER TABLE RENAME TO ...
   * 
   * 
   */
  *origin_id=-1;
  *table_id=-1;

  schema_name = palloc(strlen(cluster_name)+4);
  sprintf(schema_name,"_%s",cluster_name);
  slony_namespace = get_namespace_oid(schema_name,false);

  /**
   * open 
   *
   */
  sltable_oid = get_relname_relid("sl_table",slony_namespace);
  
  sltable_rel = relation_open(sltable_oid,AccessShareLock);
  tupdesc=RelationGetDescr(sltable_rel);  
  scandesc=heap_beginscan(sltable_rel, GetCatalogSnapshot(sltable_oid),0,NULL);
  reloid_attnum = get_attnum(sltable_oid,"tab_reloid");

  if(reloid_attnum == InvalidAttrNumber)
	  elog(ERROR,"sl_table does not have a tab_reloid column");
  set_attnum = get_attnum(sltable_oid,"tab_set");

  if(set_attnum == InvalidAttrNumber)
	  elog(ERROR,"sl_table does not have a tab_set column");
  tableid_attnum = get_attnum(sltable_oid, "tab_id");

  if(tableid_attnum == InvalidAttrNumber)
	  elog(ERROR,"sl_table does not have a tab_id column");
  
  /**
   * TODO: Make this more efficient, use a cache? or indexes?
   */
  while( (tuple = heap_getnext(scandesc,ForwardScanDirection) ))
  {
	  
	  Datum storedValue;
	  bool isnull;
	  ScanKeyData setKey[1];
	  Relation slset_rel;
	  Oid slset_oid;
	  HeapTuple slset_tuple;
	  TupleDesc slset_tupdesc;
	  AttrNumber origin_attnum;
	  AttrNumber slset_setid_attnum;
	  HeapScanDesc slset_scandesc;

	  slset_oid = get_relname_relid("sl_set",slony_namespace);

	  if ( tupdesc->attrs[reloid_attnum-1]->atttypid != OIDOID )
		  elog(ERROR,"sl_table.tab_reloid should have type %d but found %d %d"
			   , OIDOID, tupdesc->attrs[reloid_attnum-1]->atttypid,
			   reloid_attnum);
	  origin_attnum = get_attnum(slset_oid,"set_origin");
	  
	  slset_setid_attnum = get_attnum(slset_oid,"set_id");

	  if(origin_attnum == InvalidAttrNumber)
		  elog(ERROR,"sl_set does not have a set_origin column");
	  
	  storedValue = fastgetattr(tuple,reloid_attnum,tupdesc,&isnull);
	  if( DatumGetObjectId(storedValue) == tableOid )
	  {
		  /**
		   * a match.
		   *
		   */
		  retval = DatumGetInt32(fastgetattr(tuple,tableid_attnum,
											 tupdesc,&isnull));

		  if ( isnull )
			  elog(ERROR,"tab_id is null for table with oid %d",
				   tableOid);	
		  *table_id = retval;
		  
		  /*
		   * find the sl_set column.
		   */
		  retval = DatumGetInt32(fastgetattr(tuple,set_attnum,
											tupdesc,&isnull));
		  if ( isnull )
			  elog(ERROR,"tab_set is null for table with oid %d",
				  tableOid);
		  *set_id = retval;
		  /**
		   * Now find the current origin for that table
		   * in sl_set
		   */
		  ScanKeyInit(&setKey[0],slset_setid_attnum,
					  BTEqualStrategyNumber , /* StrategyNumber */
					  F_INT4EQ, /* RegProcedure */
					  Int32GetDatum(retval));
		  slset_oid = get_relname_relid("sl_set",slony_namespace);
		  slset_rel = relation_open(slset_oid,AccessShareLock);
		  slset_tupdesc = RelationGetDescr(slset_rel);
		  slset_scandesc = heap_beginscan(slset_rel,
										  GetCatalogSnapshot(slset_oid),
										  1,setKey);
		  
		  slset_tuple = heap_getnext(slset_scandesc,ForwardScanDirection);
		  if (! slset_tuple )
			  elog(ERROR,"can't find set %d in sl_set",retval);
		  /**
		   * extract the set origin from sl_set.
		   */
		  storedValue = fastgetattr(slset_tuple,origin_attnum,slset_tupdesc,
									&isnull);
		  if ( isnull)
			  elog(ERROR,"set origin is null for set %d", retval);
		  *origin_id = DatumGetInt32(storedValue);
		  heap_endscan(slset_scandesc);
		  relation_close(slset_rel,AccessShareLock);
		  break;
	  }
	  cnt++;
  }
  heap_endscan(scandesc);

  relation_close(sltable_rel,AccessShareLock);
  return retval;
}
