#!/bin/sh
echo "######################################################################"
echo "# 12_rebuild_node_1.sh"
echo "#"
echo "#	This script rebuilds node 1 and makes it the master again."
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
		${DBNAME_BASE}_2`
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
# Create the database for node 1 again.
# ----
createdb ${DBNAME_BASE}_1

# ----
# Copy the schema of the database from node 2 to node 1.
# ----
pg_dump -s -N _pgbench ${DBNAME_BASE}_2 | psql -q ${DBNAME_BASE}_1

# ----
# Add the node via slonik.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all 4 nodes
# ----
$SLONIK_PREAMBLE_1234

# ----
# Create the node.
# ----
echo 'SLONIK: create node 1';
store node (id = 1, event node = 2);

# ----
# Define communication paths.
# ----
echo 'SLONIK: store paths';
store path (server = 1, client = 2,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_1');
store path (server = 2, client = 1,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_2');

_EOF_

# ----
# Start the slon processes for node 1.
# ----
./start_slon.sh 1

# ----
# Subscribe node 1.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

echo 'SLONIK: subscribe node 1';
subscribe set (id = 1000, receiver = 1, provider = 2, forward = yes);
sync (id = 2);
wait for event (origin = 2, confirmed = all, wait on = 2, timeout = 0);

_EOF_

# ----
# Now we want to make node 1 the master again.
# ----

slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

sync (id = 2);
wait for event (origin = 2, confirmed = 1, wait on = 1, timeout = 0);

_EOF_

# ----
# Let us "forget" to stop the application at this point. The
# LOCK SET in the following slonik script should make sure it
# is getting the message.
# ----
# ./stop_pgbench.sh

# ----
# Make node 1 the origin of the set.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

echo 'SLONIK: locking set';
lock set (id = 1000, origin = 2);
echo 'SLONIK: move set to new origin node 1';
move set (id = 1000, old origin = 2, new origin = 1);
sync (id = 2);
wait for event (origin = 2, confirmed = all, wait on = 2, timeout = 0);

_EOF_

# ----
# Let us see what our application has to say about that.
# ----
echo "This is what the application thought about this MOVE:"
echo "----"
tail -20 ./log/pgbench.log
echo "----"

# ----
# Resume our application against node 1 again
# ----
echo "Resuming application against node 1"
./start_pgbench.sh 1

# ----
# Make node 1 the provider for node 4 again and drop the
# obsolete paths between 2 and 4.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

# ----
# Define the currently missing paths between nodes 1 and 4.
# ----
echo 'SLONIK: store paths 1 <-> 4';
store path (server = 1, client = 4,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_1');
store path (server = 4, client = 1,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_4');

# ----
# Resubscribe node 4 against node 1.
# ----
echo 'SLONIK: change provider for node 4 to be 1';
subscribe set (id = 1000, receiver = 4, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

# ----
# Clean up
# ----
echo 'SLONIK: dropping paths between nodes 2 and 4';
drop path (server = 2, client = 4);
drop path (server = 4, client = 2);
sync (id = 2);
sync (id = 4);
wait for event (origin = 2, confirmed = 4, wait on = 2, timeout = 0);
wait for event (origin = 4, confirmed = 2, wait on = 4, timeout = 0);


_EOF_

