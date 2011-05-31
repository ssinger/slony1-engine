
var NUM_NODES=3;
 
coordinator.includeFile('regression/common_tests.js');


function get_schema() {
	var sqlScript = coordinator.readFile('regression/testpkeychange/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = '';
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');';
}

function init_tables() {
	var script =
		"set add table (id=1, set id =1, origin=1, fully qualified name= 'public.test1',key='test1_pkey' );\n";
	
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



function exec_ddl(coordinator) {
	preamble = get_slonik_preamble();
	var slonikScript = 'EXECUTE SCRIPT(set id=1, FILENAME=\'regression/testpkeychange/ddl_updates.sql\''
		+',EVENT NODE=1);\n';
	run_slonik('update ddl',coordinator,preamble,slonikScript);
}

function change_values() {
    var sql = "update test1 set data='bar' where id2=2;";
    return sql;
}

function do_test(coordinator) {

	exec_ddl(coordinator);
	wait_for_sync(coordinator);
	sql = change_values()
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	wait_for_sync(coordinator);

}

function get_compare_queries() {
	var queries=['SELECT id1::text||id2::text as id, id1,id2,id3,data FROM test1 order by id1,id2,id3'
		     ];
	return queries;
}

run_test(coordinator,'testchangepkey');
