#!/bin/sh
echo "######################################################################"
echo "# 01_setup_standalone_db.sh"
echo "#"
echo "#	This script creates the stand alone pgbench database. It also"
echo "#	adds a primary key to the pgbench_history table in preparation"
echo "#	for using Slony."
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
./stop_all.sh

# ----
# Drop and recreate the databases.
# ----
dropdb ${DBNAME_BASE}_1 || echo "(ignored)"
dropdb ${DBNAME_BASE}_2 || echo "(ignored)"
dropdb ${DBNAME_BASE}_3 || echo "(ignored)"
dropdb ${DBNAME_BASE}_4 || echo "(ignored)"

createdb ${DBNAME_BASE}_1 || exit 1
createdb ${DBNAME_BASE}_2 || exit 1
createdb ${DBNAME_BASE}_3 || exit 1
createdb ${DBNAME_BASE}_4 || exit 1

# ----
# Initialize pgbench in database 1. Modify the schema so that 
# the history table has a primary key.
# ----
pgbench -i -s 1 ${DBNAME_BASE}_1

psql -q ${DBNAME_BASE}_1 <<_EOF_
	alter table pgbench_history add column hid bigserial;
	alter table pgbench_history add primary key (hid);
_EOF_

# ----
# Start the pgbench in the background.
# ----
./start_pgbench.sh 1

