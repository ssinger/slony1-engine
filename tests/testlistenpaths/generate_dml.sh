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
  numrows=$(random_number 150 200)
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
    echo "INSERT INTO table1(id,data) VALUES (${i},'${txta}');" >> $GENDATA
    echo "INSERT INTO table2(table1_id,data) SELECT id, '${txtb}' FROM table1 WHERE data='${txta}';" >> $GENDATA
    echo "INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data ='${txtb}';" >> $GENDATA
    echo "INSERT INTO table4(id,numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values (${i},'${ra}${rb}.${rc}','${ra}.${rb}${rc}','(${ra},${rb})','((${ra},${ra}),(${rb},${rb}),(${rc},${rc}),(${ra},${rc}))','((${ra},${rb}),(${rc},${ra}),(${rb},${rc}),(${rc},${rb}))','<(${ra},${rb}),${rc}>','192.168.${ra}.${rb}${rc}','08:00:2d:0${ra}:0${rb}:0${rc}',X'${ra}${rb}${rc}');" >> $GENDATA
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
  generate_initdata
  launch_poll
  for org in 1 2; do
      eval db=\$DB${org}
      eval host=\$HOST${org}
      eval user=\$USER${org}
      eval port=\$PORT${org}

      status "empty out schemas..."
      for tbl in table1 table2 table3 table4; do
	  status "delete from table ${tbl}"
	  $pgbindir/psql -h $host -p $port -d $db -U $user -c "delete from ${tbl};"
      done
      status "loading data into node ${org}"
      $pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log
      if [ $? -ne 0 ]; then
	  warn 3 "do_initdata failed, see $mktmp/initdata.log for details"
      fi 
      status "data load complete for node ${org}"
  done
}
