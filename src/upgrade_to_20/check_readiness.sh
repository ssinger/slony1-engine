#!/bin/sh
# $Id: check_readiness.sh,v 1.1.2.1 2009-02-23 17:19:15 cbbrowne Exp $

source upgrade_12_20_common.sh

echo "Check cluster readiness for upgrade"
date

echo "Cluster: ${CLUSTER}"
echo "PostgreSQL binaries in: ${PGBINDIR}"
echo "Slony-I tools in: ${SLTOOLDIR}"

echo "Number of nodes: ${NUMNODES}"

echo "Checking node readiness"

node=1
while : ; do
    eval db=\$DB${node}
    eval host=\$HOST${node}
    eval user=\$USER${node}
    eval port=\$PORT${node}
    psqlargs=" -d ${host} -p ${port} -d ${db} -U ${user} "

    echo "Checking Node ${node} - DB connection - PGDATABASE=${db} PGHOST=${host} PGUSER=${user} PGPORT=${port}"
    echo "--------------------------------------------------------------------------------------------"

    SETS=`${PSQL} -tq -R " " ${psqlargs} -c "select set_id from ${CLSCHEMA}.sl_set where set_origin = ${CLSCHEMA}.getlocalnodeid('_${CLUSTER}') and set_locked is null;"`

    if [ "x${SETS}" != "x" ]; then
	err 3 "node ${node} still has unlocked sets - ${SETS}"
    else
	echo "set locking - done!"
    fi

    WORSTLAG=`${PSQL} ${psqlargs} -tA -F "," -A -c "select st_received, st_lag_num_events, st_lag_time, st_lag_time > ${ACCEPTABLELAG} as bad_lag from _slony_regress1.sl_status where st_lag_time > ${ACCEPTABLELAG} order by st_lag_time desc limit 1;"`
    if [ "x${WORSTLAG}" != "x" ]; then
	bnode=`echo ${WORSTLAG} | cut -d "," -f 1`
	bevents=`echo ${WORSTLAG} | cut -d "," -f 2`
	blag=`echo ${WORSTLAG} | cut -d "," -f 3`
	err 3 "replication node ${bnode} is lagging by (events,lag) = (${bevents},${blag}) - lag > ${ACCEPTABLELAG}"
    else
	echo "No lags > ${ACCEPTABLELAG}"
    fi

    if [ ${node} -ge ${NUMNODES} ]; then
	break;
    else
	node=$((${node} + 1))
    fi
done

echo "All nodes checked - no problems found!"