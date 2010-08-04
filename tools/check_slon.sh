#!/bin/sh
# 

# nagios plugin that checks whether the slon daemon is running
# if the 3rd parameter (LOGFILE) is specified then the log file is
# checked to see if the last entry is a WARN or FATAL message
#
# three possible exit statuses:
#  0 = OK
#  1 = Warning (warning in slon log file)
#  2 = Fatal Error (slon not running, or error in log file)
#
# script requires two or three parameters:
# CLUSTERNAME - name of slon cluster to be checked
# DBNAME - name of database being replicated
# LOGFILE - (optional) location of the slon log file
#
# Author:  John Sidney-Woollett
# Created: 26-Feb-2005
# Copyright 2005-2009

# check parameters are valid
if [ $# -lt 2 ] || [ $# -gt 3 ]
then
   echo "Invalid parameters need CLUSTERNAME DBNAME DBHOST [LOGFILE]"
   exit 2
fi

# assign parameters
CLUSTERNAME=$1
DBNAME=$2
LOGFILE=$3

# check to see whether the slon daemon is running
case `uname` in
Linux) PSCOMMAND="ps auxww" ;;
SunOS) PSCOMMAND="/usr/ucb/ps -auxww" ;;
FreeBSD) PSCOMMAND="/bin/ps -auxww" ;;
AIX) PSCOMMAND="/usr/bin/ps auxww" ;;
*) PSCOMMAND="ps auxww"
esac

SLONPROCESS=`$PSCOMMAND | egrep "[s]lon $CLUSTERNAME" | egrep "dbname=$DBNAME" | awk '{print $2}'`

if [ ! -n "$SLONPROCESS" ]
then
   echo "no slon process active"
   exit 2
fi

# if the logfile is specified, check it exists
# and check for the word ERROR or WARN in the last line
if [ -n "$LOGFILE" ]
then
   # check for log file
   if [ -f "$LOGFILE" ]
   then
     LOGLINE=`tail -1 $LOGFILE`
     LOGSTATUS=`tail -1 $LOGFILE | awk '{print $1}'`
     if [ $LOGSTATUS = "FATAL" ]
     then
       echo "$LOGLINE"
       exit 2
     elif [ $LOGSTATUS = "WARN" ]
     then
       echo "$LOGLINE"
       exit 1
     fi
   else
     echo "$LOGFILE not found"
     exit 2
   fi
fi

# otherwise all looks to be OK
echo "OK - slon process $SLONPROCESS"
exit 0



