var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testutf8/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testutf8/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';
			

}


function init_tables() {
	var script=	"set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');\n"
		+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');\n"
		+"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3');\n"
		+"set add table (id=4, set id=1, origin=1, fully qualified name = 'public.utf8table', comment='Test table for multibyte/UTF8');\n";

	return script;
}
	


function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';


}

function subscribe_set() {
	var script=	"subscribe set (id = 1, provider = 1, receiver = 2, forward = no);\n";

	return script;
}



function generate_data_file(coordinator) {
	var num_rows = random_number(50,1000);
	var file = java.io.File.createTempFile("testdata",".sql");
	file.deleteOnExit();
	var fileWriter = new java.io.FileWriter(file);
	for(var idx=0; idx < num_rows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		//txta = txta.replace("'","''");
		var txtblen = random_number(1,100);
		var txtb = random_string(textlen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		fileWriter.write( "INSERT INTO table1(data) VALUES (E'"+txta+"');");
		fileWriter.write("INSERT INTO table2(table1_id,data) SELECT id, E'"+txtb+"' FROM table1 WHERE data=E'"+txta+"';");
		fileWriter.write("INSERT INTO table3(table2_id) SELECT id FROM table2 WHERE data =E'"+txtb+"';");
		fileWriter.write("INSERT INTO utf8table (string) values (E'"+txtb+" - \303\241');");
		fileWriter.write( "INSERT INTO utf8table (string) values (E'"+txtb+" -- \303\241');");
		fileWriter.write("INSERT INTO utf8table (string) values (E'\303\241 - "+txtb+"');");
		fileWriter.write("INSERT INTO utf8table (string) values (E'\303\241 -- "+txtb+"');");
		fileWriter.write("INSERT INTO utf8table (string) values (E't3 -- \303\241 - "+txtb+"');");
		fileWriter.write("INSERT INTO utf8table (string) values (E't3 - \303\241 -- "+txtb+"');");
		fileWriter.write( "INSERT INTO utf8table (string) values(repeat('test1234',4000)||'bar');");	
			
	}
	fileWriter.close();
	return file;
	
	
	
}

function do_test(coordinator) {
	
	var sqlScript='';
	//Compare the initial states.
	//They should match since this is an omit_copy test.
	do_compare(coordinator);
	var file = generate_data_file(coordinator);
	
	psql = coordinator.createPsqlCommand('db1',file);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);
	coordinator.log("done");
}

function get_compare_queries() {
	

	var queries=["SELECT id,data FROM table1 ORDER BY id"	             
	             ,"SELECT id,table1_id,data FROM table2 ORDER BY id" 	             
	             ,"SELECT id,table2_id,mod_date, data FROM table3 ORDER BY id" 
	             ,"SELECT id, string from utf8table order by id"

	            ];

	return queries;
}

run_test(coordinator,'testutf8');
