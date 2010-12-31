#!/bin/sh
# 

mktmp=$1
cluster=$2
conninfo=$3
pgbindir=$4
tty=$5

if [ -z "$6" ]; then
  case `uname` in
    SunOS*)
      bash "$0" "$1" "$2" "$3" "$4" "$5" 1
      retcode=$?
      if [ $retcode -eq 127 ]; then
        echo "Under SunOS (Solaris) we require you have bash installed, and in your path $testname"
      fi
      exit $retcode
    ;;
    AIX*)
      bash "$0" "$1" "$2" "$3" "$4" "$5" 1
      retcode=$?
      if [ $retcode -eq 127 ]; then
        echo "Under AIX we require you have bash installed, and in your path $testname"
      fi
      exit $retcode
    ;;
    *)
    ;;
  esac
fi
. ./support_funcs.sh

if [ -n "${cluster}" ]; then
  sleep 15
  SQL="select count(*) from \"_${cluster}\".sl_log_1 l 
where not(txid_visible_in_snapshot(l.log_txid, (select ev_snapshot from \"_${cluster}\".sl_event where ev_timestamp = (select max(ev_timestamp) from \"_${cluster}\".sl_event) limit 1)));"
  SQL2="SELECT max(st_lag_num_events) FROM \"_${cluster}\".sl_status"
  while : ; do
    lag=`${pgbindir}/psql -q -A -t -c "${SQL}" ${conninfo} 2>>$mktmp/poll.log | sed -e '/^$/d'`
    if [ ! -z "${lag}" ]; then
      if [ ${lag} -eq 0 ]; then
	    sleep 3
        lag=`${pgbindir}/psql -q -A -t -c "${SQL}" ${conninfo} 2>>$mktmp/poll.log | sed -e '/^$/d'`
        if [ ${lag} -eq 0 ]; then
          sleep 2
          lag=`${pgbindir}/psql -q -A -t -c "${SQL}" ${conninfo} 2>>$mktmp/poll.log | sed -e '/^$/d'`
          lag2=`${pgbindir}/psql -q -A -t -c "${SQL2}" ${conninfo} 2>>$mktmp/poll.log | sed -e '/^$/d'`
          lag3=`expr $lag + $lag2`
	      if [ ${lag3} -eq 0 ]; then
            echo "slony is caught up" > $tty
            exit
          else
            echo "$lag3 rows+events behind" > $tty
	        sleep 1
          fi
        else
          echo "$lag rows behind" > $tty
          sleep 4
        fi
      else
	    echo "$lag rows behind" > $tty
        sleep 5
      fi
    fi
  done
else
  err 2 "$0 has no cluster - dying"
fi
