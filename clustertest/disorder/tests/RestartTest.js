/**
 * This test will test various functionaility related to restarts
 * 
 * 1.  The Restart notification to ensure that slons do restart.
 * 
 * 2. Killing a slon and restarting it, replication should pick back up
 * 
 * 3.  Killing a pg backend, slon should reconnect.
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

RestartTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Tests situations where the slon process'
	+ ' gets restarted. This includes the NOTIFY restart, '
	+' killing slon instances and restarting them,'
	+' and killing the pg backends (where available)';	
}
RestartTest.prototype = new BasicTest();
RestartTest.prototype.constructor = RestartTest;

RestartTest.prototype.runTest = function() {
        this.coordinator.log("RestartTest.prototype.runTest - begin");
	this.testResults.newGroup("Restart test");	
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	  this.slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		this.slonArray[idx-1].run();
	}
	this.addTables();
	this.subscribeSet(1,1,1,[2,3]);
	this.subscribeSet(1,1,3,[4,5]);
	
	this.notifyRestart(1);
	this.notifyRestart(2);
	this.notifyRestart(3);
	
	var load = this.generateLoad();
	this.killBackend(1);
	this.killBackend(2);
	this.killBackend(3);
	this.killBackend(4);
	this.killBackend(5);
	java.lang.Thread.sleep(5*1000);
	load.stop();
	this.coordinator.join(load);
	
	this.slonikSync(1,1);
	
	
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db1','db5');
	
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		
		this.slonArray[idx].stop();
		this.coordinator.join(this.slonArray[idx]);
	}
	
	
        this.coordinator.log("RestartTest.prototype.runTest - complete");
}

RestartTest.prototype.notifyRestart=function(node_id) {
	
	var restarted=false;
	var thisRef=this;
	
	var onRestart  = {
		onEvent : function(source,eventType) {
			restarted=true;
			thisRef.coordinator.stopProcessing();
		}
	};
	
	var onTimeout = {
			onEvent : function(object, event) {
				thisRef.coordinator.stopProcessing();				
			}
		};
	
	var timedWaitObserver = new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(
			onTimeout);
	
	var timer = coordinator.addTimerTask('wait for notify', 60,
			timedWaitObserver);
	
	var restartObserver = new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(
			onRestart);
	this.coordinator.registerObserver(this.slonArray[node_id-1],
					Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_SLON_STARTED,
					restartObserver);
	var con = this.coordinator.createJdbcConnection('db' + node_id);
	var stat = con.createStatement();
	this.coordinator.log("issuing restart NOTIFY query");
	stat.execute("NOTIFY \"_" + this.getClusterName()  + "_Restart\""); 
	stat.close();
	con.close();
	this.coordinator.log('processing events for 4 seconds');	
	this.coordinator.processEvents();
	this.coordinator.log('processing events complete');
	this.coordinator.removeObserver(this.slonArray[node_id-1],
			Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_SLON_STARTED,
			restartObserver);
	this.coordinator.removeObserver(timer,
			Packages.info.slony.clustertest.testcoordinator.Coordinator.EVENT_TIMER,
			timedWaitObserver);
	this.testResults.assertCheck('slon was restarted',restarted,true);
}

/**
 * This method will terminate all pg backends for all slon connections that
 * are connected to a particular database (the one passed in).
 * 
 * All of the slon processes should reconnect and replication should resume.
 * 
 */
RestartTest.prototype.killBackend=function(node_id) {
	var con = this.coordinator.createJdbcConnection('db' + node_id);
	var stat = con.createStatement();
	var rs = stat.executeQuery(" select substring(version(),E'PostgreSQL ([0-9]\.[0-9])');");
	rs.next();
	var version = java.lang.Math.round(rs.getFloat(1)*10);
	
	if(version < 84) {
		//pg_terminate_backend is not supported with this version.
		//skip this test since there is no clean way of killing the backend.
		this.coordinator.log('skipping killBackend tests because the postgresql version is too old:' + version);
		rs.close();
		stat.close();
		con.close();
		return;
	}
	var pidcolumn='';
	if(version < 92) {
		pidcolumn='procpid';
	}
	else {
		pidcolumn='pid';
	}
	var query = "select pg_terminate_backend("+pidcolumn+") FROM pg_stat_activity where datname='"
		+properties.get("database.db" + node_id + ".dbname") + "' AND " + pidcolumn +" <> pg_backend_pid() AND"
	+ " usename='" + properties.get("database.db" + node_id + ".user.slony") + "'";
	
	this.coordinator.log(query);
	var rs = stat.executeQuery(query);
	while(rs.next()) {
		this.coordinator.log('terminating backend');
	}
		
	rs.close();
	stat.close();
	con.close();
	
	
}
