
var NUM_NODES=3;
 
coordinator.includeFile('regression/common_tests.js');


function get_schema() {
	var sqlScript = coordinator.readFile('regression/testddl/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testddl/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');';
}

function init_tables() {
	var script =
	"\nset add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');\n"
	+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');\n"
	+"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3');\n"
	+"set add table (id=4, set id=1, origin=1, fully qualified name = 'public.table4');\n"
	+"set add table (id=5, set id=1, origin=1, fully qualified name = 'public.table5');\n"
	+"set add table (id=6, set id=1, origin=1, fully qualified name = 'public.billing_discount');\n"
	+ "set add sequence(id=1,set id=1, origin=1, fully qualified name= 'public.table1_id_seq');\n"
	+ "set add sequence(id=2,set id=1, origin=1, fully qualified name= 'public.table5_id_seq');\n"
	
	
	return script;
}

function create_set() {
	return 'create set (id=1, origin=1);  # no comment - should draw one in automagically\n';

}

function subscribe_set() {
	var script=
	"echo 'sleep a couple seconds';"
	+"sleep (seconds = 2);\n"
	+"subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);\n"
	+"sync(id=1);\n"
	+"wait for event (origin=1, confirmed=2, wait on=1);\n"
	+"echo 'sleep a couple seconds';\n"
	+"sleep (seconds = 2);\n"
	+"subscribe set ( id = 1, provider = 1, receiver = 3, forward = no);\n";
	return script;
}


function generate_data() {
	var numrows = random_number(150,350);
	var sqlScript='';	
	for(var row = 0; row < numrows; row++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		//txta = txta.replace("'","''");
		textlen = random_number(1,100);
		var txtb = random_string(textlen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		//txta = txtb.replace("'","''");
		sqlScript += "INSERT INTO table1(data) VALUES (quote_literal(E'"+txta+"'));\n";
		sqlScript += "INSERT INTO table2(table1_id,data) SELECT id,quote_literal(E'"+txtb+"') FROM table2 WHERE data=quote_literal(E'" + txta+"');\n";
		sqlScript += "INSERT INTO table3(table2_id) SELECT id FROM table2 where data=quote_literal(E'" + txtb + "');\n";
		sqlScript += "INSERT INTO table4(data) VALUES (quote_literal(E'" + txtb + "'));\n";
		
	}
	//java.lang.System.out.println(sqlScript);
	return sqlScript;
}

function generate_table5_transaction() {
	var sqlScript='';	

	// This transaction inserts rows into table5, then does
	// DDL as it is supposed to be done by an application, and
	// continues to insert. The test ensures that the apply
	// trigger is flushing out the apply query cache to avoid
	// using incompatible SPI plans.
	sqlScript += "START TRANSACTION;\n";
	sqlScript += "INSERT INTO table5(data) VALUES (4);\n";
	sqlScript += "INSERT INTO table5(data) VALUES (5);\n";
	sqlScript += "SELECT _slonyregress.ddlcapture('ALTER TABLE public.table5 ALTER COLUMN data TYPE text USING data::text;', NULL);\n";
	sqlScript += "SELECT _slonyregress.ddlscript_complete(NULL);\n";
	sqlScript += "INSERT INTO table5(data) VALUES ('six');\n";
	sqlScript += "INSERT INTO table5(data) VALUES ('seven');\n";
	sqlScript += "COMMIT;\n";
		
	java.lang.System.out.println(sqlScript);
	return sqlScript;
}

function exec_ddl(coordinator) {
	preamble = get_slonik_preamble();
	var slonikScript = 'EXECUTE SCRIPT( FILENAME=\'regression/testddl/ddl_updates.sql\''
		+',EVENT NODE=1);\n';
	var slonikScript2='try {\n '
		+ 'execute script(set id=1, FILENAME=\'regression/testddl/bad_ddl.sql\''
		+', event node=1);\n'
		+ '}\n'
		+ 'on error{ \n'
		+ 'echo \'a bad DDL script gets rolled back on the master\';\n'
		+ '}\n'
		+' on success {\n' 
		+ 'echo \'a bad DDL script did not get rolled back - ERROR!\';\n'
		+ 'exit -1;\n'
		+ '}\n';
	run_slonik('update ddl',coordinator,preamble,slonikScript);
}


function individual_ddl(coordinator, nodenum) {
	preamble = get_slonik_preamble();
	slonikScript = 'EXECUTE SCRIPT( FILENAME=\'regression/testddl/ddl_update_part2.sql\''
		+ ' ,EVENT NODE=' + nodenum + ' ,EXECUTE ONLY ON = ' + nodenum +');';
	run_slonik('update ddl',coordinator,preamble,slonikScript);
}

function trigger_function(coordinator) {
	/**
	 * We stop the slons because we want to make sure that a SYNC does not
	 * happen in between the EXECUTE_SCRIPT and the next SYNC.
	 */
	terminate_slon(coordinator);
	var sql = '';
	for(var idx=0; idx < 1000; idx++) {
		sql = sql + "insert into table5(data) values ('seqtest');\n"
	}
	var psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	preamble = get_slonik_preamble();
	slonikScript = "EXECUTE SCRIPT(  SQL='alter table table1 drop column seqed;create trigger table5_trigger "
		+ " before INSERT on public.table5 for each row execute procedure "
		+ " insert_table1();'"
		+ ' ,EVENT NODE=1 );';
	run_slonik('add trigger ddl',coordinator,preamble,slonikScript);
	slonikScript = "EXECUTE SCRIPT(  SQL='insert into table5(data) values (9);'"
		+ ' ,EVENT NODE=1);';
	run_slonik('add trigger ddl',coordinator,preamble,slonikScript);

	launch_slon(coordinator);	
	
}


function inline_ddl(coordinator) {
	preamble = get_slonik_preamble();
	slonikScript = 'EXECUTE SCRIPT('
	    + 'SQL=\'ALTER TABLE table1 ADD COLUMN processed timestamp with time zone;\''
		+ ' ,EVENT NODE=1);\n';
	slonikScript += 'EXECUTE SCRIPT('
	    + 'SQL=\'UPDATE table1 SET processed = CURRENT_TIMESTAMP;\''
		+ ' ,EVENT NODE=1);\n';
	run_slonik('update ddl',coordinator,preamble,slonikScript);
}


function execute_on_subscriber(coordinator)
{
    /**
     * Perform an EXECUTE SCRIPT with a subscriber (a non-provider)
     * and verify that the DDL changes replicate to all other nodes.
     */
    var preamble = get_slonik_preamble();
    var slonikScript = 'EXECUTE SCRIPT('
	+ 'SQL=\'CREATE TABLE test_on_subscriber(id serial, value text);\''
	+ ' ,EVENT NODE=3);\n'
        + 'sync(id=3);\n wait for event(origin=3, confirmed=all, wait on=3);\n';
    
    run_slonik('execute_on_subscriber.create',coordinator,preamble,slonikScript);
    /**
     * Now drop the table on nodes 1 and 2. 
     * That will fail if the CREATE did not propogate
     */
    slonikScript = 'EXECUTE SCRIPT('
	+ 'SQL=\'DROP TABLE test_on_subscriber;\''
	+ ' ,EVENT NODE=2, EXECUTE ONLY ON=2);\n' 
	+ 'EXECUTE SCRIPT('
	+ 'SQL=\'DROP TABLE test_on_subscriber;\''
	+ ' ,EVENT NODE=1, EXECUTE ONLY ON=1);\n' 
            
    run_slonik('execute_on_subscriber.drop',coordinator,preamble,slonikScript);
    
}


function execute_only_on_list(coordinator)
{
    /**
     * Perform an EXECUTE SCRIPT with a subscriber (a non-provider)
     * and verify that the DDL changes replicate to all other nodes.
     */
    var preamble = get_slonik_preamble();
    var slonikScript = 'EXECUTE SCRIPT('
	+ 'SQL=\'CREATE TABLE test_only_on(id serial, value text);\''
	+ ' ,EVENT NODE=3,execute only on=\'1,2,3\');\n'
        + 'sync(id=3);\n wait for event(origin=3, confirmed=all, wait on=3);\n';
    
    run_slonik('execute_on_subscriber.create',coordinator,preamble,slonikScript);
    /**
     * Now drop the table on nodes 1 and 2. 
     * That will fail if the CREATE did not propogate
     */
    slonikScript = 'EXECUTE SCRIPT('
	+ 'SQL=\'DROP TABLE test_only_on;\''
	+ ' ,EVENT NODE=2, EXECUTE ONLY ON=2);\n' 
	+ 'EXECUTE SCRIPT('
	+ 'SQL=\'DROP TABLE test_only_on;\''
	+ ' ,EVENT NODE=1, EXECUTE ONLY ON=1);\n' 
	+ 'EXECUTE SCRIPT('
	+ 'SQL=\'DROP TABLE test_only_on;\''
	+ ' ,EVENT NODE=3, EXECUTE ONLY ON=3);\n' ;
            
    run_slonik('execute_on_subscriber.drop',coordinator,preamble,slonikScript);
    
}

function do_test(coordinator) {

	var numrows = random_number(150,350);
	var trippoint = numrows / 20;
	var percent=0;
	
	var sqllScript='';
	sql = generate_data();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	exec_ddl(coordinator);
	wait_for_sync(coordinator);
	for(var idx = 1; idx <= NUM_NODES;idx++) {
		individual_ddl(coordinator,idx);
	}
	wait_for_sync(coordinator);

	sql = generate_data();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);

	sql = generate_table5_transaction();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);

	inline_ddl(coordinator);
	wait_for_sync(coordinator);


	trigger_function(coordinator);
	wait_for_sync(coordinator);
	execute_on_subscriber(coordinator);
        wait_for_sync(coordinator);
        execute_only_on_list(coordinator);
}

function get_compare_queries() {
	var queries=['SELECT id,data FROM table1 ORDER BY id',
	             'SELECT id,table1_id,data FROM table2 ORDER BY id',
	             'SELECT id,table2_id,mod_date, data FROM table3 ORDER BY id',
	             'select col1,col2,col1::text||col2::text as id, data,newcol from table4 order by id'];
	return queries;
}


run_test(coordinator,'testddl');
