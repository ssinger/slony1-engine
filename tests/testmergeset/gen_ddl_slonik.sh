ddlfile=$1
year=$2
month=$3
tableid=`printf "%04d%02d" ${year} ${month}`
echo "
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${ddlfile}',
       EVENT NODE = 1
    );
  sync(id=1);
  wait for event (origin=1, confirmed=all, wait on=1);

  create set (id=999, origin = 1, comment='Temporary set for new partition');

  set add table (set id=999, origin=1, id=${tableid}, fully qualified name='public.sales_txns_${year}_${month}');
  subscribe set (id=999, provider=1, receiver=2, forward = yes);
  sync (id=2);
  wait for event (origin=2, confirmed=1, wait on=2);
  sync (id=1);
  wait for event (origin=1, confirmed=2, wait on=2);
  subscribe set (id=999, provider=1, receiver=3, forward = no);
  sync (id=3);
  wait for event (origin=3, confirmed=1, wait on=3);
  sync (id=1);
  wait for event (origin=1, confirmed=3, wait on=3);
  subscribe set (id=999, provider=2, receiver=4, forward = no);
  sync (id=4);
  wait for event (origin=4, confirmed=1, wait on=4);
  sync (id=1);
  wait for event (origin=1, confirmed=all, wait on=1);
  sleep(seconds=4);
  merge set (ID = 1, ADD ID = 999, ORIGIN = 1 );

" 
