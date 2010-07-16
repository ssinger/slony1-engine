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

more_data ()
{
  GENDATA="$mktmp/generate.data"
  #for year in 2006 2007 2008 2009 2010 2011 2012; do
  for year in 2006 ; do
     #for month in 1 2 3 4 5 6 7 8 9 10 11 12; do
     for month in 1 2 3 ; do
	 echo "" > ${GENDATA}
         numrows=$(random_number 50 75)
         status "Generating ${numrows} transactions of random data for ${year}/${month}"
	 i=0
	 while : ; do
	   if [ ${i} -ge ${numrows} ]; then
	       break;
	   else
	       i=$((${i} +1))
	   fi
	   quantity=$(random_number 1 9)
	   day=$(random_number 1 25)   # Peculiar company that closes up the last few days of the month
	   hour=$(random_number 8 19)  # sells during "human" hours of the day in London
	   minute=$(random_number 1 59)
	   echo "select purchase_product (region_code, product_id, (${quantity}+random()*3)::integer, '${year}-${month}-${day} ${hour}:${minute} GMT'::timestamptz) from regions, products order by random() limit 3;" >> ${GENDATA}
         done
	 size=`wc -l ${GENDATA}`
	 status "got data for ${year}/${month} - ${size} items"
	 status "Generate DDL script for new set for ${year}/${month}"
	 DDLSQL=$mktmp/ddl_script_${year}_${month}.sql
	 SCRIPT=$mktmp/slonik.script
	 init_preamble
	 sh ${testname}/gen_ddl_sql.sh ${year} ${month} ${CLUSTER1} > ${DDLSQL}
	 sh ${testname}/gen_ddl_slonik.sh ${DDLSQL} ${year} ${month} >> ${SCRIPT}
	 do_ik
	 status "Added new partition for ${year}/${month}, merged into set 1"
	 status "Load data for ${year}/${month}"
	 eval db=\$DB${originnode}
	 $pgbindir/psql -h $host -p $port -d $db -U $user < ${GENDATA} 1>> $mktmp/loaddata_${year}_${month}.log 2>> $mktmp/loaddata_${year}_${month}.log
      done
  done
  wait_for_catchup
  status "done"
}


do_initdata()
{
  originnode=${ORIGINNODE:-"1"}
  eval db=\$DB${originnode}
  eval host=\$HOST${originnode}
  eval user=\$USER${originnode}
  eval port=\$PORT${originnode}
  # generate_initdata  - # No initial data!
  launch_poll
  status "loading data"
  #$pgbindir/psql -h $host -p $port -d $db -U $user < $mktmp/generate.data 1> $mktmp/initdata.log 2> $mktmp/initdata.log
  if [ $? -ne 0 ]; then
    warn 3 "do_initdata failed, see $mktmp/initdata.log for details"
  fi 
  wait_for_catchup
  status "done"

  more_data
}
