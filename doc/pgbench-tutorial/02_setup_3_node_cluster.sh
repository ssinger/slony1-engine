#!/bin/sh
echo "######################################################################"
echo "# 02_setup_3_node_cluster.sh"
echo "#"
echo "#	This script creates the initial 3 node cascaded cluster."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# Make sure the log and .status directories exist.
# ----
mkdir -p log
mkdir -p .status

# ----
# Stop pgbench and all slon processes, in case they are running.
# ----
./stop_slon.sh 1
./stop_slon.sh 2
./stop_slon.sh 3
./stop_slon.sh 4

# ----
# Copy the schema of the database for node 1 into nodes 2 and 3.
# ----
pg_dump -s ${DBNAME_BASE}_1 | psql -q ${DBNAME_BASE}_2
pg_dump -s ${DBNAME_BASE}_1 | psql -q ${DBNAME_BASE}_3

# ----
# Create the Slony-I cluster using the slonik utility.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for node 1, 2 and 3.
# ----
$SLONIK_PREAMBLE_123

# ----
# Create the 3 nodes.
# ----
echo 'SLONIK: create nodes';
init cluster (id = 1);
store node (id = 2, event node = 1);
store node (id = 3, event node = 1);

# ----
# Define communication paths.
# ----
echo 'SLONIK: store paths';
store path (server = 1, client = 2,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_1');
store path (server = 2, client = 1,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_2');

store path (server = 2, client = 3,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_2');
store path (server = 3, client = 2,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_3');

_EOF_

# ----
# Start the slon processes for nodes 1, 2 and 3.
# Setup the initial Slony-1 cluster.
# ----
./start_slon.sh 1
./start_slon.sh 2
./start_slon.sh 3

# ----
# Now we create the set and subscriptions.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for node 1, 2 and 3.
# ----
$SLONIK_PREAMBLE_123

# ----
# Create the set with all 4 tables and the hid sequence.
# ----
echo 'SLONIK: create set';
create set (id = 1000, origin = 1);
set add table (set id = 1000, id = 1001,
	fully qualified name = 'public.pgbench_accounts');
set add table (set id = 1000, id = 1002,
	fully qualified name = 'public.pgbench_branches');
set add table (set id = 1000, id = 1003,
	fully qualified name = 'public.pgbench_tellers');
set add table (set id = 1000, id = 1004,
	fully qualified name = 'public.pgbench_history');
set add sequence (set id = 1000, id = 1005,
	fully qualified name = 'public.pgbench_history_hid_seq');

# ----
# Subscribe the set to node 2.
# ----
echo 'SLONIK: subscribe node 2';
subscribe set (id = 1000, receiver = 2, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

# ----
# Subscribe the set to node 3 using node 2 as provider.
# ----
echo 'SLONIK: subscribe node 3';
subscribe set (id = 1000, receiver = 3, provider = 2, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_

