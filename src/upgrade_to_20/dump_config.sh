#!/bin/sh
# $Id: dump_config.sh,v 1.1.2.1 2009-02-23 17:19:15 cbbrowne Exp $

source upgrade_12_20_common.sh

echo "Dumping configuration info about upgrade"
date

echo "Cluster: ${CLUSTER}"
echo "PostgreSQL binaries in: ${PGBINDIR}"
echo "Slony-I tools in: ${SLTOOLDIR}"
echo "Acceptable replication lag: ${ACCEPTABLELAG}"

echo "Number of nodes: ${NUMNODES}"

node=1
while : ; do
    eval db=\$DB${node}
    eval host=\$HOST${node}
    eval user=\$USER${node}
    eval port=\$PORT${node}
    echo "Node ${node} - DB connection - PGDATABASE=${db} PGHOST=${host} PGUSER=${user} PGPORT=${port}"
    if [ ${node} -ge ${NUMNODES} ]; then
	break;
    else
	node=$((${node} + 1))
    fi
done

