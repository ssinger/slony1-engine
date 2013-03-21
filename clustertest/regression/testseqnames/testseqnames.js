var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testseqnames/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testseqnames/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';
			

}


function init_tables() {
	//var script=	coordinator.readFile('regression/testseqnames/init_add_tables.ik');
	var script='include<regression/testseqnames/init_add_tables.ik>';

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
	var num_rows = random_number(25,35);
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
		fileWriter.write( "INSERT INTO foo.table1(data) VALUES (E'"+txta+"');");
	    fileWriter.write("INSERT INTO foo.table2(table1_id,data) SELECT id, '"+txtb+"' FROM table1 WHERE data=E'"+txta+"';\n");
	    fileWriter.write("INSERT INTO foo.table3(table2_id) SELECT id FROM table2 WHERE data =E'"+txtb+"';\n");
	    fileWriter.write("INSERT INTO \"Schema.name\".\"Capital Idea\" (\"user\", description) values (E'"+txta+"', E'"+txtb+"');\n");
	    fileWriter.write("INSERT INTO \"Schema.name\".\"user\" (\"user\", id) values (E'"+txtb+"', "+txtblen+");\n");
	    fileWriter.write("select nextval('\"Schema.name\".\"a.periodic.sequence\"');");
	    fileWriter.write("select nextval('\"Studly Spacey Schema\".\"user\"');\n");
	    fileWriter.write("select nextval('\"Schema.name\".\"a.periodic.sequence\"');\n");
	    fileWriter.write("select nextval('\"Studly Spacey Schema\".\"user\"');\n");
	    var d4array = [ 8,3,9, 0, 6, 7, 1, 4, 5, 2];
	    for(var  d4idx=0; d4idx < d4array.length; d4idx++) {
	    	var d4 = d4array[d4idx];
	    	var d2array=[0, 2, 1, 3, 9, 5, 6, 4, 8, 7];
	    	for(var d2idx=0; d2idx < d2array.length; d2idx++) {
	    		var d2 = d2array[d2idx];
	    		for(var d1=0; d1 <=1; d1++) {
	    			var d3array =[5, 2, 1, 6, 4, 8, 3, 9, 0, 7];
	    			for(var d3idx=0; d3idx < d3array.length; d3idx++) {
	    				var d3=d3array[d3idx];			
	    				fileWriter.write("select nextval('public.seq40"+d1+""+d2+""+d3+""+d4+"');\n");
	    			}
	    		}
	    	}
	    }
		
			
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
	             ,"select last_value from \"Studly Spacey Schema\".\"user\""
	             ,"select last_value from \"Schema.name\".\"a.periodic.sequence\""
	            ];

	return queries;
}

run_test(coordinator,'testseqnames');
