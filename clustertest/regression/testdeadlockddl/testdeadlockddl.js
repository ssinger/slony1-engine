var NUM_NODES=3;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testdeadlockddl/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testdeadlockddl/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';


}

function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');\n"
	+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');\n"
	+"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3');\n"
	+"set add table (id=4, set id=1, origin=1, fully qualified name = 'public.table4');\n"
	+"set add table (id=5, set id=1, origin=1, fully qualified name = 'public.billing_discount');\n";

	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=2, wait on=1);\n"
		+"echo 'sleep a couple seconds';\n"
		+"sleep (seconds = 2);\n"
		+"subscribe set ( id = 1, provider = 1, receiver = 3, forward = no);\n"
	return script;
}


function generate_data() {
	var sqlScript='';
	
	var numrows=random_number(50,90);
	for(var idx = 0; idx < numrows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		textlen = random_number(1,100);
		var txtb = random_string(textlen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		
	    sqlScript+= "INSERT INTO table1(data) VALUES (E'" + txta + "');\n";
	    sqlScript+= "INSERT INTO table2(table1_id,data) SELECT id,E'"+txtb+"' FROM table1 WHERE data=E'"+txta+"';\n";
	    sqlScript+= "INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data =E'"+txtb+"}';\n" ;
	    sqlScript+= "INSERT INTO table4(data) VALUES (E'"+txtb+"');\n";

	}

	
	return sqlScript;
}


function exec_ddl(coordinator) {
	var slonikScript = "EXECUTE SCRIPT( FILENAME='regression/testdeadlockddl/ddl_updates.sql',EVENT NODE=1);\n";
	var preamble = get_slonik_preamble();
	
	run_slonik('exec ddl',coordinator,preamble,slonikScript);
	
}

var psqlInstances=[];
function start_deadlock_inducing_queries(coordinator) {
	var query = "  begin; "
	    +"select id into temp table foo1 from table1 order by random() limit 5;"
	    +"select id into temp table foo2 from table2 order by random() limit 5;"
	    +"select id into temp table foo3 from table3 order by random() limit 5;"
	    +"lock table4 in access share mode;"
	    +"lock table2 in access share mode;"
	    +"lock table1 in access share mode;"
	    +"lock table3 in access share mode;"
	    +"SELECT pg_sleep(3);"
		+"select * from table1 order by random() limit 5;"
		+"select pg_sleep(1);";
	
		for(var idx = 0; idx < 40; idx++) {
			var waitSeconds = random_number(1,5);
			query +="select * from table1 t1, table2 t2, table3 t3, foo1, foo2, foo3"
				+"    where t1.id = t2.table1_id and t3.table2_id = t2.id and"
				+" foo1.id = t1.id limit 5;\n"
				+" select pg_sleep(" + waitSeconds+");\n";
		}
		for(var idx = 0; idx < 8; idx++) {
			psqlInstance = coordinator.createPsqlCommand('db1',query);
			psqlInstance.run();
			psqlInstances[idx]=psqlInstance;
		}
}

function do_test(coordinator) {

	var numrows = random_number(150,350);
	var trippoint = numrows / 20;
	var percent=0;
	
	var sqllScript='';
	sql = generate_data();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	start_deadlock_inducing_queries(coordinator);
	
	//execute DDL
	exec_ddl(coordinator);
	sql = generate_data();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	
	coordinator.join(psql);
	for(var idx=0; idx < psqlInstances.length; idx++) {
		coordinator.join(psqlInstances[idx]);
	}
	wait_for_sync(coordinator);
	
	
	
}

function get_compare_queries() {
	
	//Note for table4 we are expecting the original column names not the altered
	//ones.  We expect the DDL to fail.
	var queries=['SELECT id,data FROM table1 ORDER BY id'
	             ,'SELECT id,table1_id,data FROM table2 ORDER BY id'
	             ,'SELECT id,table2_id,mod_date, data FROM table3 ORDER BY id'
	             ,'select col1::text||col2::text as id, col1,col2,data from table4 order by id' 
	            ];

	return queries;
}

run_test(coordinator,'testdeadlockddl');
