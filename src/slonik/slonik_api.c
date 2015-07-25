#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <alloca.h>
#else
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#define sleep(x) Sleep(x*1000)
#endif


#include "types.h"
#include "libpq-fe.h"

#include "slonik_api.h"
#include "slonik.h"

struct SlonikContext {
	SlonikScript * script;
	
};

static int header_setup(SlonikStmt *hdr,
						struct SlonikContext * context);

static int commit_check(SlonikStmt *hdr,struct SlonikContext * context);
static int rollback_check(SlonikStmt *hdr,struct SlonikContext * context);

struct SlonikContext * slonik_api_init_context(const char * clustername,
											   SlonikApi_NodeConnInfo ** adm_conninfo)
{
	SlonikApi_NodeConnInfo ** nodeInfo;

	SlonikScript * script = malloc(sizeof(SlonikScript));
	memset(script,0,sizeof(SlonikScript));
	script->clustername=strdup(clustername);
	script->last_event_node=-1;
	SlonikAdmInfo * prevAdmInfo=NULL;
	for(nodeInfo = adm_conninfo; *nodeInfo != NULL; nodeInfo++)
	{
		SlonikAdmInfo * admInfo = malloc(sizeof(SlonikAdmInfo));
		memset(admInfo,0,sizeof(SlonikAdmInfo));
		admInfo->no_id = (*nodeInfo)->no_id;
		admInfo->conninfo = strdup((*nodeInfo)->conninfo);
		admInfo->script=script;
		if(prevAdmInfo != NULL) 
		{
			prevAdmInfo->next=admInfo;
		}
		else {
			script->adminfo_list = admInfo;
		}
		prevAdmInfo=admInfo;
			
	}
	struct SlonikContext * newContext = malloc(sizeof(struct SlonikContext));
	memset(newContext,0,sizeof(struct SlonikContext));
	newContext->script=script;
	return newContext;
}



int slonik_api_sync(struct SlonikContext * context, int no_id) 
{
	
	int rc;
	SlonikStmt_sync sync;

	header_setup(&sync.hdr,context);
	sync.hdr.stmt_type=STMT_SYNC;
	sync.no_id = no_id;
	rc = slonik_sync(&sync);
	if(rc == 0)
	{
		commit_check(&sync.hdr,context);
	}
	else 
	{
		rollback_check(&sync.hdr,context);
	}
	return rc;


}


int slonik_api_subscribe_set(struct SlonikContext * context,
							 int set_id,
							 int provider,
							 int receiver,
							 int forward,
							 int omit_copy)
{
	SlonikStmt_subscribe_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SUBSCRIBE_SET;
	stmt.sub_setid = set_id;
	stmt.sub_provider=provider;
	stmt.sub_receiver=receiver;
	stmt.sub_forward=forward;
	stmt.omit_copy=omit_copy;
	rc = slonik_subscribe_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

int slonik_api_restart_node(struct SlonikContext * context, int no_id)
{
	SlonikStmt_restart_node stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_RESTART_NODE;
	stmt.no_id=no_id;
	rc = slonik_restart_node(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

int slonik_api_repair_config(struct SlonikContext * context, int no_id,int only_on_node)
{
	SlonikStmt_repair_config stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_REPAIR_CONFIG;
	stmt.ev_origin=no_id;
	stmt.only_on_node=only_on_node;
	rc = slonik_repair_config(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_init_cluster(struct SlonikContext * context, int no_id)
{
	SlonikStmt_init_cluster stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_INIT_CLUSTER;
	stmt.no_id=no_id;
	rc = slonik_init_cluster(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_store_node(struct SlonikContext * context, int no_id,int ev_origin)
{
	SlonikStmt_store_node stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_STORE_NODE;
	stmt.no_id=no_id;
	stmt.ev_origin=ev_origin;
	rc = slonik_store_node(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_drop_node(struct SlonikContext * context, int * no_id_list,int ev_origin)
{
	SlonikStmt_drop_node stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_DROP_NODE;
	stmt.no_id_list=no_id_list;
	stmt.ev_origin=ev_origin;
	rc = slonik_drop_node(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_uninstall_node(struct SlonikContext * context, int  no_id)
{
	SlonikStmt_uninstall_node stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_UNINSTALL_NODE;
	stmt.no_id=no_id;
	rc = slonik_uninstall_node(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

int slonik_api_clone_prepare(struct SlonikContext * context, int  no_id,int provider)
{
	SlonikStmt_clone_prepare stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_CLONE_PREPARE;
	stmt.no_id=no_id;
	stmt.no_provider = provider;
	rc = slonik_clone_prepare(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_clone_finish(struct SlonikContext * context, int  no_id,int provider)
{
	SlonikStmt_clone_finish stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_CLONE_FINISH;
	stmt.no_id=no_id;
	stmt.no_provider = provider;
	rc = slonik_clone_finish(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}



int slonik_api_store_path(struct SlonikContext * context, int  client,int server,char * conninfo,
	int connretry)
{
	SlonikStmt_store_path stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_STORE_PATH;
	stmt.pa_server = server;
	stmt.pa_client = client;
	stmt.pa_conninfo = conninfo;
	stmt.pa_connretry = connretry;
	rc = slonik_store_path(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

int slonik_api_drop_path(struct SlonikContext * context, int  client,int server,int ev_origin)
{
	SlonikStmt_drop_path stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_DROP_PATH;
	stmt.pa_server = server;
	stmt.pa_client = client;
	stmt.ev_origin=ev_origin;
	rc = slonik_drop_path(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_create_set(struct SlonikContext * context, int set_id,int  set_origin,char * comment)
{
	SlonikStmt_create_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_CREATE_SET;
	stmt.set_id = set_id;
	stmt.set_origin = set_origin;
	stmt.set_comment = comment;

	rc = slonik_create_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_drop_set(struct SlonikContext * context, int  set_id,int origin)
{
	SlonikStmt_drop_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_DROP_SET;
	stmt.set_id = set_id;
	stmt.set_origin = origin;
	rc = slonik_drop_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

int slonik_api_merge_set(struct SlonikContext * context, int  set_id,int add_id,int origin)
{
	SlonikStmt_merge_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_MERGE_SET;
	stmt.set_id = set_id;
	stmt.set_origin = origin;
	stmt.add_id=add_id;
	rc = slonik_merge_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}

	

int slonik_api_set_add_table(struct SlonikContext * context, int  set_id,char * fully_qualified_name
							 ,int origin)
{
	SlonikStmt_set_add_table stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_ADD_TABLE;
	stmt.set_id = set_id;
	stmt.set_origin = origin;
	stmt.tab_fqname = fully_qualified_name;

	rc = slonik_set_add_table(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}



int slonik_api_set_add_sequence(struct SlonikContext * context,char * fully_qualified_name
							 ,int origin) 
{
	SlonikStmt_set_add_sequence stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_ADD_SEQUENCE;
	stmt.set_origin = origin;
	stmt.seq_fqname = fully_qualified_name;

	rc = slonik_set_add_sequence(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_set_drop_table(struct SlonikContext * context,int table_id
							 ,int origin) 
{
	SlonikStmt_set_drop_table stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_DROP_TABLE;
	stmt.set_origin = origin;
	stmt.tab_id = table_id;

	rc = slonik_set_drop_table(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_set_drop_sequence(struct SlonikContext * context,int seq_id
							 ,int origin) 
{
	SlonikStmt_set_drop_sequence stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_DROP_SEQUENCE;
	stmt.set_origin = origin;
	stmt.seq_id = seq_id;

	rc = slonik_set_drop_sequence(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}



int slonik_api_set_move_table(struct SlonikContext * context,int table_id
							  ,int origin, int new_set_id) 
{
	SlonikStmt_set_move_table stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_MOVE_TABLE;
	stmt.set_origin = origin;
	stmt.tab_id = table_id;
	stmt.new_set_id = new_set_id;

	rc = slonik_set_move_table(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_set_move_sequence(struct SlonikContext * context,int seq_id
							  ,int origin, int new_set_id) 
{
	SlonikStmt_set_move_sequence stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_MOVE_SEQUENCE;
	stmt.set_origin = origin;
	stmt.seq_id = seq_id;
	stmt.new_set_id = new_set_id;

	rc = slonik_set_move_sequence(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}




int slonik_api_unsubscribe_set(struct SlonikContext * context, int  set_id,int receiver)
{
	SlonikStmt_unsubscribe_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SET_MOVE_TABLE;
	stmt.sub_setid = set_id;
	stmt.sub_receiver = receiver;

	rc = slonik_unsubscribe_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_lock_set(struct SlonikContext * context, int  set_id)
{
	SlonikStmt_lock_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_LOCK_SET;
	stmt.set_id = set_id;

	rc = slonik_lock_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_unlock_set(struct SlonikContext * context, int  set_id)
{
	SlonikStmt_unlock_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_UNLOCK_SET;
	stmt.set_id = set_id;

	rc = slonik_unlock_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_move_set(struct SlonikContext * context, int  set_id,int old_origin,int new_origin)
{
	SlonikStmt_move_set stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_MOVE_SET;
	stmt.set_id = set_id;
	stmt.old_origin = old_origin;
	stmt.new_origin = new_origin;

	rc = slonik_move_set(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_ddl_script(struct SlonikContext * context, int  origin_id,char * ddl_sql,
						  int ** only_on_nodes)
{
	SlonikStmt_ddl_script stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_DDL_SCRIPT;
	stmt.ev_origin = origin_id;
	stmt.ddl_sql = ddl_sql;
	int ** ptr = only_on_nodes;
	while(*ptr != NULL)
	{
		char buffer[32];
		sprintf(buffer,"%d",**ptr);
		if(stmt.only_on_nodes == NULL)
		{
			stmt.only_on_nodes = (char*) malloc(strlen(buffer)+1);
			stmt.only_on_nodes[0]='\0';
		}
		else 
		{
			stmt.only_on_nodes = (char*)realloc(stmt.only_on_nodes,strlen(stmt.only_on_nodes)+strlen(buffer)+1);
			strncat(stmt.only_on_nodes,buffer,strlen(buffer));
		}
	}


	rc = slonik_ddl_script(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	if(stmt.only_on_nodes != NULL)
		free(stmt.only_on_nodes);
	return rc;
}


int slonik_api_update_functions(struct SlonikContext * context, int  node_id)
{
	SlonikStmt_update_functions stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_UPDATE_FUNCTIONS;
	stmt.no_id = node_id;

	rc = slonik_update_functions(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_wait_event(struct SlonikContext * context, int  wait_on, int wait_origin,int wait_confirmed,
	int timeout)
{
	SlonikStmt_wait_event stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_WAIT_EVENT;
	stmt.wait_origin = wait_origin;
	stmt.wait_on = wait_on;
	stmt.wait_timeout = timeout;
	stmt.wait_confirmed = wait_confirmed;

	rc = slonik_wait_event(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}


int slonik_api_switch_log(struct SlonikContext * context, int  no_id)
{
	SlonikStmt_switch_log stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SWITCH_LOG;
	stmt.no_id = no_id;

	rc = slonik_switch_log(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	return rc;
}



int slonik_api_failed_node(struct SlonikContext * context,SlonikApi_FailedNode ** failed_nodes)
{
	SlonikStmt_failed_node stmt;
	int rc;

	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_FAILED_NODE;
	failed_node_entry * last_entry = NULL;
	SlonikApi_FailedNode **ptr;
	for(ptr = failed_nodes; *ptr != NULL; ptr++)
	{
		failed_node_entry * next_entry = (failed_node_entry*) malloc (sizeof(failed_node_entry));
		memset(next_entry,0,sizeof(failed_node_entry));
		next_entry->no_id = (*ptr)->node_id;
		next_entry->backup_node = (*ptr)->backup_node;
		if(last_entry != NULL) 
		{
			last_entry->next = next_entry;
		}
		else {
			stmt.nodes = next_entry;
		}
		last_entry = next_entry;
	}

	
	

	rc = slonik_failed_node(&stmt);
	if(rc == 0)
	{
		commit_check(&stmt.hdr,context);
	}
	else 
	{
		rollback_check(&stmt.hdr,context);
	}
	failed_node_entry *failed_entry;
	for(failed_entry = stmt.nodes; failed_entry != NULL; )
	{
		failed_node_entry * del = failed_entry;
		failed_entry = failed_entry->next;
		free(del);
	}

	return rc;
}


static int header_setup(SlonikStmt *hdr,
						struct SlonikContext * context) 
{
	
	hdr->stmt_filename="";
	hdr->stmt_lno=0;
	hdr->script=context->script;
	hdr->next=NULL;
	return 0;
}


static int commit_check(SlonikStmt *hdr,struct SlonikContext * context)
{
	if(context->script->current_try_level == 0)
	{
		script_commit_all(hdr,context->script);
	}
	 return 0;
 }

static int rollback_check(SlonikStmt *hdr,struct SlonikContext * context)
{
	if(context->script->current_try_level == 0)
	{
		script_rollback_all(hdr,context->script);
	}
	return 0;
}

