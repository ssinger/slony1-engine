#!/bin/sh

. ./config.sh

for id in 1 2 3 4 ; do
	PIDFILE=./.status/slon_${id}.pid
	if [ -f $PIDFILE ] ; then
		PID=`cat $PIDFILE`
		kill $PID && echo "slon process for node $id (pid=$PID) killed"
		rm $PIDFILE
		sleep 1
		slon -f slon.conf pgbench "dbname=${DBNAME_BASE}_$id" >./log/slon_$id.log 2>&1 &
		PID=$!
		echo $PID >$PIDFILE
		echo "slon for node $id started (pid=$PID)"
	fi

done
