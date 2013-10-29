/**
 * This file contains support functions for the 'common' tests.
 */


/**
 * Create the origin database. (db1)
 */
function init_origin_rdbms(coordinator,testname) {
	
	var createdb = coordinator.createCreateDb('db1');
	createdb.run();
	coordinator.join(createdb);
	results.assertCheck('createdb worked',createdb.getReturnCode(),0);
			
	var sql = 'CREATE LANGUAGE plpgsql;\n';
	sql += get_schema();
	sql += gen_weak_user();
	var psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	coordinator.join(psql);
	
}

function create_subscribers(coordinator) {
	var sql = 'CREATE LANGUAGE plpgsql;\n';
	sql += get_schema();
	sql += gen_weak_user();
	for(var node = 2; node <= NUM_NODES; node++) {
		var createdb = coordinator.createCreateDb('db' + node);
		createdb.run();
		coordinator.join(createdb);
		results.assertCheck('createdb worked',createdb.getReturnCode(),0);
		var psql = coordinator.createPsqlCommand('db'+node,sql);
		psql.run();
		coordinator.join(psql);
	}
}

function generate_weak_slony_grants(coordinator) {
	return '';
}

function gen_weak_user() {
	return '';
}

function drop_databases(coordinator) {
	for(var node = 1; node <= NUM_NODES; node++) {
		var dropdb = coordinator.createDropDbCommand('db' + node);
		dropdb.run();
		coordinator.join(dropdb);
	}
}
function cleanup(coordinator) {
	
}


var slon_array=[];

function launch_slon(coordinator) {
	for(var idx = 1; idx <= NUM_NODES; idx ++) {
		var slon = coordinator.createSlonLauncher('db' + idx);
		slon.run();
		slon_array[idx-1]=slon;
	}
}

function terminate_slon(coordinator) {
	for(var idx=0; idx < slon_array.length; idx++) {
		slon_array[idx].stop();		
		coordinator.join(slon_array[idx]);		
	}
}

function wait_for_catchup(coordinator) {
	
}

function diff_db(coordinator) {
	
}

function store_nodes() {
	var script='';
	for(var idx=2; idx <= NUM_NODES; idx++) {
		script+='STORE NODE (id=' + idx + ', comment=\'node ' + idx + '\',event node=1);\n'; 
	}
	return script;
}

function store_paths() {
	var script='';
	for(var i=1; i <= NUM_NODES; i++) {
		for(var j=1; j < i; j++) {
			script+='STORE PATH (SERVER=' + i + ', CLIENT=' + j + 
			', CONNINFO=@CONNINFO'+i+');\n';
			script+='STORE PATH (SERVER=' + j + ', CLIENT=' + i + 
			', CONNINFO=@CONNINFO'+j+');\n';
		}
	}
	return script;
	
}

function run_slonik(name,coordinator,preamble,script) {
	var slonik = coordinator.createSlonik(name,preamble,script);
	
	//If replication has stopped because of a problem the sync will never finish
	//we don't want to be waiting on slonik forever.
	//Setup a timer event.
	var abortTest = {
		onEvent : function(source,eventType) {
			results.assertCheck("slonik did not finish in the timelimit:"+name,true,false);
			//kill the slonik			
			slonik.stop();
			
			
		}		
	
	};
	var timeoutObserver = new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(abortTest);
	var timer = coordinator.addTimerTask('sync failed',60*8 /*2 minutes*/, timeoutObserver );
	
	slonik.run();
	coordinator.join(slonik);
	results.assertCheck('slonik return code for' + name, slonik.getReturnCode(),0);
	coordinator.removeObserver(timer,Packages.info.slony.clustertest.testcoordinator.Coordinator.EVENT_TIMER,timeoutObserver);
}

function get_slonik_preamble() {
	
	
	var slonikPre = 'cluster name=slonyregress;\n';
	for(var idx = 1; idx <= NUM_NODES; idx++) {
		var port='';
		if(properties.getProperty('database.db' + idx + '.port') != null) {
			port='port=$database.db' + idx + '.port ';
		}
		slonikPre+=  'node '+idx+' admin conninfo=\'dbname=$database.db'+idx+'.dbname host=$database.db'+idx+'.host '+port+' user=$database.db'+idx+'.user.slony password=$database.db'+idx+'.password\';\n';
		slonikPre+= 'define CONNINFO'+idx+' \'dbname=$database.db'+idx+'.dbname host=$database.db'+idx+'.host '+port+' user=$database.db'+idx+'.user.slony password=$database.db'+idx+'.password\';\n';
	}
	return slonikPre;

}


var random=new java.util.Random();
function random_number(min,max) {
	num = random.nextInt(max-min);
	return num+min;	
}

function random_string(slen) {
	var result='';
	for(var idx = 0; idx < slen; idx++) {
		c = random_number(48,122);
		result += new java.lang.Character(c).toString();
	}
	return result;
}

function do_compare(coordinator) {
	var queries =get_compare_queries();
	for(var idx=0; idx < queries.length; idx++) {
		for(var dbidx =2; dbidx <=NUM_NODES;dbidx++) {
			compareOp = coordinator.createCompareOperation('db1','db' + dbidx,queries[idx],'id');
			compareOp.run();
			coordinator.join(compareOp);
			results.assertCheck('database comparison',compareOp.getResultCode(),Packages.info.slony.clustertest.testcoordinator.CompareOperation.COMPARE_EQUALS);
		}
	}
}

function run_test(coordinator,testname) {
	results.newGroup(testname);
	init_origin_rdbms(coordinator);	
	create_subscribers(coordinator);
	load_data(coordinator);
	
	var slonikPreamble = get_slonik_preamble(coordinator);	
	var slonikScript = init_cluster();
	slonikScript += create_set();
	slonikScript += init_tables();
	run_slonik('init set',coordinator,slonikPreamble,slonikScript);
	
	
	//run slonik.
	slonikScript = store_nodes();
	slonikScript += store_paths();
	
	//run_slonik
	generate_weak_slony_grants(coordinator);
	run_slonik('setup nodes',coordinator,slonikPreamble,slonikScript);
			
	launch_slon(coordinator);
	
	
	
	slonikScript = subscribe_set();
	//run slonik
	run_slonik('subscribe',coordinator,slonikPreamble,slonikScript);
	wait_for_sync(coordinator);
	do_test(coordinator);
	wait_for_sync(coordinator);
	//diff
	do_compare(coordinator);
	
	
	//terminate slon
	terminate_slon(coordinator);
	drop_databases(coordinator);
	cleanup(coordinator);
	
	
	
}

function wait_for_sync(coordinator) {
	var slonikScript = ' sync(id=1'  + ');\n';
	slonikScript += ' wait for event(origin=1' +  ', wait on=1'  + ',confirmed=all);\n';
	var preamble = get_slonik_preamble();
	
	run_slonik('sync',coordinator,preamble,slonikScript);

	
}
