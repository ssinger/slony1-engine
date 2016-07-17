#!/bin/sh
echo "######################################################################"
echo "# 08_resume_node_2.sh"
echo "#"
echo "#	This script resumes node 2 after the maintenance outage."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# Fist start the slon process.
# ----
./start_slon.sh 2

slonik <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_1234

# ----
# Since the node is behind, we first want to let it catch up.
# ----
echo 'SLONIK: waiting for node 2 to catch up';
sync (id = 1);
wait for event (origin = 1, confirmed = 2, wait on = 2, timeout = 0);

# ----
# We can now put node 3 into cascaded subscription again and drop
# the paths between nodes 1 and 3.
# ----
echo 'SLONIK: resubscribing node 3 cascaded from node 2';
subscribe set (id = 1000, receiver = 3, provider = 2, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = 3, wait on = 1, timeout = 0);
echo 'SLONIK: dropping paths between nodes 1 and 3';
drop path (server = 1, client = 3);
drop path (server = 3, client = 1);
sync (id = 1);
wait for event (origin = 1, confirmed = 3, wait on = 1, timeout = 0);

_EOF_


