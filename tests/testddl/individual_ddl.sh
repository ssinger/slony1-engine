testname=$1
node=$2
echo "
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${testname}/ddl_update_part2.sql',
       EVENT NODE = 1,
       EXECUTE ONLY ON = ${node}
    );
"
