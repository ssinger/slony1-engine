var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testschemanames/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testschemanames/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';

				


}




function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'foo.table1', comment='accounts table');\n"
	+"set add table (id=2, set id=1, origin=1, fully qualified name = 'foo.table2', key='table2_id_key');\n"
	+"set add table (id=3, set id=1, origin=1, fully qualified name = 'foo.table3');\n"
	+"set add table (set id = 1, origin = 1, id = 6, fully qualified name ="
	+"'\"Schema.name\".user', comment = 'Table with evil name - user, and a field called user');\n"
	+"set add table (set id = 1, origin = 1, id = 7, fully qualified name ="
	+"'\"Schema.name\".\"Capital Idea\"', comment = 'Table with spaces in its name, caps, and a user field as PK');\n"
	+"set add sequence (set id = 1, origin = 1, id = 2, fully qualified name = '\"Studly Spacey Schema\".\"user\"');\n"
	+"set add sequence (set id = 1, origin = 1, id = 3, fully qualified name = '\"Schema.name\".\"a.periodic.sequence\"');\n"


	return script;
}
	


function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';


}

function subscribe_set() {
	var script=	"subscribe set (id = 1, provider = 1, receiver = 2, forward = no);\n";

	return script;
}



function generate_data(coordinator) {
	var num_rows = random_number(50,1000);
	var sqlScript='';
	for(var idx=0; idx < num_rows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		//txta = txta.replace("'","''");
		var txtblen = random_number(1,100);
		var txtb = random_string(textlen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		sqlScript += "INSERT INTO foo.table1(data) VALUES (E'"+txta+"');";
	    sqlScript+= "INSERT INTO foo.table2(table1_id,data) SELECT id, E'"+txtb+"' FROM foo.table1 WHERE data=E'"+txta+"';\n";
	    sqlScript+= "INSERT INTO foo.table3(table2_id) SELECT id FROM foo.table2 WHERE data =E'"+txtb+"';\n";
	    sqlScript+= "INSERT INTO \"Schema.name\".\"Capital Idea\" (\"user\", description) values (E'"+txta+"', E'"+txtb+"');\n";
	    sqlScript+= "INSERT INTO \"Schema.name\".\"user\" (\"user\", id) values (E'"+txtb+"', "+txtblen+");\n";
	    sqlScript+= "select nextval('\"Schema.name\".\"a.periodic.sequence\"');";
	    sqlScript+= "select nextval('\"Studly Spacey Schema\".\"user\"');";
		
			
	}
	return sqlScript;
	
	
	
}

function do_test(coordinator) {
	
	var sqlScript='';
	//Compare the initial states.
	//They should match since this is an omit_copy test.
	do_compare(coordinator);
	var sql = generate_data(coordinator);
	
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);
	coordinator.log("done");
}

function get_compare_queries() {
	

	var queries=["SELECT id,data FROM foo.table1 ORDER BY id"
	             ,"SELECT id,table1_id,data FROM foo.table2 ORDER BY id"
	             ,"SELECT id,table2_id,mod_date, data FROM foo.table3 ORDER BY id"
	             ,"SELECT id, \"user\" from \"Schema.name\".\"user\" order by id"
	             ,"SELECT \"user\" as id , description from \"Schema.name\".\"Capital Idea\" order by \"user\""
	             ,"select last_value from \"Studly Spacey Schema\".\"user\" as id"
	             ,"select last_value from \"Schema.name\".\"a.periodic.sequence\" as id"

	            ];

	return queries;
}

run_test(coordinator,'testschemanames');
