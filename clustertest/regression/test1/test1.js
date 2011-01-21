var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/test1/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/test1/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');'
		+ "echo 'update functions on node 1 after initializing it';\n"
		+ "update functions (id=1);\n";

}

function init_tables() {
	var script = "set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');\n"
		+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');\n"
		+"\n"
		+"try {\n"
	    +"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3', key = 'no_good_candidate_pk', comment='bad table - table 3');\n"
		+"} on error {\n"
	    +"echo 'Tried to replicate table3 with no good candidate PK - rejected';\n"
		+"} on success {\n"
		+"echo 'Tried to replicate table3 with no good candidate PK - accepted';\n"
		+ "exit 1;\n"
		+"}\n"
		+"\n"
		+"set add table (id=4, set id=1, origin=1, fully qualified name = 'public.table4', comment='a table of many types');\n"
		+"set add table (id=5, set id=1, origin=1, fully qualified name = 'public.table5', comment='a table with composite PK strewn across the table');\n";
		
	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = yes);\n"
	+"echo 'sleep a couple seconds';\n"
	+"sleep (seconds = 2);\n"
	+"echo 'done sleeping';\n";
	return script;
}


function generate_data() {
	var numrows = random_number(50,1000);
	var sqlScript='';
	for(var idx = 0; idx < numrows; idx++) {
		var txtalen = random_number(1,100);
		var txta=random_string(txtalen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		var txtblen = random_number(1,100);
		var txtb = random_string(txtblen);
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
		sqlScript+= "INSERT INTO table5(d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11) values (E'"+txta+ra+"',E'"+txta+rb+"',E'"+txta+rc+"',E'"+txtb+ra+"',E'"+txtb+rb+"',E'" 
				+txtb+rc+"',E'"+txtb+ra+"',E'"+txtb+rb+"',E'"+txtb+rc+"',E'"+txtb+ra+"',E'"+txtb+rb+"');\n";
			
	}	
	return sqlScript;
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
	wait_for_sync(coordinator);
	
	
}

function get_compare_queries() {
	var queries=['SELECT id,data FROM table1 ORDER BY id',
	             'SELECT id,table1_id,data FROM table2 ORDER BY id',
	             'SELECT id,numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol, bitcol from table4 order by id',
	             'SELECT id::text||id2::text||id3::text as id,d1,d2,id2,d3,d4,d5,d6,id3,d7,d8,d9,d10,d11 from table5 order by id,id2,id3'];
	return queries;
}


run_test(coordinator,'test1');
