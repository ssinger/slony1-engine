testname=$1
echo "
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${testname}/ddl_updates.sql',
       EVENT NODE = 1
    );
"
