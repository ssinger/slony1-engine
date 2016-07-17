#!/bin/sh
echo "######################################################################"
echo "# 13_drop_all.sh"
echo "#"
echo "#	This script stops everything and then drops all 4 databases."
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# Stop pgbench and all slon processes, in case they are running.
# ----
./stop_all.sh

# ----
# Drop and recreate the databases.
# ----
dropdb ${DBNAME_BASE}_1 || echo "(ignored)"
dropdb ${DBNAME_BASE}_2 || echo "(ignored)"
dropdb ${DBNAME_BASE}_3 || echo "(ignored)"
dropdb ${DBNAME_BASE}_4 || echo "(ignored)"
