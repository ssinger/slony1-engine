#!/bin/sh

pgbindir=${PGBINDIR:-"/usr/local/pgsql/bin"}
numerrors=0
testname=$1

if [ -z "$testname" ]; then
  echo "usage $0 testname"
  exit 1;
fi

if [ ! -d "$testname" ]; then
  echo "No such test $testname"
  exit 1;
fi

if [ -z "$2" ]; then
  case `uname` in
    SunOS*)
      bash $0 $1 1
      retcode=$?
      if [ $retcode -eq 127 ]; then
        echo "Under SunOS (Solaris) we require you have bash installed, and in your path $testname"
      fi
      exit $retcode
    ;;
    AIX*)
       bash $0 $1 1
       retcode=$?
       if [ $retcode -eq 127 ]; then
         echo "Under AIX we require you have bash installed, and in your path to run $testname"
       fi
       exit $retcode
    ;;
    *)
    ;;
  esac
fi

if [ ! -x "$pgbindir/psql" ]; then
  echo "please set the PGBINDIR envvar to the directory containing psql, createdb, ..."
  exit 1;
fi

#load settings

. settings.ik
. $testname/settings.ik
. support_funcs.sh

trap '
	echo ""
	echo "**** user abort"
	stop_processes
	exit 1
' 2 15

stop_processes()
{
	alias=1
	while : ; do
	  eval workerpid=\$worker${alias}_pid
	  if [ ! -z $workerpid ]; then
	    echo "**** killing worker $alias"
	    kill -15 $workerpid
	  fi
	  if [ ${alias} -ge ${WORKERS} ]; then
	    break;
	  else
	    alias=`expr ${alias} + 1`
	  fi
	done
	stop_poll
	stop_slons
}

stop_poll()
{
	if [ ! -z ${poll_pid} ]; then
          case `uname` in
            MINGW32*)
              foo=`ps |awk '{ print $1 }'| grep ${poll_pid}`
              if [ ! -z "$foo" ]; then
  	        echo "***** killing poll_cluster"
	        kill -15 ${poll_pid}
              fi
            ;;
            *)
	      ps -p ${poll_pid} >/dev/null
              if [ $? -eq 0 ]; then
  	        echo "***** killing poll_cluster"
	        kill -15 ${poll_pid}
	      fi
            ;;
          esac
        fi
}

stop_slons()
{
	alias=1
        while : ; do
          eval slonpid=\$slon${alias}_pid
	  if [ ! -z $slonpid ]; then
            echo "**** killing slon node $alias"
            kill -15 $slonpid
	  fi
          if [ ${alias} -ge ${NUMNODES} ]; then
            break;
          else
            alias=`expr ${alias} + 1`
          fi
        done
}


init_preamble() {
	alias=1
	while : ; do
	  eval cluster=\$CLUSTER${alias}
	  if [ -n "${cluster}" ]; then
	    echo "CLUSTER NAME = ${cluster};" > $mktmp/slonik.script
	    if [ ${alias} -ge ${NUMCLUSTERS} ]; then
	      break;
	    else
	      alias=expr ${alias} + 1
	    fi
	  else
	    break;
	  fi
	done

	alias=1

	while : ; do
	  eval db=\$DB${alias}
	  eval host=\$HOST${alias}
	  eval user=\$USER${alias}
	
	  if [ -n "${db}" -a "${host}" -a "${user}" ]; then
	    conninfo="dbname=${db} host=${host} user=${user}"
	    echo "NODE ${alias} ADMIN CONNINFO = '${conninfo}';" >> $mktmp/slonik.script
	    if [ ${alias} -ge ${NUMNODES} ]; then
	      break;
	    else
	      alias=`expr ${alias} + 1`
	    fi   
	  else
	    break;
	  fi
	done
}


store_node()
{
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}

  if [ -n "${odb}" -a "${ohost}" -a "${ouser}" ]; then
    alias=1
    while : ; do
      eval db=\$DB${alias}
      eval host=\$HOST${alias}
      eval user=\$USER${alias}

      if [ -n "${db}" -a "${host}" -a "${user}" ]; then
        if [ ${alias} -ne ${originnode} ]; then
          echo "STORE NODE (id=${alias}, comment='node ${alias}');" >> $mktmp/slonik.script
        fi
        if [ ${alias} -ge ${NUMNODES} ]; then
          break;
        else
          alias=$((${alias} + 1))
        fi
      else
        break;
      fi
    done
  else
    err 3 "No origin in  ${odb} ${ohost} ${ouser}"
  fi
}

store_path()
{
  i=1
  while : ; do
    eval db=\$DB${i}
    eval host=\$HOST${i}
    eval user=\$USER${i}

    if [ -n "${db}" -a "${host}" -a "${user}" ]; then
      j=1
      while : ; do
        if [ ${i} -ne ${j} ]; then
          eval bdb=\$DB${j}
          eval bhost=\$HOST${j}
          eval buser=\$USER${j}
          if [ -n "${bdb}" -a "${bhost}" -a "${buser}" ]; then
	    echo "STORE PATH (SERVER=${i}, CLIENT=${j}, CONNINFO='dbname=${db} host=${host} user=${user}');" >> $mktmp/slonik.script
          else
            err 3 "No conninfo"
          fi
        fi
        if [ ${j} -ge ${NUMNODES} ]; then
          break;
        else
          j=$((${j} + 1))
        fi
      done
      if [ ${i} -ge ${NUMNODES} ]; then
        break;
      else
        i=$((${i} +1))
      fi
    else
      err 3 "no DB"
    fi
  done
}

init_origin_rdbms()
{
	originnode=${ORIGINNODE:-"1"}
        eval db=\$DB${originnode}
        eval host=\$HOST${originnode}
        eval user=\$USER${originnode}
	eval pgbindir=\$PGBINDIR${originnode}
	eval port=\$PORT${originnode}

	if [ -n "${db}" -a "${host}" -a "${user}" ]; then
	  status "creating origin DB: $user -h $host -U $user -p $port $db"
  	  $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING $db 1> ${mktmp}/createdb.${originnode} 2> ${mktmp}/createdb.${originnode}
	  if [ $? -ne 0 ]; then	   
	    err 3 "An error occured trying to $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING $db, ${mktmp}/createdb.${originnode} for details"
	  fi
	else
	  err 3 "No db '${db}' or host '${host}' or user '${user}' or port '${port}' specified"
	fi
	status "loading origin DB with $testname/init_schema.sql"
	$pgbindir/psql -h $host -p $port $db $user < $testname/init_schema.sql 1> ${mktmp}/init_schema.sql.${originnode} 2>${mktmp}/init_schema.sql.${originnode}
	status "done"
}

create_subscribers()
{
        originnode=${ORIGINNODE:-"1"}
        eval odb=\$DB${originnode}
        eval ohost=\$HOST${originnode}
        eval ouser=\$USER${originnode}
	eval opgbindir=\$PGBINDIR${originnode}
	eval oport=\$PORT${originnode}

        if [ -n "${odb}" -a "${ohost}" -a "${ouser}" ]; then
          alias=1
          while : ; do
            eval db=\$DB${alias}
            eval host=\$HOST${alias}
            eval user=\$USER${alias}
	    eval pgbindir=\$PGBINDIR${alias}
	    eval port=\$PORT${alias}

            if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
              if [ ${alias} -ne ${originnode} ]; then
		status "creating subscriber ${alias} DB: $user -h $host -U $user -p $port $db"
	        $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING $db 1> ${mktmp}/createdb.${alias} 2> ${mktmp}/createdb.${alias}
		status "loading subscriber ${alias} DB from $odb"
	        $opgbindir/pg_dump -s  -h $ohost -U $ouser -p $oport $odb | $pgbindir/psql -h $host -p $port $db $user 1> ${mktmp}/init_schema.sql.${alias} 2> ${mktmp}/init_schema.sql.${alias}
		status "done"
              fi
              if [ ${alias} -ge ${NUMNODES} ]; then
                break;
              else
                alias=$((${alias} + 1))
              fi   
            else
              break;
            fi
          done
	else
	
	  err 3 "No ORIGINNODE defined"
	fi
}

drop_databases()
{
	alias=1
	status "dropping database"
	while : ; do
	  eval db=\$DB${alias}
	  eval host=\$HOST${alias}
	  eval user=\$USER${alias}
	  eval pgbindir=\$PGBINDIR${alias}
	  eval port=\$PORT${alias}

	  status "${db}"

	  if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
	    $pgbindir/dropdb -U $user -h $host -p $port $db 1> ${mktmp}/dropdb.${alias} 2> ${mktmp}/dropdb.${alias}
	    if [ ${alias} -ge ${NUMNODES} ]; then
              break;
            else
              alias=$((${alias} + 1))
            fi
	  else
            break;
          fi
        done
	status "done"
}

cleanup()
{
	if [ ${numerrors} -eq 0 ]; then
		rm -rf $mktmp
	        echo "***************************"
        	echo "test ${testname} completed successfully"
	        echo "***************************"
	else
		echo "there were ${numerrors} warnings during the run of $testname, check the files in $mktmp for more details"
	fi
}

create_set()
{
	if [ -f $testname/init_create_set.ik ]; then
	  cat $testname/init_create_set.ik >> $mktmp/slonik.script
	fi
}

add_tables()
{	if [ -f $testname/init_add_tables.ik ]; then
	  cat $testname/init_add_tables.ik >> $mktmp/slonik.script
	fi
}

init_cluster()
{
	if [ -f $testname/init_cluster.ik ]; then
	  cat $testname/init_cluster.ik >> $mktmp/slonik.script
	fi
}

subscribe_set()
{
	if [ -f $testname/init_subscribe_set.ik ]; then
          cat $testname/init_subscribe_set.ik >> $mktmp/slonik.script
	fi
}

load_data()
{
        eval odb=\$DB${originnode}
        eval ohost=\$HOST${originnode}
        eval ouser=\$USER${originnode}
	eval opgbindir=\$PGBINDIR${originnode}
	eval oport=\$PORT${originnode}
	if [ -n "${odb}" -a "${ohost}" -a "${ouser}" ]; then
		
	  if [ -f $testname/init_data.sql ]; then
	    $opgbindir/psql -h $ohost -p $oport $odb $ouser < $testname/init_data.sql 1> $mktmp/init_data.sql.log 2> $mktmp/init_data.sql.log
	    if [ $? -ne 0 ]; then
              warn 3 "$testname/init_data.sql generated an error see $mktmp/init_data.sql.log for details"
	    fi
	  fi
        else
          err 3 "No origin found in ${odb} ${ohost} ${ouser} ${oport}"
	fi
}


do_ik()
{
	if [ -f $mktmp/slonik.script ]; then
#	  cat $mktmp/slonik.script >`tty`
	  $pgbindir/slonik < $mktmp/slonik.script > $mktmp/slonik.log 2>&1
	  if [ $? -ne 0 ]; then
	    err 3 "Slonik error see $mktmp/slonik.log for details"
	    cat $mktmp/slonik.log
	  fi
	fi
}

launch_poll()
{
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval opgbindir=\$PGBINDIR${originnode}
  eval oport=\$PORT${originnode}
  eval cluster=\$CLUSTER1
  conninfo="-h ${ohost} -p ${oport} ${odb} ${ouser}"
  status "launching polling script"
  case `uname` in
    MINGW32*)
      ./poll_cluster.sh "$mktmp" "${cluster}" "${conninfo}" "${opgbindir}" /dev/tty &
      poll_pid=$!
      ;;
    *)
      if [ "x$SHLVL" != "x" ]; then
        ./poll_cluster.sh "$mktmp" "${cluster}" "${conninfo}" "${opgbindir}" `tty` &
      else
        ./poll_cluster.sh "$mktmp" "${cluster}" "${conninfo}" "${opgbindir}" /dev/null &
      fi
      tmppid=$!
      tmpppid=$$
      sleep 1
      foo=$(_check_pid poll_cluster ${tmppid} ${tmpppid})
      poll_pid=${foo}
      if [ -z "${foo}" -o "${tmppid}" != "${foo}" ]; then
        warn 3 "Failed to launch poll_cluster check $mktmp/poll.log for details"
      fi
      ;;
  esac
}

launch_slon()
{
  alias=1
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval opgbindir=\$PGBINDIR${originnode}
  eval oport=\$PORT${originnode}
  eval cluster=\$CLUSTER1

  conninfo="dbname=${odb} host=${ohost} user=${ouser} port=${oport}"
  status "launching originnode : $opgbindir/slon -s500 -g10 $cluster \"$conninfo\""
  $opgbindir/slon -s500 -g10 $cluster "$conninfo" 1> $mktmp/slon_log.${originnode} 2> $mktmp/slon_log.${originnode} &
  tmppid=$!
  tmpppid=$$
  sleep 1
  foo=$(_check_pid slon ${tmppid} ${tmpppid})
          
  eval slon${originnode}_pid=${foo}
  if [ -z "${foo}" -o "${tmppid}" != "${foo}" ]; then
    warn 3 "Failed to launch slon on node ${originnode} check $mktmp/slon_log.${originnode} for details"
  fi

  sleep 10

  while : ; do
    if [ ${alias} -ne ${originnode} ]; then

      eval db=\$DB${alias}
      eval host=\$HOST${alias}
      eval user=\$USER${alias}
      eval pgbindir=\$PGBINDIR${alias}
      eval port=\$PORT${alias}
      eval cluster=\$CLUSTER1

      if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
        unset conninfo

        unset tmppid
        unset tmpppid
        eval slon${alias}_pid=

        conninfo="dbname=${db} host=${host} user=${user} port=${port}"

        status "launching: $pgbindir/slon -s500 -g10 $cluster \"$conninfo\""

        $pgbindir/slon -s500 -g10 $cluster "$conninfo" 1>> $mktmp/slon_log.${alias} 2>&1 &
        tmppid=$!
        tmpppid=$$
        sleep 1

        foo=$(_check_pid slon ${tmppid} ${tmpppid})


        eval slon${alias}_pid=${foo}
        if [ -z "${foo}" -o "${tmppid}" != "${foo}" ]; then
          warn 3 "Failed to launch slon on node ${alias} check $mktmp/slon_log.${alias} for details"
        fi
      fi
    fi
    if [ ${alias} -ge ${NUMNODES} ]; then
      break;
    else
      alias=$((${alias} + 1))
    fi
  done
}

poll_cluster()
{
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval opgbindir=\$PGBINDIR${originnode}
  eval oport=\$PORT${originnode}

  alias=1
  while : ; do
    eval cluster=\$CLUSTER${alias}
    if [ -n "${cluster}" ]; then
      SQL="SELECT * FROM \"_${cluster}\".sl_status"
      ${opgbindir}/psql -c "${SQL}" -h ${ohost} -p ${oport} ${odb} ${ouser}
      if [ ${alias} -ge ${NUMCLUSTERS} ]; then
        break;
      else
        alias=expr ${alias} + 1
      fi
    else
      break;
    fi
  done
}      

wait_for_catchup()
{
  alias=1
  status "waiting for nodes to catchup"

  poll_cluster

  sleep 20

  poll_cluster
  status "done"
}

diff_db()
{
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval opgbindir=\$PGBINDIR${originnode}
  eval oport=\$PORT${originnode}

  if [ -f ${mktmp}/db_${originnode}.dmp ]; then
    err 3 "${mktmp}/db_${originnode}.dmp exists but should not"
  fi
  status "getting data from origin DB for diffing"
  cat ${testname}/schema.diff | while read SQL; do
    ${opgbindir}/psql -c "${SQL}" -h ${ohost} -p ${oport} ${odb} ${ouser} >> ${mktmp}/db_${originnode}.dmp
  done
  status "done"
  alias=1
  while : ; do
    eval db=\$DB${alias}
    eval host=\$HOST${alias}
    eval user=\$USER${alias}
    eval port=\$PORT${alias}
    eval pgbindir=\$PGBINDIR${alias}

    if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
      if [ ${alias} -ne ${originnode} ]; then
        if [ -f ${mktmp}/db_${alias}.dmp ]; then
          err 3 "${mktmp}/db_${alias}.dmp exists but should not"
        fi
	status "getting data from node ${alias} for diffing against origin"
        cat ${testname}/schema.diff | while read SQL; do
          ${pgbindir}/psql -h ${host} -c "${SQL}" -p ${port} ${db} ${user} >> ${mktmp}/db_${alias}.dmp
        done
	status "comparing"
	diff ${mktmp}/db_${originnode}.dmp ${mktmp}/db_${alias}.dmp > ${mktmp}/db_diff.${alias}
        if [ $? -ne 0 ]; then
          warn 3 "${mktmp}/db_${originnode}.dmp ${mktmp}/db_${alias}.dmp differ, see ${mktmp}/db_diff.${alias} for details"
	else
	  echo "subscriber node ${alias} is the same as origin node ${originnode}"
        fi
	status "done"
      fi
    else
      err 3 "Not Found ${db} -a ${host} ${user}"
    fi
    if [ ${alias} -ge ${NUMNODES} ]; then
      break;
    else
      alias=$((${alias} + 1))
    fi
  done
}

case `uname` in
  SunOS|AIX|MINGW32*)
        for i in /tmp /usr/tmp /var/tmp; do
      if [ -d $i ]; then
        tmpdir=$i
        break
      fi
    done
    if [ -z $tmpdir ]; then
       err 3 "unable to create tmp dir"
       exit
    fi

    # do this 10 times else fail
    rstring=$(random_az 8)
    rstring=slony-regress.$rstring
    
    mkdir $tmpdir/$rstring
    retcode=$?
    if [ $retcode -ne 0 ]; then
      err $retcode "unable to create temp dir"
    else
      mktmp=$tmpdir/$rstring
    fi
  ;;
  Linux)
    mktmp=`mktemp -d -t slony-regress.XXXXXX`
    if [ ! -d $mktmp ]; then
      err 3 "mktemp failed"
    fi
  ;;
  *)
    mktmp=`mktemp -d -t slony-regress`
    if [ ! -d $mktmp ]; then
      err 3 "mktemp failed"
    fi
  ;;
esac



. $testname/generate_dml.sh

init_origin_rdbms

load_data
create_subscribers

init_preamble
init_cluster
create_set
add_tables
status "creating cluster"
do_ik
status "done"

status "storing nodes"
init_preamble
store_node
do_ik
status "done"

status "storing paths"
init_preamble
store_path
do_ik
status "done"

launch_slon
status "subscribing"
init_preamble
subscribe_set
do_ik 
status "done"

do_initdata
if [ ! -z ${poll_pid} ]; then
  wait ${poll_pid}
fi

diff_db

stop_processes

status "waiting for slons to die"
sleep 5
status "done"

drop_databases
cleanup
