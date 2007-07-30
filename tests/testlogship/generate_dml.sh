. support_funcs.sh

init_dml()
{
  echo "init_dml()"
}

begin()
{
  echo "begin()"
}

rollback()
{
  echo "rollback()"
}

commit()
{
  echo "commit()"
}

generate_initdata()
{
  numrows=$(random_number 50 1000)
  i=0;
  trippoint=`expr $numrows / 20`
  j=0;
  percent=0
  status "generating ${numrows} transactions of random data"
  percent=`expr $j \* 5`
  status "$percent %"
  GENDATA="$mktmp/generate.data"
  echo "" > ${GENDATA}
  while : ; do
    txtalen=$(random_number 1 100)
    txta=$(random_string ${txtalen})
    txta=`echo ${txta} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`
    txtblen=$(random_number 1 100)
    txtb=$(random_string ${txtblen})
    txtb=`echo ${txtb} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`
    ra=$(random_number 1 9)
    rb=$(random_number 1 9)
    rc=$(random_number 1 9)
    echo "INSERT INTO table1(data) VALUES ('${txta}');" >> $GENDATA
    echo "INSERT INTO table2(table1_id,data) SELECT id, '${txtb}' FROM table1 WHERE data='${txta}';" >> $GENDATA
    echo "INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data ='${txtb}';" >> $GENDATA
    echo "INSERT INTO table5(numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values ('${ra}${rb}.${rc}','${ra}.${rb}${rc}','(${ra},${rb})','((${ra},${ra}),(${rb},${rb}),(${rc},${rc}),(${ra},${rc}))','((${ra},${rb}),(${rc},${ra}),(${rb},${rc}),(${rc},${rb}))','<(${ra},${rb}),${rc}>','192.168.${ra}.${rb}${rc}','08:00:2d:0${ra}:0${rb}:0${rc}',X'${ra}${rb}${rc}');" >> $GENDATA
    if [ ${i} -ge ${numrows} ]; then
      break;
    else
      i=$((${i} +1))
      working=`expr $i % $trippoint`
      if [ $working -eq 0 ]; then
        j=`expr $j + 1`
        percent=`expr $j \* 5`
        status "$percent %"
      fi 
    fi
  done
  status "done"
}

do_initdata()
{
  originnode=${ORIGINNODE:-"1"}
  SCRIPT=${mktmp}/slonik.script
  eval db=\$DB${originnode}
  eval host=\$HOST${originnode}
  eval user=\$USER${originnode}
  eval port=\$PORT${originnode}
  generate_initdata
  launch_poll
  status "loading data"
  $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log
  if [ $? -ne 0 ]; then
    warn 3 "do_initdata failed, see $mktmp/initdata.log for details"
  fi 
  status "data load complete - nodes are seeded reasonably"

  status "purge archive log files up to present in order to eliminate those that cannot be used"
  for file in `/usr/bin/find ${mktmp}/archive_logs_2 -name "slony1_log_*.sql" -type f`; do
    status "purge ${file}"
    rm ${file}
  done
  sleep 5
  status "pull log shipping dump" 
  PGHOST=${HOST2} PGPORT=${PORT2} PGUSER=${USER2} ${SLTOOLDIR}/slony1_dump.sh ${DB2} ${CLUSTER1} > ${mktmp}/logship_dump.sql
  status "load schema for replicated tables into node #3"
  ${PGBINDIR3}/psql -h ${HOST3} -p ${PORT3} -U ${USER3} -d ${DB3} -f ${testname}/init_schema.sql
  status "load log shipping dump into node #3"
  ${PGBINDIR3}/psql -h ${HOST3} -p ${PORT3} -U ${USER3} -d ${DB3} -f ${mktmp}/logship_dump.sql
  
  status "generate more data to test log shipping"
  generate_initdata
  launch_poll
  status "loading data"
  $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/moredata.log 2> $mktmp/moredata.log
  if [ $? -ne 0 ]; then
    warn 3 "loading data failed, see $mktmp/moredata.log for details"
  fi
  wait_for_catchup

  status "execute DDL script"
  init_preamble
  sh ${testname}/exec_ddl.sh ${testname} >> $SCRIPT
  do_ik
  status "completed DDL script"

  status "Generate some more data"
  generate_initdata
  eval db=\$DB${originnode}
  status "loading extra data to node $db"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> ${mktmp}/even_more_data.log 2> ${mktmp}/even_more_data.log2

  wait_for_catchup

  status "move set to node 4"

  init_preamble
  sh ${testname}/moveset.sh ${testname} >> $SCRIPT
  do_ik

  status "origin moved"

  generate_initdata
  eval db=\$DB4
  status "loading extra data to node $db"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> ${mktmp}/even_more_data.log 2> ${mktmp}/even_more_data.log2

  wait_for_catchup


  status "final data load complete - now load files into log shipped node"
  for logfile in `/usr/bin/find ${mktmp}/archive_logs_2 -name "slony1_log_*.sql" -type f | sort`; do
    $pgbindir/psql -h ${HOST3} -p ${PORT3} -d ${DB3} -U ${USER3} -f ${logfile} >> $mktmp/logshipping_output.log 2>> $mktmp/logshipping_errors.log
    status "load file ${logfile} - ${?}"
  done
  status "done"
}
