#!/bin/sh
# 

# nagios plugin that checks whether the slave nodes in a slony cluster
# are being updated from the master
#
# possible exit statuses:
#  0 = OK
#  2 = Error, one or more slave nodes are not sync'ing with the master
#
# script requires two parameters:
# CLUSTERNAME - name of slon cluster to be checked
# DBNAME - name of master database
# DBHOST - host name of master database
#
# It also depends on PGPORT being set to the appropriate port
#
# Author:  John Sidney-Woollett
# Created: 26-Feb-2005
# Copyright 2005-2009

# check parameters are valid
if [ $# -ne 3 ]
then
   echo "Invalid parameters need CLUSTERNAME DBNAME DBHOST"
   exit 2
fi

# assign parameters
CLUSTERNAME=$1
DBNAME=$2
DBHOST=$3

# setup the query to check the replication status
SQL="select case
   when ttlcount = okcount then 'OK - '||okcount||' nodes in sync'
   else 'ERROR - '||ttlcount-okcount||' of '||ttlcount||' nodes not in sync'
end as syncstatus
from (
-- determine total active receivers
select (select count(distinct sub_receiver)
     from \"_$CLUSTERNAME\".sl_subscribe
     where sub_active = true) as ttlcount,
(
-- determine active nodes syncing within 10 seconds
  select count(*) from (
   select st_received, st_last_received_ts - st_last_event_ts as cfmdelay
   from \"_$CLUSTERNAME\".sl_status
   where st_received in (
     select distinct sub_receiver
     from \"_$CLUSTERNAME\".sl_subscribe
     where sub_active = true
   )
) as t1
where cfmdelay < interval '10 secs') as okcount
) as t2"

# query the master database
CHECK=`psql -c "$SQL" --tuples-only -U postgres -h $DBHOST $DBNAME`

if [ ! -n "$CHECK" ]
then
   echo "ERROR querying $DBNAME"
   exit 2
fi

# echo the result of the query
echo $CHECK

# and check the return status
STATUS=`echo $CHECK | awk '{print $1}'`
NODESOK=`echo $CHECK | awk '{print $3}'`
if [ $STATUS = "OK" ] && [ $NODESOK != "0" ]
then
   exit 0
else
   exit 2
fi
