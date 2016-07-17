#!/bin/sh
echo "######################################################################"
echo "# 10_resume_node_1.sh"
echo "#"
echo "#	This script resumes node 1 and restores the cluster to"
echo "#	the state from before script 09."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# Start the slon process for node 1 and let it catch up.
# ----
./start_slon.sh 1

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
# obsolete paths.
# ----
slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

echo 'SLONIK: change provider for node 4 to be 1';
subscribe set (id = 1000, receiver = 4, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

echo 'SLONIK: dropping paths between nodes 2 and 4';
drop path (server = 2, client = 4);
drop path (server = 4, client = 2);
sync (id = 2);
sync (id = 4);
wait for event (origin = 2, confirmed = 4, wait on = 2, timeout = 0);
wait for event (origin = 4, confirmed = 2, wait on = 4, timeout = 0);

_EOF_

