#!/bin/sh

. ./config.sh

if [ $# -ne 1 ] ; then
    echo "usage: $0 NODE_ID" >&2
	exit 2
fi

# ----
# Make sure the log and .status directories exist.
# ----
mkdir -p log
mkdir -p .status 

PIDFILE=./.status/pgbench.pid
if [ -f $PIDFILE ] ; then
	PID=`cat $PIDFILE`
	echo "pid file for pgbench exists ... stopping it first"
	kill $PID && echo "pgbench (pid=$PID) killed"
	rm $PIDFILE
	sleep 1
fi

pgbench -n -c2 -T7200 -R2 ${DBNAME_BASE}_$1 >./log/pgbench.log 2>&1 &
PID=$!
echo $PID >$PIDFILE
echo "pgbench started (pid=$PID)"

