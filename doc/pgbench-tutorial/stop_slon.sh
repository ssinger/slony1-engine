#!/bin/sh

if [ $# -ne 1 ] ; then
	echo "usage: $0 NODE_ID" >&2
	exit 2
fi

PIDFILE=./.status/slon_${1}.pid
if [ -f $PIDFILE ] ; then
	PID=`cat $PIDFILE`
	kill $PID && echo "slon process for node $1 (pid=$PID) killed"
	rm $PIDFILE
fi

