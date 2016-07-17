#!/bin/sh
echo "######################################################################"
echo "# 07_outage_node_2.sh"
echo "#"
echo "#	This script prepares node 2 for offline maintenance."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

# ----
# In order to make node 3 a direct subscriber of 1, we need to
# add the communication paths.
# ----
echo 'SLONIK: adding paths between nodes 1 and 3';
store path (server = 1, client = 3,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_1');
store path (server = 3, client = 1,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_3');

echo 'SLONIK: change provider for node 3 to be 1';
subscribe set (id = 1000, receiver = 3, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_

# ----
# We now can stop the slon process for node 2 and it is isolated.
# It will fall behind from now on.
# ----
./stop_slon.sh 2

