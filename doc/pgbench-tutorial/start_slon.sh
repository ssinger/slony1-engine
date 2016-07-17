#!/bin/sh

. ./config.sh

if [ $# -ne 1 ] ; then
	echo "usage: $0 NODE_ID" >&2
	exit 2
fi

PIDFILE=./.status/slon_${1}.pid
if [ -f $PIDFILE ] ; then
	PID=`cat $PIDFILE`
	echo "pid file for slon of node $1 exists ... stopping it first"
	kill $PID && echo "slon process for node $1 (pid=$PID) killed"
	rm $PIDFILE
	sleep 1
fi

slon -f slon.conf pgbench "dbname=${DBNAME_BASE}_$1" >./log/slon_$1.log 2>&1 &
PID=$!
echo $PID >$PIDFILE
echo "slon for node $1 started (pid=$PID)"

