#ifndef SLONIK_API_H
#define SLONIK_API_H


struct SlonikContext;
	
typedef struct {
	int no_id;
	const char * conninfo;
}SlonikApi_NodeConnInfo ;

typedef struct {
	int node_id;
	int backup_node;
}SlonikApi_FailedNode;

struct SlonikContext * slonik_api_init_context(const char * clustername,
											   SlonikApi_NodeConnInfo ** adm_conninfo);
int slonik_api_sync(struct SlonikContext * context, int no_id);
int slonik_api_subscribe_set(struct SlonikContext * context,
							 int set_id,
							 int provider,
							 int receiver,
							 int forward,
							 int omit_copy);
int slonik_api_restart_node(struct SlonikContext * context, int no_id);

int slonik_api_repair_config(struct SlonikContext * context, int no_id,int only_on_node);

int slonik_api_init_cluster(struct SlonikContext * context, int no_id);

int slonik_api_store_node(struct SlonikContext * context, int no_id,int ev_origin);

int slonik_api_drop_node(struct SlonikContext * context, int * no_id_list,int ev_origin);


int slonik_api_uninstall_node(struct SlonikContext * context, int  no_id);


int slonik_api_clone_prepare(struct SlonikContext * context, int  no_id,int provider);

int slonik_api_clone_finish(struct SlonikContext * context, int  no_id,int provider);

int slonik_api_store_path(struct SlonikContext * context, int  client,int server, char * conninfo,
						  int connretry);


int slonik_api_drop_path(struct SlonikContext * context, int  client,int server,int ev_origin);


int slonik_api_create_set(struct SlonikContext * context, int set_id,int  set_origin,char * comment);



int slonik_api_drop_set(struct SlonikContext * context, int  set_id,int origin);


int slonik_api_merge_set(struct SlonikContext * context, int  set_id,int add_id,int origin);



int slonik_api_set_add_table(struct SlonikContext * context, int  set_id,char * fully_qualified_name
							 ,int origin);


int slonik_api_set_add_sequence(struct SlonikContext * context,char * fully_qualified_name
								,int origin);



int slonik_api_set_drop_table(struct SlonikContext * context,int table_id
							  ,int origin) ;



int slonik_api_set_drop_sequence(struct SlonikContext * context,int seq_id
								 ,int origin);


int slonik_api_set_move_table(struct SlonikContext * context,int table_id
							  ,int origin, int new_set_id) ;


int slonik_api_set_move_sequence(struct SlonikContext * context,int seq_id
								 ,int origin, int new_set_id) ;


int slonik_api_unsubscribe_set(struct SlonikContext * context, int  set_id,int receiver);


int slonik_api_lock_set(struct SlonikContext * context, int  set_id);


int slonik_api_unlock_set(struct SlonikContext * context, int  set_id);



int slonik_api_move_set(struct SlonikContext * context, int  set_id,int old_origin,int new_origin);



int slonik_api_ddl_script(struct SlonikContext * context, int  origin_id,char * ddl_sql,
						  int **only_on_nodes);



int slonik_api_update_functions(struct SlonikContext * context, int  node_id);


int slonik_api_wait_event(struct SlonikContext * context, int  wait_on, int wait_origin,int wait_confirmed,
						  int timeout);



int slonik_api_switch_log(struct SlonikContext * context, int  no_id);


int slonik_api_failed_node(struct SlonikContext * context,SlonikApi_FailedNode ** failed_nodes);


#endif
