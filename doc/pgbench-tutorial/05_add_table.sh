#!/bin/sh
echo "######################################################################"
echo "# 05_add_table.sh"
echo "#"
echo "#	This script adds table pgbench_branches back into replication."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

slonik <<_EOF_
# ----
# Use the slony preamble for node 1, 2 and 3.
# ----
$SLONIK_PREAMBLE_123

# ----
# This is simple.
# ----
create set (id = 9999, origin = 1);
set add table (set id = 9999, id = 1002,
	fully qualified name = 'public.pgbench_branches');
echo 'SLONIK: temporary set 9999 with table pgbench_branches created';

# ----
# Subscribe the set to node 2.
# ----
echo 'SLONIK: subscribe node 2';
subscribe set (id = 9999, receiver = 2, provider = 1, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

# ----
# Subscribe the set to node 3 using node 2 as provider.
# ----
echo 'SLONIK: subscribe node 3';
subscribe set (id = 9999, receiver = 3, provider = 2, forward = yes);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

# ----
# Merge the sets.
# ----
echo 'SLONIK: merging 9999 into 1000';
merge set (id = 1000, add id = 9999, origin = 1);
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);

_EOF_

