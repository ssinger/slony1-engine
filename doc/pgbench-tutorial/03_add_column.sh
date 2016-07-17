#!/bin/sh
echo "######################################################################"
echo "# 03_add_column.sh"
echo "#"
echo "#	This script adds a last_update column to pgbench_accounts."
echo "#	The column is initialized to the max(mtime) found in the"
echo "#	pgbench_history table, or epoch. A trigger is created for"
echo "#	setting the column to current_timestamp on insert or update."
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
# Alter the table to add the last_update column. We also set the column
# value via EXECUTE so we do not push millions of updates through the
# row-level replication.
# ----
execute script (event node = 1, filename = 'sql/03_add_column_1.sql');
echo 'SLONIK: table altered';
sync (id = 1);
wait for event (origin = 1, confirmed = all, wait on = 1, timeout = 0);
echo 'SLONIK: changes replicated';

_EOF_

