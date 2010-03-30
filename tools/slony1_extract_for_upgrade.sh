#!/bin/sh
# ----------
# slony1_extract_for_upgrade
#
#	Script to extract the user schema of a slony node in preperation
#       for an upgrade of postgresql.
#
# This script will:
#
#      1) Call slony1_extract_schema.sh to extract the schema in the format
#         suitable for the current pg version
#      2) Create a temp database and restore the schema to it
#      3) Use the pg_dump of the postgresql version you are upgrading to to
#         dump the schema to a format suitable for restoring on the new
#         postgresql.
#
#
# Note: This script requires slony1_extract_schema.sh to be in your $PATH
#
# ----------

# ----
# Check for correct usage
# ----
if test $# -ne 5 ; then
	echo "usage: $0 dbname clustername tempdbname existing_pgbindir new_pgbindir" >&2
	exit 1
fi

# Remember call arguments and get the nodeId of the DB specified
# ----
dbname=$1
cluster=$2
tmpdb=$3
existing_pgbindir=$4
new_pgbindir=$5
TMP=tmp_schema.$$



if [ $? -ne 0 ]
then
   echo "error extracting existing schema"
   exit 1
fi

if [ ! -f $existing_pgbindir/pg_dump ]
then
    echo "error $existing_pgbindir does not contain pg_dump"
    exit 1
fi

if [ ! -f $new_pgbindir/pg_dump ]
then
    echo "error $new_pgbindir does not contain pg_dump"
    exit 1
fi

###
# Step 1, extract the schema.
##
PATH=$existing_pgbindir:$PATH slony1_extract_schema.sh $dbname $cluster $tmpdb >$TMP.sql


createdb $tmpdb >/dev/null
if [ $? -ne 0 ]
then
    echo "error creating temp db"
    exit 1
fi

psql $tmpdb <$TMP.sql >/dev/null
if [ $? -ne 0 ]
then
    echo "error restoring schema"
    exit 1;
fi
$new_pgbindir/pg_dump -s $tmpdb

dropdb $tmpdb