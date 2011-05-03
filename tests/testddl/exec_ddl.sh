testname=$1
echo "
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${testname}/ddl_updates.sql',
       EVENT NODE = 1
    );

  try {
       execute script (
            set id=1,
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
