
coordinator.includeFile('disorder/tests/BasicTest.js');


BigBacklogTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription="Tests replication with a backlog" +
			"All slons are stopped, database updates are "
			+" performed for a few minutes then the slons" 
			+" are started again.";
	 this.slonArray=[];
}
BigBacklogTest.prototype = new BasicTest();
BigBacklogTest.prototype.constructor = BigBacklogTest;


BigBacklogTest.prototype.runTest = function() {
	
	this.coordinator.log("BigBacklogTest.prototype.runTest - start");
	this.testResults.newGroup("Big Backlog ");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	
	this.logdirectoryFile = java.io.File.createTempFile('BigBacklogTest','_spool');
	this.logdirectoryFile["delete"]();
	this.logdirectoryFile.mkdir();
	this.startSlons();
	
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
	/**
	 * Subscribe node 2,3
	 */
	this.subscribeSet(1,1,1,[2,3]);
	java.lang.Thread.sleep(10*1000);
	this.subscribeSet(1,1,3,[4,5]);
	
	//Generate some load.
	var populate=this.generateLoad();
	this.prepareDb(['db6']);
	
	var dumpFile = java.io.File.createTempFile('slon_BigBacklogTest','.sql');
	dumpFile.deleteOnExit();
	var dumpProcess = this.coordinator.createLogShippingDump('db4',dumpFile);
	dumpProcess.run();
	this.coordinator.join(dumpProcess);
	
	//Drop the DB, 
	//we want the dump to be restored into a clean state.
	
	var loadInitial = this.coordinator.createPsqlCommand('db6',dumpFile);
	loadInitial.run();
	this.coordinator.join(loadInitial);
	this.testResults.assertCheck('initial load of db6 - BigBacklogTest okay',
			loadInitial.getReturnCode(),0);
	
	
	//Invoke log shipping daemon.
	var logShippingDaemon = this.coordinator.createLogShippingDaemon('db6',this.logdirectoryFile);
	logShippingDaemon.run();
	/**
	 * Stop all of the slons
	 */
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		this.slonArray[idx].stop();
		this.coordinator.join(this.slonArray[idx]);
	}
	
	
	this.coordinator.log('replicating at full tile for 5 minutes');
	var lastCount=0;
	for(var minute=0; minute <=5;minute++) {
		java.lang.Thread.sleep(60*1000);
		var con = this.coordinator.createJdbcConnection('db1');
		var stat = con.createStatement();
		var rs = stat.executeQuery("SELECT COUNT(*) FROM disorder.do_order;");
		rs.next();
		var count = rs.getInt(1);
		this.testResults.assertCheck('at minute ' + minute + ' orders are increasing',count>lastCount,true);
		lastCount=count;
		rs.close();
		stat.close();
		con.close();
	}
	
	//
	// Now start the slons back.
	this.startSlons();
	this.slonikSync(1,1);
	populate.stop();
	this.coordinator.join(populate);	
	this.slonikSync(1, 1);
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db2','db1');
	this.compareDb('db4','db5');
	
	this.coordinator.log("Shutting down slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		this.slonArray[idx-1].stop();
		this.coordinator.join(this.slonArray[idx-1]);	
	}
	this.coordinator.log("waiting to give time for log shipping to catch up");
	java.lang.Thread.sleep(30*1000);
	logShippingDaemon.stop();
	this.coordinator.join(logShippingDaemon);
	
	this.compareDb('db4','db6');
	this.dropDb(['db6']);
	this.coordinator.log("BigBacklogTest.prototype.runTest - complete");
}

BigBacklogTest.prototype.startSlons=function() {
	this.coordinator.log("BigBacklogTest.prototype.startSlons - begin");
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		
		if(idx == 4) {
			
			this.slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx,this.logdirectoryFile);
		}
		else {
			this.slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		}
		this.slonArray[idx-1].run();
	}
	this.coordinator.log("BigBacklogTest.prototype.startSlons - complete");
}