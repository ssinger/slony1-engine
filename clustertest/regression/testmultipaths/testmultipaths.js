var NUM_NODES=3;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testmultipaths/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testmultipaths/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n'
		+"echo 'update functions on node 1 after initializing it';\n" 
		+"update functions (id=1);\n" ;
				


}




function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');\n"
	+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');\n"
	+"set add table (id=4, set id=2, origin=1, fully qualified name = 'public.table4', comment='a table of many types');\n"
	return script;
}
	


function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n'
		+"create set (id=2, origin=1, comment='set number 2');";

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=all, wait on=1);\n"
		+"subscribe set ( id = 1, provider = 1, receiver = 3, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=all, wait on=1);\n"
		+"subscribe set ( id = 2, provider = 1, receiver = 2, forward = yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=all, wait on=1);\n";	
	return script;
}

function test_invalid_config() {
	var preamble = get_slonik_preamble();
	var script ="subscribe set ( id = 2, provider = 2, receiver = 3, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=all, wait on=1);\n";
	var slonik = coordinator.createSlonik('test invalid path',preamble,script);
	slonik.run();
	coordinator.join(slonik);
	results.assertCheck('slonik did not add invalid path',slonik.getReturnCode()!=0,true);
	//Now subscribe node 3 from 1. We do this so the compare works later
	script ="subscribe set ( id = 2, provider = 1, receiver = 3, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=all, wait on=1);\n";
	var slonik = coordinator.createSlonik('valid subscribe path',preamble,script);
	slonik.run();
	coordinator.join(slonik);
	results.assertCheck('slonik did not add invalid path',slonik.getReturnCode(),0);
}

function generate_data(coordinator) {
	var num_rows = random_number(50,1000);
	var sqlScript='';
	for(var idx=0; idx < num_rows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		//txta = txta.replace("'","''");
		textlen = random_number(1,100);
		var txtb = random_string(textlen);
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		var ra=random_number(1,9);
		var rb = random_number(1,9);
		var rc = random_number(1,9);
		
		sqlScript += "INSERT INTO table1(data) VALUES (E'"+txta+"');\n";
		sqlScript += "INSERT INTO table1(data) VALUES (E'"+txta+"');\n" ;
		sqlScript += "INSERT INTO table2(table1_id,data) SELECT id, E'"+txtb +"' FROM table1 WHERE data=E'"+txta+"';\n";
		sqlScript += "INSERT INTO table3(id) SELECT id FROM table2 WHERE data =E'"+txtb+"';\n";
		sqlScript += "INSERT INTO table4(numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values ('"
			+ra+rb+"."+rc+"','"+ra+"."
			+rb+rc+"','("+ra+","+rb+")','(("+ra+","+ra+"),("+rb+","+rb+"),("+rc+","+rc+"),("+ra+","+rc+"))','(("+ra+","+rb+"),("
			+rc+","+ra+"),("+rb+","+rc+"),("+rc+","+rb+"))','<("+ra+","+rb+"),"+rc+">','192.168."+ra+"."+rb+rc+"','08:00:2d:0"+ra+":0"+rb+":0"+rc+"',X'"
			+ra+rb+rc+"');\n"; 
		
			
	}
	return sqlScript;
}

function do_test(coordinator) {
	
	var sqlScript='';
	coordinator.log("done");
	sqlScript = generate_data(coordinator);
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	test_invalid_config();
	
	
	
}

function get_compare_queries() {
	

	var queries=['SELECT id,data FROM table1 ORDER BY id'
	             ,'SELECT id,table1_id,data FROM table2 ORDER BY id'
	             ,'SELECT id,numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol, bitcol from table4 order by id' 
	            ];

	return queries;
}

run_test(coordinator,'testmultipaths');
