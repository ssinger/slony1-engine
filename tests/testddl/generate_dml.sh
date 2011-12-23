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
  numrows=$(random_number 150 350)
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
    echo "INSERT INTO table1(data) VALUES ('${txta}');" >> $GENDATA
    echo "INSERT INTO table2(table1_id,data) (SELECT id, '${txtb}' FROM table1 WHERE data='${txta}' order by id desc limit 3);" >> $GENDATA
    echo "INSERT INTO table3(table2_id) (SELECT id FROM table2 WHERE data ='${txtb}' order by id desc limit 3);" >> $GENDATA
    echo "INSERT INTO table4(data) VALUES ('${txtb}');" >> $GENDATA
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
  LOG=${mktmp}/initdata.log
  SCRIPT=${mktmp}/slonik.script
  originnode=${ORIGINNODE:-"1"}
  eval db=\$DB${originnode}
  eval host=\$HOST${originnode}
  eval user=\$USER${originnode}
  eval port=\$PORT${originnode}
  generate_initdata
  launch_poll
  status "loading data"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> $LOG 2> $LOG
  if [ $? -ne 0 ]; then
    warn 3 "do_initdata failed, see $mktmp/initdata.log for details"
  fi

  status "execute DDL script"
  init_preamble
  sh ${testname}/exec_ddl.sh ${testname} >> $SCRIPT
  do_ik
  status "completed DDL script"

  status "Generate some more data"
  generate_initdata
  eval db=\$DB${originnode}
  status "loading extra data to node $db"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> $LOG 2> $LOG
  wait_for_catchup

  status "Execute a script on each node, one by one"
  for tnode in 1 2 3; do
      init_preamble
      sh ${testname}/individual_ddl.sh ${testname} ${tnode} >> ${SCRIPT}
      status "execute DDL script on node ${tnode}"
      do_ik
      status "DDL script completed on node ${tnode}"
  done
  wait_for_catchup

  status "Generate still more data"
  generate_initdata
  eval db=\$DB${originnode}
  status "loading extra data to node $db"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> $LOG 2> $LOG
  wait_for_catchup

  status "done"
}
