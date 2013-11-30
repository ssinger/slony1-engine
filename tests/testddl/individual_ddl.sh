testname=$1
tnode=$2
echo "
  EXECUTE SCRIPT (
       FILENAME = '${testname}/ddl_update_part2.sql',
       EVENT NODE = ${tnode},
       EXECUTE ONLY ON = ${tnode}
    );
"
