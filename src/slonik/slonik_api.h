#ifndef SLONIK_API_H
#define SLONIK_API_H


struct SlonikContext;
	
typedef struct {
	int no_id;
	const char * conninfo;
}SlonikApi_NodeConnInfo ;

struct SlonikContext * slonik_api_init_context(const char * clustername,
											   SlonikApi_NodeConnInfo ** adm_conninfo);
int slonik_api_sync(struct SlonikContext * context, int no_id);

#endif
