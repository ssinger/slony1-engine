#!/bin/sh

# 

# Christopher Browne
# Afilias Canada
# 2004-11-15
# For the Slony-I project

# This script may be run every few minutes to force in syncs on a
# particular node.

# Supposing you have some possibly-flakey slon daemon that might not
# run all the time, you might return from a weekend away only to
# discover the following situation...

# On Friday night, something went "bump" and while the database came
# back up, none of the slon daemons survived.  Your online application
# then saw nearly three days worth of heavy transactions.

# When you restart slon on Monday, it hasn't done a SYNC on the master
# since Friday, so that the next "SYNC set" comprises all of the
# updates between Friday and Monday.  Yuck.

# If you run generate_syncs.sh as a cron job every 20 minutes, it will
# force in a periodic SYNC on the "master" server, which means that
# between Friday and Monday, the numerous updates are split into more
# than 100 syncs, which can be applied incrementally, making the
# cleanup a lot less unpleasant.

# Note that if SYNCs _are_ running regularly, this script won't bother
# doing anything.

export NAMESPACE="_${1}"
export PGHOST=$2
export PGPORT=$3
export DATABASE=$4
INTERVAL="10 minutes"   # Don't bother generating a SYNC if there has been
                        # one in the last $INTERVAL
PSQL=`which psql`    # Replace this with your favorite command to run psql

echo "select \"$NAMESPACE\".generate_sync_event('$INTERVAL');" | \
   ${PSQL} -h $PGHOST -p $PGPORT -d $DATABASE
