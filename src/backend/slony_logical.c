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

PG_MODULE_MAGIC;

void _PG_init(void);


extern void pg_decode_init(void **private);

extern bool pg_decode_begin_txn(void *private, StringInfo out, ReorderBufferTXN* txn);
extern bool pg_decode_commit_txn(void *private, StringInfo out, ReorderBufferTXN* txn, XLogRecPtr commit_lsn);
extern bool pg_decode_change(void *private, StringInfo out, ReorderBufferTXN* txn, Oid tableoid, ReorderBufferChange *change);

char * columnAsText(TupleDesc tupdesc, HeapTuple tuple,int idx);

unsigned int local_id=0;

HTAB * replicated_tables=NULL;

typedef struct  {
	const char * key;
	const char * namespace;
	const char * table_name;
	int set;
	const char* unique_keyname;

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
pg_decode_init(void **private)
{	
	const char * table;
	bool found;
	HASHCTL hctl;

	AssertVariableIsOfType(&pg_decode_init, LogicalDecodeInitCB);
	*private = AllocSetContextCreate(TopMemoryContext,
									 "text conversion context",
									 ALLOCSET_DEFAULT_MINSIZE,
									 ALLOCSET_DEFAULT_INITSIZE,
									 ALLOCSET_DEFAULT_MAXSIZE);
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
	table=pstrdup("public.a");
	replicated_table * entry=hash_search(replicated_tables,
										 &table,HASH_ENTER,&found);
	entry->key=table;
	entry->namespace="public";
	entry->table_name="a";
	entry->set=1;
	entry->unique_keyname="a_pk";
	
	table="public.b";
	entry=hash_search(replicated_tables,
					  &table,HASH_ENTER,&found);
	entry->key=table;
	entry->namespace="public";
	entry->table_name="b";
	entry->set=1;
	entry->unique_keyname="b_pkey";
	

}


bool
pg_decode_begin_txn(void *private, StringInfo out, ReorderBufferTXN* txn)
{
	AssertVariableIsOfType(&pg_decode_begin_txn, LogicalDecodeBeginCB);
	/**
	 * we can ignore the begin and commit. slony operates
	 * on SYNC boundaries.
	 */
	elog(NOTICE,"inside of begin");
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
	elog(NOTICE,"inside of commit");
	appendStringInfo(out, "COMMIT %d", txn->xid);
	return true;
}



bool
pg_decode_change(void *private, StringInfo out, ReorderBufferTXN* txn,
				 Oid tableoid, ReorderBufferChange *change)
{

	
	Relation relation = RelationIdGetRelation(tableoid);	
	TupleDesc	tupdesc = RelationGetDescr(relation);
	Form_pg_class class_form = RelationGetForm(relation);
	MemoryContext context = (MemoryContext)private;
	MemoryContext old = MemoryContextSwitchTo(context);
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
	int		   cmddims[1];
	int        cmdlbs[1];
	ArrayType  *outvalues;
	const char * array_text;
	Oid arraytypeoutput;
	bool  arraytypeisvarlena;
	bool       first=true;
	HeapTuple array_type_tuple;
	FmgrInfo flinfo;
	char action;
	int update_cols=0;
	int table_id=0;
	const char * table_name;
	const char * namespace;
	
	elog(NOTICE,"inside og pg_decode_change");

	
	namespace=get_namespace_name(class_form->relnamespace);
	table_name=NameStr(class_form->relname);

	if( ! is_replicated(namespace,table_name) ) 
	{
		return false;
	}

	if(change->action == REORDER_BUFFER_CHANGE_INSERT)
	{		
		/**
		 * convert all columns to a pair of arrays (columns and values)
		 */
		tuple=&change->newtuple->tuple;
		action='I';
		cmdargs = cmdargselem = palloc( (relation->rd_att->natts * 2 +2) * sizeof(Datum)  );
		cmdnulls = cmdnullselem = palloc( (relation->rd_att->natts *2 + 2) * sizeof(bool));
		
		

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
		
		/**
		 * we need to fine the columns that make up the unique key.
		 * in the log trigger these were arguments to the trigger
		 * ie (kvvkk).  
		 * our options are:
		 *
		 * 1. search for the unique indexes on the table and pick one
		 * 2. Lookup the index name from sl_table
		 * 3. Have the _init() method load a list of all replicated tables
		 *    for this remote and the associated unique key names.
		 */
		
	}
	else if (change->action == REORDER_BUFFER_CHANGE_DELETE)
	{
		/**
		 * convert the key columns to a pair of arrays.
		 */
	  action='D';
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
	
	appendStringInfo(out,"%d,%d,%d,NULL,%s,%s,%c,%d,%s"
					 ,local_id
					 ,txn->xid
					 ,table_id
					 ,namespace
					 ,table_name
					 ,action
					 ,update_cols
					 ,array_text);
	RelationClose(relation);
	ReleaseSysCache(array_type_tuple);
	
	
	
	MemoryContextSwitchTo(old);
	MemoryContextReset(context);

	elog(NOTICE,"leaving og pg_decode_change:");
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

/**
 * finds information about the unique index of the given name
 * 
 * idx_name - the name of the index 
 * indcols - an array of integers that the attributes that make up the index will 
 *           be stored in
 * indsize - the number of attributes that make up the index will be stored here
 * 
 */
bool get_unique_idx(const char * idx_name, int ** indcols, int * indsize)
{
  return true;

}
