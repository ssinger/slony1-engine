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
  numrows=$(random_number 50 90)
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
    echo "INSERT INTO table2(table1_id,data) SELECT id, '${txtb}' FROM table1 WHERE data='${txta}';" >> $GENDATA
    echo "INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data ='${txtb}';" >> $GENDATA
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

start_deadlock_inducing_queries
  generate_initdata
  launch_poll
  status "loading data"
  $pgbindir/psql -h $host -p $port -U $user -d $db < $mktmp/generate.data 1> $LOG 2> $LOG
  if [ $? -ne 0 ]; then
    warn 3 "do_initdata failed, see $mktmp/initdata.log for details"
  fi

  start_deadlock_inducing_queries

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

  status "done"
}

start_deadlock_inducing_queries()
{
    status "start deadlock-inducing queries"
    QUERY="
    begin;
    select id into temp table foo1 from table1 order by random() limit 5;
    select id into temp table foo2 from table2 order by random() limit 5;
    select id into temp table foo3 from table3 order by random() limit 5;
    lock table4 in access share mode;
    lock table2 in access share mode;
    lock table1 in access share mode;
    lock table3 in access share mode;
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
\!sh -c 'sleep 3'
	select * from table1 order by random() limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo1.id = t1.id limit 5;
\!sh -c 'sleep 2'

	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo2.id = t2.id limit 5;
\!sh -c 'sleep 1'
	select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3
        where t1.id = t2.table1_id and t3.table2_id = t2.id and
	foo3.id = t3.id limit 5;
\!sh -c 'sleep 2'
"

    echo $QUERY

    for i in 1 2 3 4 5 6 7 8; do
	status "start query thread ${i}"
	(echo "${QUERY}"  | $pgbindir/psql -h $HOST2 -p $PORT2 -U $USER2 -d $DB2; status "end query thread ${i}")&
	sleep 1
    done
    status "all deadlock-inducing queries running"
}