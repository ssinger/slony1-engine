#!/bin/sh
# $Id: lock_sets.sh,v 1.1.2.1 2009-02-23 17:19:15 cbbrowne Exp $

source upgrade_12_20_common.sh

echo "Dumping configuration info about upgrade"
date

echo "Cluster: ${CLUSTER}"
echo "PostgreSQL binaries in: ${PGBINDIR}"
echo "Slony-I tools in: ${SLTOOLDIR}"

echo "Number of nodes: ${NUMNODES}"

echo "Locking nodes..."

node=1
while : ; do
    eval db=\$DB${node}
    eval host=\$HOST${node}
    eval user=\$USER${node}
    eval port=\$PORT${node}

    echo "Node ${node} - DB connection - PGDATABASE=${db} PGHOST=${host} PGUSER=${user} PGPORT=${port}"
    echo "--------------------------------------------------------------------------------------------"

    SETS=`${PSQL} -tq -R " " -d ${db} -h ${host} -U ${user} -p ${port} -c "select set_id from ${CLSCHEMA}.sl_set where set_origin = ${CLSCHEMA}.getlocalnodeid('_${CLUSTER}') and set_locked is null;"`

    echo "Unlocked replication sets originating on this node: ${SETS}"

    echo "Locking them..."
    ${PSQL} -d ${db} -h ${host} -U ${user} -p ${port} -c "select ${CLSCHEMA}.lockSet(set_id) from ${CLSCHEMA}.sl_set where set_origin = ${CLSCHEMA}.getlocalnodeid('_${CLUSTER}') and set_locked is null;"

    if [ ${node} -ge ${NUMNODES} ]; then
	break;
    else
	node=$((${node} + 1))
    fi
done

