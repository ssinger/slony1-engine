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


struct SlonikContext * slonik_api_init_context(const char * clustername,
											   SlonikApi_NodeConnInfo ** adm_conninfo)
{
	SlonikApi_NodeConnInfo ** nodeInfo;

	SlonikScript * script = malloc(sizeof(SlonikScript));
	memset(script,0,sizeof(SlonikScript));
	script->clustername=strdup(clustername);
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
			prevAdmInfo=admInfo;
		}
		else {
			script->adminfo_list = admInfo;
		}
			
	}
	struct SlonikContext * newContext = malloc(sizeof(struct SlonikContext));
	memset(newContext,0,sizeof(struct SlonikContext));
	newContext->script=script;
	return newContext;
}



int slonik_api_sync(struct SlonikContext * context, int no_id) 
{
	

	SlonikStmt_sync sync;

	header_setup(&sync.hdr,context);
	sync.hdr.stmt_type=STMT_SYNC;
	sync.no_id = no_id;
	return slonik_sync(&sync);
	
		


}


int slonik_api_subscribe_set(struct SlonikContext * context,
							 int set_id,
							 int provider,
							 int receiver,
							 int forward,
							 int omit_copy)
{
	SlonikStmt_subscribe_set stmt;
	header_setup(&stmt.hdr,context);
	stmt.hdr.stmt_type=STMT_SUBSCRIBE_SET;
	stmt.sub_setid = set_id;
	stmt.sub_provider=provider;
	stmt.sub_receiver=receiver;
	stmt.sub_forward=forward;
	stmt.omit_copy=omit_copy;
	return slonik_subscribe_set(&stmt);
}



static int header_setup(SlonikStmt *hdr,
						struct SlonikContext * context) 
{
	
	hdr->stmt_filename="";
	hdr->stmt_lno=0;
	hdr->script=context->script;
	hdr->next=NULL;
}
