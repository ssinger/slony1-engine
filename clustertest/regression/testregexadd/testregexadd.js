var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testregexadd/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testregexadd/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';
			
}


function init_tables() {
	var script=	coordinator.readFile('regression/testregexadd/init_add_tables.ik');

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

	var file = java.io.File.createTempFile("testdata",".sql");
	file.deleteOnExit();
	var fileWriter = new java.io.FileWriter(file);
	fileWriter.write("select nextval('test.seq1');");
	fileWriter.write("select nextval('testx.seq1');");
	fileWriter.close();
	return file;
}

/**
 * advance the sequence on the second node.
 * We need the sequence values to be equal to
 * pass the compare. Hopefully changes to the second
 * sequence were not replicated.
 */
function second_sequence_advance(coordinator) {
	var file = java.io.File.createTempFile("testdata2",".sql");
	file.deleteOnExit();
	var fileWriter = new java.io.FileWriter(file);
	//advance once from the init script and once from the generate_data_file
	fileWriter.write("select nextval('testx.seq1');");
	fileWriter.write("select nextval('testx.seq1');");
	fileWriter.close();
	psql = coordinator.createPsqlCommand('db2',file);
	psql.run();
	coordinator.join(psql);
		
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
	second_sequence_advance(coordinator);
	coordinator.log("done");
}

function get_compare_queries() {
	

	var queries=["SELECT 1 as id, last_value from testx.seq1",
				 "SELECT 1 as id, last_value from test.seq1 "
	            ];

	return queries;
}

run_test(coordinator,'testregexadd');
