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
  status "generating products"
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
	  pn=$(random_number 1 10000)
	  price=$(random_number 5 100)
	  echo "select mkproduct('p${pn}${i}', ${price});" >> ${GENDATA}
	  cn=$(random_number 1 10000)
	  echo "select mkcustomer('c${cn}${i}');" >> ${GENDATA}
  done

  status "generating orders"
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
	  echo "drop table if exists t_order;" >> ${GENDATA}
	  echo "select mkorder (cname) onum into temp table t_order from (select cname from customers order by random() limit 1) as foo;" >> ${GENDATA}
	  echo "select * from t_order;" >> ${GENDATA}
	  numprods=$(random_number 3 10)
	  echo "select order_line (onum, pname, (random()*15+3)::integer) from t_order, (select pname from products order by random() limit  ${numprods}) as foo;" >> ${GENDATA}
  done
  status "done"
}

do_initdata()
{
  originnode=${ORIGINNODE:-"1"}
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
  status "first load complete"

  $pgbindir/psql -h $host -p $port -d $db -U $user -c "truncate line_items;" 1> $mktmp/truncate1.log.1 2> $mktmp/truncate1.log
  status "simple truncate"
  
  generate_initdata
  $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log
  
  $pgbindir/psql -h $host -p $port -d $db -U $user -c "truncate orders cascade;" 1> $mktmp/truncate1.log.1 2> $mktmp/truncate1.log
  status "truncate orders (cascades to line_items);"

  generate_initdata
  $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log

  $pgbindir/psql -h $host -p $port -d $db -U $user -c "truncate customers, orders, line_items;" 1> $mktmp/truncate1.log.1 2> $mktmp/truncate1.log
  status "truncate customers, orders, line_items"  

  generate_initdata
  $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log

  SCRIPT=${mktmp}/slonik.script
  status "ran order processing tests, now drop table_237 from replication"
  init_preamble
  echo "set drop table (origin=1, id=237);" >> $SCRIPT
  do_ik

  wait_for_catchup

  status "Dropped table_237 - now truncate it on both nodes"
  $pgbindir/psql -h $HOST1 -p $PORT1 -d $DB1 -U $USER1 -c "truncate public.test_237;"
  $pgbindir/psql -h $HOST2 -p $PORT2 -d $DB2 -U $USER2 -c "truncate public.test_237;"

  status "done"
}
