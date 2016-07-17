#!/bin/sh
echo "######################################################################"
echo "# 04_drop_table.sh"
echo "#"
echo "#	This script removes table pgbench_branches from replication."
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
set drop table (id = 1002, origin = 1);
echo 'SLONIK: table pgbench_branches removed from replication';
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);
echo 'SLONIK: changes replicated';

_EOF_

