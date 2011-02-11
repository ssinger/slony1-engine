var NUM_NODES=3;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testmultiplemoves/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testmultiplemoves/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';

				


}




function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='a table');\n"
	+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table1a', comment='a table');\n"
	+"set add table (id=3, set id=2, origin=1, fully qualified name = 'public.table2', comment='a table');\n"
	+"set add table (id=4, set id=2, origin=1, fully qualified name = 'public.table2a', comment='a table');";
	return script;
}
	


function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n'
		+"create set (id=2, origin=1, comment='set number 2');";

}

function subscribe_set() {
	var script=	"subscribe set (id=1, provider=1, receiver = 2, forward=yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=all, confirmed=2, wait on=1);\n" 				
		+"subscribe set (id=2, provider=1, receiver = 2, forward=yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=all, confirmed=2, wait on=1);\n"
		+"subscribe set (id=1, provider=2, receiver = 3, forward=yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=all, confirmed=3, wait on=1);\n"
		+"subscribe set (id=2, provider=2, receiver = 3, forward=yes);\n"
		+"sync(id=1);\n"				
		+"wait for event (origin=all, confirmed=3, wait on=1);\n"

	return script;
}

function move_set(coordinator) {
	
	var slonikScript="lock set (id=1, origin=1);\n"
		+"move set (id=1, old origin=1, new origin=3);\n"
		+"lock set (id=2, origin=1);\n"	
		+"move set (id=2, old origin=1, new origin=3);\n";
	var preamble = get_slonik_preamble();
	run_slonik('move set',coordinator,preamble,slonikScript);
}


function generate_data_file(coordinator) {
	var num_rows = random_number(50,1000);
	var sqlScript='';
	var file = java.io.File.createTempFile("testdata",".sql");
	file.deleteOnExit();
	var fileWriter = new java.io.FileWriter(file);
	for(var idx=0; idx < num_rows; idx++) {
		for(var setid=1; setid <3; setid++) {
			var txtalen=random_number(1,100);
			var txta=random_string(txtalen);
			txta = new java.lang.String(txta).replace("\\","\\\\");
			var txtblen = random_number(1,100);
			var txtb = random_string(txtblen);
			txtb = new java.lang.String(txtb).replace("\\","\\\\");
			fileWriter.write("INSERT INTO table" + setid + "(data) VALUES (E'" + txta+"');\n");
			fileWriter.write("INSERT INTO table"+setid+"a (table"+setid+"_id,data) SELECT id,E'" + txtb+"' FROM "
					+" table"+setid+" WHERE data=E'" + txta + "';\n");
			
		}
			
	}
	fileWriter.close();
	return file;
	
	
}

function do_test(coordinator) {
	
	var sqlScript='';
	
	var file = generate_data_file(coordinator);
	
	psql = coordinator.createPsqlCommand('db1',file);
	psql.run();
	coordinator.join(psql);
	//Move set. 
	//We do it before the sync since it is more interesting.
	coordinator.log("Moving sets");
	move_set(coordinator);
	//wait_for_sync(coordinator);
	file = generate_data_file(coordinator);
	psql = coordinator.createPsqlCommand('db3',file);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);
	coordinator.log("done");
}

function get_compare_queries() {
	

	var queries=["select 'table1', id, data from table1 order by id"
	             ,"select 'table1a', id, table1_id, data from table1a order by id"
	             ,"select 'table2', id, data from table2 order by id"
	             ,"select 'table2a', id, table2_id, data from table2a order by id"
	            ];

	return queries;
}

run_test(coordinator,'testmultiplemoves');
