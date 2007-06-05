testname=$1
echo "
  store trigger (table id=1, trigger name='nonexistant trigger', event node=1);
  store trigger (table id=1, trigger name='another nonexistant trigger', event node=1);
  store trigger (table id=1, trigger name='still another nonexistant trigger', event node=1);
  EXECUTE SCRIPT (
       SET ID = 1,
       FILENAME = '${testname}/ddl_updates.sql',
       EVENT NODE = 1
    );
  store trigger (table id=1, trigger name='late, another nonexistant trigger', event node=1);
  store trigger (table id=1, trigger name='late, still another nonexistant trigger', event node=1);
"
