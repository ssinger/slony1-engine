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
  numrows=$(random_number 125 150)
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
    echo "INSERT INTO table1(data) VALUES ('${txta} - Gross C format string: %d%05d%s%s%f%l%-72.52LG');" >> ${GENDATA}
    echo "INSERT INTO table2(table1_id,data) SELECT id, '${txtb}' FROM table1 WHERE data='${txta}';" >> ${GENDATA}
    echo "INSERT INTO table3(id2) SELECT id FROM table2 WHERE data ='${txtb}';" >> ${GENDATA}
    echo "INSERT INTO table4(numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values ('${ra}${rb}.${rc}','${ra}.${rb}${rc}','(${ra},${rb})','((${ra},${ra}),(${rb},${rb}),(${rc},${rc}),(${ra},${rc}))','((${ra},${rb}),(${rc},${ra}),(${rb},${rc}),(${rc},${rb}))','<(${ra},${rb}),${rc}>','192.168.${ra}.${rb}${rc}','08:00:2d:0${ra}:0${rb}:0${rc}',X'${ra}${rb}${rc}');" >> ${GENDATA}
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
  status "pull log shipping dump"
  PGHOST=${HOST2} PGPORT=${PORT2} PGUSER=${USER2} ${SLTOOLDIR}/slony1_dump.sh ${DB2} ${CLUSTER1} > ${mktmp}/logship_dump.sql

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

  status "move set to node 3"

  init_preamble
  sh ${testname}/moveset.sh ${testname} >> $SCRIPT
  do_ik

  status "origin moved"

  generate_initdata
  status "loading extra data to node 3"
  $pgbindir/psql -h $HOST3 -p $PORT3 -U $USER3 -d $DB3 < $mktmp/generate.data 1> ${mktmp}/even_more_data.log 2> ${mktmp}/even_more_data.log2

  wait_for_catchup
  status "done"

  status "final data load complete - now load files into log shipped node"
  status "set up database for log shipped node"
  ${PGBINDIR4}/createdb -p ${PORT4} -U ${USER4} ${DB4}
  ${PGBINDIR4}/createlang plpgsql ${DB4}

  status "load schema for replicated tables into node #4"
  ${PGBINDIR4}/psql -h ${HOST4} -p ${PORT4} -U ${USER4} -d ${DB4} -f ${testname}/init_schema.sql
  status "load log shipping dump into node #4"
  ${PGBINDIR4}/psql -h ${HOST4} -p ${PORT4} -U ${USER4} -d ${DB4} -f ${mktmp}/logship_dump.sql


  firstseq=`psql -At -d ${DB4} -p ${PORT4} -c 'select at_counter from _slony_regress1.sl_archive_tracking ;'`
  lastseq=`(cd ${mktmp}/archive_logs_2; /usr/bin/find -name "*.sql") | cut -d "_" -f 4 | cut -d "." -f 1 | sort -n | tail -1`
  status "Logs numbered from ${firstseq} to ${lastseq}"
  currseq=${firstseq}
  while : ; do
    cs=`printf "%020d" ${currseq}`
    status "current sequence value: ${cs}"
    firstseq=`psql -At -d ${DB4} -p ${PORT4} -c 'select at_counter from _slony_regress1.sl_archive_tracking ;'`
    status "archive tracking ID: ${firstseq}"
    for logfile in `/usr/bin/find ${mktmp}/archive_logs_2 -name "slony1_log_*_${cs}.sql" -type f`; do
      ${PGBINDIR4}/psql -h ${HOST4} -p ${PORT4} -d ${DB4} -U ${USER4} -f ${logfile} >> $mktmp/logshipping_output.log 2>> $mktmp/logshipping_errors.log
      status "load file ${logfile} - ${?}"
    done
    if [ ${currseq} -gt ${lastseq} ]; then
      break;
    else
      currseq=`expr ${currseq} + 1`
    fi
  done
  status "done loading data into log shipping node"

  NUMNODES=4
  status "Changed number of nodes to 4 to reflect the log shipping node"

}
