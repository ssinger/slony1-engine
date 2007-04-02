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
    for set in 1 2 3; do
	txtalen=$(random_number 1 100)
	txta=$(random_string ${txtalen})
	txta=`echo ${txta} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`
	txtblen=$(random_number 1 100)
	txtb=$(random_string ${txtblen})
	txtb=`echo ${txtb} | sed -e "s/\\\\\\\/\\\\\\\\\\\\\\/g" -e "s/'/''/g"`
	echo "INSERT INTO table${set}(data) VALUES ('${txta}');" >> $GENDATA
	echo "INSERT INTO table${set}a(table${set}_id,data) SELECT id, '${txtb}' FROM table${set} WHERE data='${txta}';" >> $GENDATA
    done
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

  status "Move sets to node 3"
  init_preamble
  cat ${testname}/move_sets.ik >> $mktmp/slonik.script
  do_ik
  status "Completed moving sets"
  stop_poll

  ORIGINNODE=3
  originnode=3
  eval db=\$DB${originnode}
  eval host=\$HOST${originnode}
  eval user=\$USER${originnode}
  eval port=\$PORT${originnode}

  status "Generate some more data, switching origin to node 3"
  generate_initdata
  status "loading extra data"
  launch_poll

  $pgbindir/psql -h $host -U $user -d $db < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log
  status "done"
}
