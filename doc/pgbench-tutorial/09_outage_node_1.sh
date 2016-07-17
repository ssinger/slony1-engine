#!/bin/sh
echo "######################################################################"
echo "# 09_outage_node_1.sh"
echo "#"
echo "#	This script prepares node 1 for offline maintenance. Since"
echo "#	node 1 is our master in normal mode, we need to move the"
echo "#	application from node 1 to node 2."
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
# First thing is to make node 2 the data provider for node 4.
# ----
echo 'SLONIK: adding paths between nodes 2 and 4';
store path (server = 2, client = 4,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_2');
store path (server = 4, client = 2,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_4');
sync (id = 2);
sync (id = 4);
wait for event (origin = 2, confirmed = all, wait on = 1, timeout = 0);
wait for event (origin = 4, confirmed = all, wait on = 1, timeout = 0);

echo 'SLONIK: change provider for node 4 to be 2';
subscribe set (id = 1000, receiver = 4, provider = 2, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_

# ----
# Now we need to stop the application (for the first time during
# this tutorial).
# ----
./stop_pgbench.sh

# ----
# Make node 2 the origin of the set.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

echo 'SLONIK: locking set';
lock set (id = 1000, origin = 1);
echo 'SLONIK: move set to new origin node 2';
move set (id = 1000, old origin = 1, new origin = 2);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_

# ----
# Resume our application, but this time against node 2.
# ----
echo "Resuming application against node 2"
./start_pgbench.sh 2

# ----
# We now can stop the slon process for node 1 and it is isolated.
# It will fall behind from now on.
# ----
./stop_slon.sh 1

