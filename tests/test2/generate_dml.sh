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
  status "generating ${numrows} tranactions of random data"
  percent=`expr $j \* 5`
  status "$percent %"
  while : ; do
    txtalen=$(random_number 1 100)
    txta=$(random_string ${txtalen})
    txta=`echo ${txta} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`
    txtblen=$(random_number 1 100)
    txtb=$(random_string ${txtblen})
    txtb=`echo ${txtb} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`

    echo "INSERT INTO table1(data) VALUES ('${txta}');" >> $mktmp/generate.data
    echo "INSERT INTO table2(table1_id,data) SELECT id, '${txtb}' FROM table1 WHERE data='${txta}';" >> $mktmp/generate.data
    echo "INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data ='${txtb}';" >> $mktmp/generate.data
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
  generate_initdata
  launch_poll
  rows=`wc -l ${mktmp}/generate.data | awk '{print($1)}'`
  if [ -f ${testname}/init_failover.ik ]; then
    splitsize=`expr $rows / 3`
    split -l $splitsize ${mktmp}/generate.data ${mktmp}/generate.data.
    status "loading ${splitsize} rows of data"
    $pgbindir/psql -h $host -d $db -U $user < $mktmp/generate.data.aa 1> $LOG 2> $LOG
    if [ $? -ne 0 ]; then
      warn 3 "do_initdata failed, see $LOG for details"
    fi
echo "move set from node 1 to node 2"
    init_preamble
    cat ${testname}/init_moveset.ik  >> $SCRIPT
    do_ik
echo "fail over from node 2 to node 3"    
    init_preamble
    cat ${testname}/init_failover.ik  >> $SCRIPT
    do_ik
echo "failover ik done"
echo "destroy node 1"
    $pgbindir/dropdb -h $host slonyregress1
    sleep 1
echo "stop polling/slons"
    stop_poll
    stop_slons
    sleep 3
echo "restart slons"
    launch_slon
    launch_poll
    status "loading second set of ${splitsize} rows of data"
    $pgbindir/psql -h $host -d $db -U $user < $mktmp/generate.data.ab 1>> $LOG 2> $LOG
    if [ $? -ne 0 ]; then
      warn 3 "do_initdata failed, see $LOG for details"
    fi
echo "drop node 2"    
    . ${testname}/init_dropnode.sh > $SCRIPT
    cp $SCRIPT $mktmp/dropnode.slonik
    do_ik
    status "loading remaining rows of data"
    $pgbindir/psql -h $host -d $db -U $user < $mktmp/generate.data.ac 1>> $LOG 2> $LOG
    if [ $? -ne 0 ]; then
      warn 3 "do_initdata failed, see $LOG for details"
    fi
  else    
    status "loading ${rows} rows of data"
    $pgbindir/psql -h $host -d $db -U $user < $mktmp/generate.data 1> $LOG 2> $LOG
    if [ $? -ne 0 ]; then
      warn 3 "do_initdata failed, see $LOG for details"
    fi
  fi 
  status "done"
}
