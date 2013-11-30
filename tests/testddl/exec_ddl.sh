testname=$1
echo "
  EXECUTE SCRIPT (
       FILENAME = '${testname}/ddl_updates.sql',
       EVENT NODE = 1
    );

  try {
       execute script (
            filename='${testname}/bad_ddl.sql',
            event node=1
       );
  } on error {
     echo 'a bad DDL script gets rolled back on the master';
  } on success {
     echo 'a bad DDL script did not get rolled back - ERROR!';
     exit 255;
  }
"
