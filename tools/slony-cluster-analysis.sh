#!/bin/sh
# 
# Analyze Slony-I Configuration

# This script pulls some overall configuration from a Slony-I cluster
# and stows it in /opt/log/Slony-I/Configuration, doing a compare from
# run to run to see if there have been changes.  If the set of nodes,
# paths, sets, or tables changes, then an email will be sent to the
# DBA user.

# Requires some combination of PGDATA/PGHOST/PGPORT/... set up to
# connect to a database

CLUSTER=$1
LOGDIR=/opt/logs/Slony-I/Configuration
mkdir -p $LOGDIR
TFILE=/tmp/temp_slony_state.${CLUSTER}.$$
LASTSTATE=${LOGDIR}/${CLUSTER}.last
RECIPIENT="cbbrowne@example.info"

Q1="select no_id as \"node_id\", no_comment as \"node_description\" from \"_${CLUSTER}\".sl_node order by no_id; "
Q2="select pa_server, pa_client, pa_conninfo from \"_${CLUSTER}\".sl_path order by pa_server, pa_client; "
Q3="select set_id, set_origin, set_comment from \"_${CLUSTER}\".sl_set order by set_id; "
Q4="select tab_id, tab_set, tab_nspname, tab_relname, tab_comment from \"_${CLUSTER}\".sl_table order by tab_id;"

psql -c "$Q1" > $TFILE
psql -c "$Q2" >> $TFILE
psql -c "$Q3" >> $TFILE
psql -c "$Q4" >> $TFILE

if ( /usr/bin/cmp -s $TFILE $LASTSTATE ) ; then
   # Do nothing
   DO=0   # Non-Bash shells need a statement here...
else
    diff -c $LASTSTATE $TFILE | mail -s "Configuration changed for Slony-I cluster ${CLUSTER}" $RECIPIENT
    TODAY=`date +"%Y-%m-%d"`
    cp $LASTSTATE ${LOGDIR}/${CLUSTER}.changed_from_at.${TODAY}
fi

mv $TFILE $LASTSTATE
