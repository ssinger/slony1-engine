#!/bin/sh
# 

# The following lines are ones you may wish to customize;
# alternatively, you may set SLON_BUILD and SLON_CONF in your
# environment to override the values in this script.

SLON_BIN_PATH=${SLON_BUILD:-"/home/chris/dbs/postgresql-8.3.3/bin"}
SLON_CONF=${SLON_CONF:-"${HOME}/test/slon-conf.1"}
SLON_LOG=${SLON_LOG:-"${HOME}/test/slon.1.log"}    # If you use syslog, then this may use /dev/null

# shouldn't need to edit anything below this

test -x "$SLON_BIN_PATH/slon" || (echo "missing slon - ${SLON_BIN_PATH}/slon"; exit 1)
test -r "$SLON_CONF" || (echo "No slon conf file - $SLON_CONF"; exit 1)

PID_LINE=`grep pid_file $SLON_CONF | cut -d "#" -f 1 | grep "^[[:space:]]*pid_file='.*'"`
PID_FILE=`echo $PID_LINE | cut -d "=" -f 2 | cut -d "'" -f 2`
if [ "x$PID_FILE" == "x" ]; then
    echo "pid_file not found in slon conf file - $SLON_CONF"
    exit 1
else
    if [ -f $PID_FILE ]; then
       PID=`cat $PID_FILE`
       FINDPID=`ps -p ${PID} | awk '{print $1}' | grep "^$PID\$"`
    fi
fi

case "$1" in
  start)
        if [ ! -z "$FINDPID" ]; then
	    echo "**** slon already running - PID $PID ****"
	    exit 1
	fi
	touch $SLON_LOG
	test -w "$SLON_LOG" || (echo "**** SLON_LOG not writable - $SLON_LOG ****"; exit 1)
        echo "Starting slon: $SLON_BIN_PATH/slon -f ${SLON_CONF} 1>> ${SLON_LOG} 2>&1 &"
	$SLON_BIN_PATH/slon -f ${SLON_CONF} 1>> ${SLON_LOG} 2>&1 &
        ;;
  stop)
        echo "Stopping slon"
	if [ ! -z "$FINDPID" ]; then
	    kill -15 ${PID}
	    echo "Killed slon at PID ${PID}"
	else
	    echo "**** slon with PID ${PID} not found ****"
        fi
        ;;
  status)
        echo "SLON_CONF:${SLON_CONF}"
	echo "SLON_BIN_PATH:${SLON_BIN_PATH}"
	if [ -f $PID_FILE ]; then
	    if [ ! -z "$FINDPID" ]; then
		echo "**** Slon running as PID:$PID ****"
	    else
		echo "**** Slon not running - PID:$PID - ${FINDPID} ****"
	    fi
	else
	    echo "**** Slon not running - no PID file ${PID_FILE} ****"
	fi
	;;
  *)
        echo "Usage: $0 [start|stop|status]"
        ;;
esac

