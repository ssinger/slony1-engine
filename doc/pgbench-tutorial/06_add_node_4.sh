#!/bin/sh
echo "######################################################################"
echo "# 06_add_node_4.sh"
echo "#"
echo "#	This script adds node 4 as a direct subscriber."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# Discovered a Slony bug while creating this tutorial. 
# If there are entries in sl_log_script, a new node will try
# to replicated them no matter what. This is a workaround.
# ----
while true ; do
	N=`psql -qAt -c "select count(*) from _pgbench.sl_log_script" \
		${DBNAME_BASE}_1`
	if [ $N -eq 0 ] ; then
		break
	fi
	echo "Not yet cleaned up SCRIPT replication data detected,"
	echo "calling cleanupEvent() to clean this up."
	psql -c "select _pgbench.cleanupEvent('0s'::interval)" \
		${DBNAME_BASE}_1 >/dev/null 2>&1
	sleep 2
done

# ----
# Copy the schema of the database from node 1 to node 4
# ----
pg_dump -s -N _pgbench ${DBNAME_BASE}_1 | psql -q ${DBNAME_BASE}_4

# ----
# Add the node via slonik.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all 4 nodes
# ----
$SLONIK_PREAMBLE_1234

# ----
# Create node 4.
# ----
echo 'SLONIK: create node 4';
store node (id = 4, event node = 1);

# ----
# Define communication paths.
# ----
echo 'SLONIK: store paths';
store path (server = 1, client = 4,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_1');
store path (server = 4, client = 1,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_4');

_EOF_

# ----
# Start the slon processes for node 4.
# ----
./start_slon.sh 4

# ----
# Subscribe node 4.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

echo 'SLONIK: subscribe node 4';
subscribe set (id = 1000, receiver = 4, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_
