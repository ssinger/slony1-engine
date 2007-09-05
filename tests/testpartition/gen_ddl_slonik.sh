ddlfile=$1
echo "
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${ddlfile}',
       EVENT NODE = 1
    );
"
