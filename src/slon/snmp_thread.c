#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "slon.h"

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>


extern int	slon_log_level;

void
init_nstAgentSubagentObject(void)
{
	static oid	nstAgentSubagentObject_oid[] =
	{1, 3, 6, 1, 4, 1, 20366, 32, 2, 3, 32, 1};

	netsnmp_register_int_instance("nstAgentSubagentObject",
								  nstAgentSubagentObject_oid,
								  OID_LENGTH(nstAgentSubagentObject_oid),
								  &slon_log_level, NULL);
}


void *
snmpThread_main(void *dummy)
{
	int			agentx_subagent = 1;


	/*************************************************************************
	 * Check configuration to see if have been asked to run as a
	 * master agent (0) the default is to run as a subagent (1)
	 ************************************************************************/


	/************************************************************************
	 * we really should make sure that we point snmp_log to slony_log, this
	 * work is being saved for a later date once I better understand how
	 * snmp_log works
	 ************************************************************************/

	if (agentx_subagent)
	{
		/* make us a agentx client. */
		netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
							   NETSNMP_DS_AGENT_ROLE, 1);

		/*
		 * ********************************************************************
		 * If we are running slon as root allow the snmp agent to have full
		 * access to it's internals, this is required to run as a master agent
		 * (from my understanding) ********************************************************************
		 */

		if (getuid() != 0)
		{
			netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
								   NETSNMP_DS_AGENT_NO_ROOT_ACCESS, 1);
		}
	}

	init_agent("slon-demon");


	/*
	 * initialize the mib code found in: init_nstAgentSubagentObject from
	 * nstAgentSubagentObject.C
	 */
	init_nstAgentSubagentObject();

	/* initialize vacm/usm access control  */
/*
  if (!agentx_subagent)
  {
	  init_vacm_vars();
	  init_usmUser();
  }
*/
	init_snmp("slon-demon");

	/* If we're going to be a snmp master agent, initial the ports */
	if (!agentx_subagent)
	{
		init_master_agent();	/* open the port to listen on (defaults to
								 * udp:161) */
	}

	while (true)
	{
		/***********************************************************************
		 * Begin processing snmp requests we can pass in 0 if we rather do this
		 * in a non blocking method
		 ***********************************************************************/

		agent_check_and_process(1);		/* 0 == don't block */

	}

	snmp_shutdown("slon-demon");
	pthread_exit(NULL);
}
