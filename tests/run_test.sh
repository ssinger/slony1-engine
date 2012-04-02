#!/bin/sh
# 

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

TESTSTARTTIME=`date +"%Y-%m-%d %H:%M:%S %Z"`


if [ ! -x "$pgbindir/psql" ]; then
  echo "please set the PGBINDIR environment variable to the directory containing psql, createdb, ..."
  exit 1;
fi

# Display the test documentation
echo "test: $testname"
echo "----------------------------------------------------"
cat $testname/README
echo "----------------------------------------------------"

#load settings

. settings.ik
. $testname/settings.ik
. support_funcs.sh

echo "Test by ${SLONYTESTER} to be summarized in ${SLONYTESTFILE}"

trap '
	echo ""
	echo "**** user abort"
	stop_processes
	exit 1
' 2 15

stop_processes()
{
	node=1
	while : ; do
	  eval workerpid=\$worker${node}_pid
	  if [ ! -z $workerpid ]; then
	    echo "**** killing worker $node"
	    kill -15 $workerpid
	  fi
	  if [ ${node} -ge ${WORKERS} ]; then
	    break;
	  else
	    node=`expr ${node} + 1`
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
	node=1
        while : ; do
	  SLON_BUILD=$PGBINDIR SLON_CONF=${mktmp}/slon-conf.${node} $SLTOOLDIR/start_slon.sh stop
          if [ ${node} -ge ${NUMNODES} ]; then
            break;
          else
            node=`expr ${node} + 1`
          fi
        done
}


init_preamble() {
	node=1
	while : ; do
	  eval cluster=\$CLUSTER${node}
	  if [ -n "${cluster}" ]; then
	    echo "CLUSTER NAME = ${cluster};" > $mktmp/slonik.preamble
	    if [ ${node} -ge ${NUMCLUSTERS} ]; then
	      break;
	    else
	      node=expr ${node} + 1
	    fi
	  else
	    break;
	  fi
	done

	node=1

	while : ; do
	  eval db=\$DB${node}
	  eval host=\$HOST${node}
	  eval user=\$USER${node}
	  eval port=\$PORT${node}
	  echo "define node${node} ${node};" >> $mktmp/slonik.preamble
	
	  if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
	    conninfo="dbname=${db} host=${host} user=${user} port=${port}"
	    echo "NODE ${node} ADMIN CONNINFO = '${conninfo}';" >> $mktmp/slonik.preamble
	    if [ ${node} -ge ${NUMNODES} ]; then
	      break;
	    else
	      node=`expr ${node} + 1`
	    fi   
	  else
	    break;
	  fi
	done
	echo "include <${mktmp}/slonik.preamble>;" > $mktmp/slonik.script
}


store_node()
{
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval oport=\$PORT${originnode}

  if [ -n "${odb}" -a "${ohost}" -a "${ouser}" -a "${oport}" ]; then
    node=1
    while : ; do
      eval db=\$DB${node}
      eval host=\$HOST${node}
      eval user=\$USER${node}
      eval port=\$PORT${node}

      if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
        if [ ${node} -ne ${originnode} ]; then
            echo "STORE NODE (id=@node${node}, comment='node ${node}', event node=${originnode});" >> $mktmp/slonik.script
        fi
        if [ ${node} -ge ${NUMNODES} ]; then
          break;
        else
          node=$((${node} + 1))
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
    eval port=\$PORT${i}

    if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
      j=1
      while : ; do
        if [ ${i} -ne ${j} ]; then
          eval bdb=\$DB${j}
          eval bhost=\$HOST${j}
          eval buser=\$WEAKUSER${j}
          eval bport=\$PORT${j}
          if [ -n "${bdb}" -a "${bhost}" -a "${buser}" -a "${bport}" ]; then
	    echo "STORE PATH (SERVER=@node${i}, CLIENT=@node${j}, CONNINFO='dbname=${db} host=${host} user=${buser} port=${port}');" >> $mktmp/slonik.script
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
        eval weakuser=\$WEAKUSER${originnode}
	eval pgbindir=\$PGBINDIR${originnode}
	eval port=\$PORT${originnode}

	if [ -n "${db}" -a "${host}" -a "${user}" ]; then
	  status "creating origin DB: $user -h $host -U $user -p $port $db"
  	  $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING -T template0 $db 1> ${mktmp}/createdb.${originnode} 2> ${mktmp}/createdb.${originnode}
	  if [ $? -ne 0 ]; then	   
	    err 3 "An error occurred trying to $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING -T template0 $db, ${mktmp}/createdb.${originnode} for details"
	  fi
	else
	  err 3 "No db '${db}' or host '${host}' or user '${user}' or port '${port}' specified"
	fi
        status "add plpgsql to Origin"
        $pgbindir/createlang -h $host -U $user -p $port plpgsql $db
	status "loading origin DB with $testname/init_schema.sql"
	$pgbindir/psql -h $host -p $port $db $user < $testname/init_schema.sql 1> ${mktmp}/init_schema.sql.${originnode} 2>${mktmp}/init_schema.sql.${originnode}
	status "setting up user ${weakuser} to have weak access to data"
	$SHELL ${testname}/gen_weak_user.sh ${weakuser} > ${mktmp}/grant_weak_access.sql
                 $pgbindir/psql -h $host -p $port -d $db -U $user < ${mktmp}/grant_weak_access.sql > ${mktmp}/genweakuser.sql.${originnode} 2> ${mktmp}/genweakuser.sql.${originnode}
	status "done"
}

create_subscribers()
{
        originnode=${ORIGINNODE:-"1"}
        eval odb=\$DB${originnode}
        eval ohost=\$HOST${originnode}
        eval ouser=\$USER${originnode}
        eval oweakuser=\$WEAKUSER${originnode}
	eval opgbindir=\$PGBINDIR${originnode}
	eval oport=\$PORT${originnode}

        if [ -n "${odb}" -a "${ohost}" -a "${ouser}" ]; then
          node=1
          while : ; do
            eval db=\$DB${node}
            eval host=\$HOST${node}
            eval user=\$USER${node}
            eval weakuser=\$WEAKUSER${node}
	    eval pgbindir=\$PGBINDIR${node}
	    eval port=\$PORT${node}

            if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
              if [ ${node} -ne ${originnode} ]; then
		status "creating subscriber ${node} DB: $user -h $host -U $user -p $port $db"
	        $pgbindir/createdb -O $user -h $host -U $user -p $port --encoding $ENCODING -T template0 $db 1> ${mktmp}/createdb.${node} 2> ${mktmp}/createdb.${node}
		status "add plpgsql to subscriber"
		$pgbindir/createlang -h $host -U $user -p $port plpgsql $db
		status "loading subscriber ${node} DB from $odb"
	        $opgbindir/pg_dump -h $ohost -U $ouser -p $oport $odb | $pgbindir/psql -h $host -p $port $db $user 1> ${mktmp}/init_schema.sql.${node} 2> ${mktmp}/init_schema.sql.${node}
		status "done"
              fi
              if [ ${node} -ge ${NUMNODES} ]; then
                break;
              else
                node=$((${node} + 1))
              fi   
            else
              break;
            fi
          done
	else
	
	  err 3 "No ORIGINNODE defined"
	fi
}

generate_weak_slony_grants ()
{
  node=1

  ROTBLS="sl_action_seq sl_config_lock sl_confirm sl_event
  sl_event_seq sl_listen sl_local_node_id sl_log_1 sl_log_2
  sl_log_status sl_node  sl_path sl_registry
  sl_seqlastvalue sl_seqlog sl_sequence sl_set sl_setsync
  sl_status sl_subscribe sl_table"

  RWTBLS="sl_nodelock sl_nodelock_nl_conncnt_seq"

  while : ; do
    eval db=\$DB${node}
    eval host=\$HOST${node}
    eval user=\$USER${node}
    eval weakuser=\$WEAKUSER${node}
    eval pgbindir=\$PGBINDIR${node}
    eval port=\$PORT${node}

    if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
      $pgbindir/psql -h $host -p $port -U $user -d $db -c "grant usage on schema \"_${CLUSTER1}\" to ${weakuser};" > /dev/null 2> /dev/null
      for table in `echo $ROTBLS`; do
        $pgbindir/psql -h $host -p $port -U $user -d $db -c "grant select on \"_${CLUSTER1}\".${table} to ${weakuser};" > /dev/null 2> /dev/null
      done
      for table in `echo $RWTBLS`; do
        $pgbindir/psql -h $host -p $port -U $user -d $db -c "grant all on \"_${CLUSTER1}\".${table} to ${weakuser};" > /dev/null 2> /dev/null
      done
    fi
    if [ ${node} -ge ${NUMNODES} ]; then
       break;
    else
       node=$((${node} + 1))
    fi   
  done    
}

drop_databases()
{
	node=1
	status "dropping database"
	while : ; do
	  eval db=\$DB${node}
	  eval host=\$HOST${node}
	  eval user=\$USER${node}
	  eval pgbindir=\$PGBINDIR${node}
	  eval port=\$PORT${node}

	  status "${db}"

	  if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
	    $pgbindir/dropdb -U $user -h $host -p $port $db 1> ${mktmp}/dropdb.${node} 2> ${mktmp}/dropdb.${node}
	    if [ ${node} -ge ${NUMNODES} ]; then
              break;
            else
              node=$((${node} + 1))
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
  conninfo="-h ${ohost} -p ${oport} -d ${odb} -U ${ouser}"
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

build_slonconf()
{
    node=$1
    conninfo=$2
    eval archive=\$ARCHIVE${node}
    CONFFILE=$mktmp/slon-conf.${node}
    echo "log_level=2" > ${CONFFILE}
    echo "vac_frequency=2" >> ${CONFFILE}
    echo "cleanup_interval=\"30 seconds\"" >> ${CONFFILE}
    echo "sync_interval=2010" >> ${CONFFILE}
    echo "sync_interval_timeout=15000" >> ${CONFFILE}
    echo "sync_group_maxsize=8" >> ${CONFFILE}
    echo "syslog=1" >> ${CONFFILE}
    echo "log_timestamp=true" >> ${CONFFILE}
    echo "log_timestamp_format='%Y-%m-%d %H:%M:%S %Z'" >> ${CONFFILE}
    echo "pid_file='${mktmp}/slon-pid.${node}'" >> ${CONFFILE}
    echo "syslog_facility=LOCAL0" >> ${CONFFILE}
    echo "syslog_ident=slon-${cluster}-${node}" >> ${CONFFILE}
    echo "cluster_name='${cluster}'" >> ${CONFFILE}
    echo "conn_info='${conninfo}'" >> ${CONFFILE}
    echo "desired_sync_time=60000" >> ${CONFFILE}
    echo "sql_on_connection=\"SET log_min_duration_statement to '1000';\"" >> ${CONFFILE}
    echo "lag_interval=\"2 seconds\"" >> ${CONFFILE}
    if [ "x${archive}" = "xtrue" ]; then
	status "slonconf configures archive logging for node ${node}"
	echo "archive_dir='${mktmp}/archive_logs_${node}'" >> ${CONFFILE}
	eval pgbindir=\$PGBINDIR${node}
    fi
}

launch_slon()
{
  node=1
  originnode=${ORIGINNODE:-"1"}
  eval odb=\$DB${originnode}
  eval ohost=\$HOST${originnode}
  eval ouser=\$USER${originnode}
  eval opgbindir=\$PGBINDIR${originnode}
  eval oport=\$PORT${originnode}
  eval cluster=\$CLUSTER1

  conninfo="dbname=${odb} host=${ohost} user=${ouser} port=${oport}"
  build_slonconf ${originnode} "${conninfo}"
  slonparms=" -f ${mktmp}/slon-conf.${originnode} "
  status "launching originnode - PGBINDIR=${PGBINDIR}"
  SLON_BUILD=${PGBINDIR} SLON_CONF=${mktmp}/slon-conf.${originnode} SLON_LOG=$mktmp/slon_log.${originnode} $SLTOOLDIR/start_slon.sh start

  sleep 1
  SLPID=`SLON_BUILD=$PGBINDIR SLON_CONF=${mktmp}/slon-conf.${originnode} $SLTOOLDIR/start_slon.sh status | grep "Slon running as PID:"`
  if [ -z "${SLPID}" ]; then
      echo "SLPID: ${SLPID}"
    warn 3 "Failed to launch slon on node ${originnode} check $mktmp/slon_log.${originnode} for details"
  fi

  sleep 10

  node=1
  while : ; do
      eval archive=\$ARCHIVE${node}
      if [ "x${archive}" = "xtrue" ]; then
	  ARCHIVENODE=${node}
      fi
      if [ ${node} -ge ${NUMNODES} ]; then
	  break;
      else
	  node=$((${node} + 1))
      fi
  done
  status "Archive node: ${ARCHIVENODE}"

  node=1
  while : ; do
    if [ ${node} -ne ${originnode} ]; then

      eval db=\$DB${node}
      eval host=\$HOST${node}
      eval user=\$USER${node}
      eval pgbindir=\$PGBINDIR${node}
      eval port=\$PORT${node}
      eval cluster=\$CLUSTER1
      eval archive=\$ARCHIVE${node}

      if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
        unset conninfo
        unset tmppid
        unset tmpppid
        eval slon${node}_pid=
	status "Considering node ${node}"

        if [ "x${archive}" = "xtrue" ]; then
          status "Creating log shipping directory - $mktmp/archive_logs_${node}"
          mkdir -p $mktmp/archive_logs_${node}
          archiveparm="-a ${mktmp}/archive_logs_${node}"
	else
	  archiveparm=""
        fi
        conninfo="dbname=${db} host=${host} user=${user} port=${port}"

	build_slonconf ${node} "${conninfo}"
	status "launching slon"
	SLON_BUILD=$PGBINDIR SLON_CONF=${CONFFILE} SLON_LOG=$mktmp/slon_log.${node} $SLTOOLDIR/start_slon.sh start
	sleep 1
	
	SLPID=`SLON_BUILD=${PGBINDIR} SLON_CONF=${mktmp}/slon-conf.${node} $SLTOOLDIR/start_slon.sh status | grep "Slon running as PID:"`
	if [ -z "${SLPID}" ]; then
	    echo "SLPID: ${SLPID}"
	    warn 3 "Failed to launch slon on node ${node} check $mktmp/slon_log.${node} for details"
	fi
      fi
    fi
    if [ ${node} -ge ${NUMNODES} ]; then
      break;
    else
      node=$((${node} + 1))
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

  node=1
  while : ; do
    eval cluster=\$CLUSTER${node}
    if [ -n "${cluster}" ]; then
      SQL="SELECT * FROM \"_${cluster}\".sl_status"
      ${opgbindir}/psql -c "${SQL}" -h ${ohost} -p ${oport} ${odb} ${ouser}
      if [ ${node} -ge ${NUMCLUSTERS} ]; then
        break;
      else
        node=`expr ${node} + 1`
      fi
    else
      break;
    fi
  done
}      

# wait_for_catchup()
# {
#   node=1
#   status "waiting for nodes to catch up"

#   poll_cluster

#   sleep 20

#   poll_cluster
#   status "done"
# }

wait_for_catchup ()
{
    eval onode=${ORIGINNODE:-"1"}
    status "submit SYNC to node ${onode}, wait for event to propagate to all nodes..."
    echo "include <${mktmp}/slonik.preamble>;" > $mktmp/wait-for-propagation.slonik
    echo "sync (ID=${onode});" >> $mktmp/wait-for-propagation.slonik
    echo "wait for event (origin=${onode},confirmed=ALL,wait on=${onode});" >> $mktmp/wait-for-propagation.slonik
    $pgbindir/slonik < $mktmp/wait-for-propagation.slonik > $mktmp/wait-for-propagation.log 2>&1
    status "...event propagated to all nodes"
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
  node=1
  while : ; do
    eval db=\$DB${node}
    eval host=\$HOST${node}
    eval user=\$USER${node}
    eval port=\$PORT${node}
    eval pgbindir=\$PGBINDIR${node}

    if [ -n "${db}" -a "${host}" -a "${user}" -a "${port}" ]; then
      if [ ${node} -ne ${originnode} ]; then
        if [ -f ${mktmp}/db_${node}.dmp ]; then
          err 3 "${mktmp}/db_${node}.dmp exists but should not"
        fi
	status "getting data from node ${node} for diffing against origin"
        cat ${testname}/schema.diff | while read SQL; do
          ${pgbindir}/psql -h ${host} -c "${SQL}" -p ${port} ${db} ${user} >> ${mktmp}/db_${node}.dmp
        done
	status "comparing"
	diff ${mktmp}/db_${originnode}.dmp ${mktmp}/db_${node}.dmp > ${mktmp}/db_diff.${node}
        if [ $? -ne 0 ]; then
          warn 3 "${mktmp}/db_${originnode}.dmp ${mktmp}/db_${node}.dmp differ, see ${mktmp}/db_diff.${node} for details"
	else
	  echo "subscriber node ${node} is the same as origin node ${originnode}"
        fi
	status "done"
      fi
    else
      err 3 "Not Found ${db} -a ${host} ${user}"
    fi
    if [ ${node} -ge ${NUMNODES} ]; then
      break;
    else
      node=$((${node} + 1))
    fi
  done
}

TMPDIR=${TMPDIR:-"/tmp"}    # Consider either a default value, or /tmp
if [ -d ${TMPDIR} ]; then
    T_TMPDIR=${TMPDIR}
else
    for i in /tmp /usr/tmp /var/tmp; do
	if [ -d ${i} ]; then
	    TMPDIR=$i
	    break
	fi
    done
fi

case `uname` in
  SunOS|AIX|MINGW32*)
    if [ -z $TMPDIR ]; then
       err 3 "unable to create temp dir"
       exit
    fi

    # do this 10 times else fail
    rstring=$(random_az 8)
    rstring=slony-regress.$rstring
    
    mkdir ${TMPDIR}/$rstring
    retcode=$?
    if [ $retcode -ne 0 ]; then
      err $retcode "unable to create temp dir"
    else
      mktmp=${TMPDIR}/$rstring
    fi
  ;;
  Linux)
    mktmp=`mktemp -d -t slony-regress.XXXXXX`
    if [ $MY_MKTEMP_IS_DECREPIT ] ; then
        mktmp=`mktemp -d ${TMPDIR}/slony-regress.XXXXXX`
    fi
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

echo ${testname} > ${mktmp}/TestName

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

status "Granting weak access on Slony-I schema"
generate_weak_slony_grants
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

gen_testinfo
drop_databases
cleanup
