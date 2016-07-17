#!/bin/sh

PIDFILE=./.status/pgbench.pid
if [ -f $PIDFILE ] ; then
	PID=`cat $PIDFILE`
	kill $PID && echo "pgbench (pid=$PID) killed"
	rm $PIDFILE
fi

