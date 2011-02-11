var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testlargetuples/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testlargetuples/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';


}

function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='sub table 1');\n";
                                                                                                  
	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=2, wait on=1);\n";
	return script;
}


var subTxnPsqls=[];
function generate_data_via_file(coordinator) {
	
	
	var numrows=random_number(50,1000);
	coordinator.log("Generating data 0%");
	var sqlFile = java.io.File.createTempFile("testdata","sql");
	sqlFile.deleteOnExit();
	var fileWriter = new java.io.FileWriter(sqlFile);
	
	for(var idx = 0; idx < numrows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		txta = new java.lang.String(txta).replace("'","''");
		textlen = random_number(1,100);
		var txtblen = random_number(1,100);
		var txtb = random_string(txtblen);		
		var txtclen = random_number(1,100);
		var txtc = random_string(txtclen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		txtb = new java.lang.String(txtb).replace("'","''");
		txtc = new java.lang.String(txtc).replace("\\","\\\\");
		txtc = new java.lang.String(txtc).replace("'","''");

		fileWriter.write( "INSERT INTO table1(data) VALUES (E'" + txta + "');\n");
		fileWriter.write( "INSERT INTO table1(data) VALUES (E'");
		for(var repeatCnt=0; repeatCnt<100; repeatCnt++) {
			fileWriter.write(txta + ' padding things out to make them bigger ' 
			+ txtb + " and even bigger" + txtc);			
		}
		fileWriter.write("');\n");
		if(idx % (numrows/5) ==0 ) {
			coordinator.log("generated " + (idx/numrows)*10 + "%");
		}
	}
	fileWriter.close();
	
	return sqlFile;
}


function do_test(coordinator) {

	var numrows = random_number(150,350);
	var trippoint = numrows / 20;
	var percent=0;
	
	
	var sqlFile = generate_data_via_file(coordinator);
	coordinator.log("inserting data");
	psql = coordinator.createPsqlCommand('db1',sqlFile);
	psql.run();
	coordinator.join(psql);
	//Switch the log
	var sqlScript = 'select ' + properties.getProperty("clustername") + '.logswitch_start();';
	coordinator.log("calling log switch");
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
	sqlFile = generate_data_via_file(coordinator);
	coordinator.log("inserting data");
	psql = coordinator.createPsqlCommand('db1',sqlFile);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);
	
	
	
}

function get_compare_queries() {
	

	var queries=['SELECT id,data FROM table1 ORDER BY id' 
	            ];

	return queries;
}

run_test(coordinator,'testinherit');
