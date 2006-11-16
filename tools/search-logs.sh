#!/bin/sh 
# $Id: search-logs.sh,v 1.1 2006-11-16 19:01:56 cbbrowne Exp $ 
 
# Search logs for errors from the last hour 
LOGHOME=${LOGHOME:-"/opt/logs"}    # Directory to search
LOGRECIPIENT=${LOGRECIPIENT:-""}   # Email recipient - if not set, use STDOUT
LOGTIMESTAMP=${LOGTIMESTAMP:-""}   # Override time - format should be "YYYY-MM-DD HH"

if [[ -z $LOGTIMESTAMP ]] ; then
    HRRE=`date -d "1 hour ago" +"%Y-%m-%d %H:[0-9][0-9]:[0-9][0-9] ${TZ}"`
else
    HRRE="${LOGTIMESTAMP}:[0-9][0-9]:[0-9][0-9] ${TZ}"
fi

for log in `find ${LOGHOME} -name "*.log" -mmin -60 | egrep "/node[0-9]+/[^/]+.log"` ; do 
    egrep "${HRRE} (ERROR|FATAL)" $log > /tmp/slony-errors.$$ 
    if [[ -s /tmp/slony-errors.$$ ]] ; then 
        echo "
Errors in log ${log} 
===============================================================" >> /tmp/slony-summary.$$ 
        cat /tmp/slony-errors.$$ >> /tmp/slony-summary.$$ 
    fi 
done 
 
if [[ -s /tmp/slony-summary.$$ ]] ; then 
    if [[ -z $LOGRECIPIENT ]] ; then
	echo "Errors found!"
	cat /tmp/slony-summary.$$  
    else
	mail -s "Slony-I log errors for ${HRRE}" ${LOGRECIPIENT} < /tmp/slony-summary.$$
    fi
fi 
rm -f /tmp/slony-errors.$$ /tmp/slony-summary.$$
