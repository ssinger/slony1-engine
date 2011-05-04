/**
 * This tests a heavy extended replication load.
 * 
 * It sets up the replication set and lets things replicate for a while.
 * 
 * It doesn't mess wiht the set configuration causing re-subscribes once the test
 * has been setup.
 * 
 * It includes node 6 as a log shipping node from node 4.
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


HeavyLoadTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription="Tests replication with a 'heavy load'."
		+ " In practice this means running 6 nodes including a log "
		+ " shipping node and letting transactions run for a while " 
		+ " without resubscribing";

}
HeavyLoadTest.prototype = new BasicTest();
HeavyLoadTest.prototype.constructor = HeavyLoadTest;


HeavyLoadTest.prototype.runTest = function() {
        this.coordinator.log("HeavyLoadTest.prototype.runTest - begin");
	
	this.testResults.newGroup("Heavy load");
	this.setupReplication();
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - start slons");
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	this.logdirectoryFile = java.io.File.createTempFile('HeavyLoadTest','_spool');
	this.logdirectoryFile["delete"]();
	this.logdirectoryFile.mkdir();
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		
		if(idx == 4) {
			
			slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx,this.logdirectoryFile);
		}
		else {
			slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		}
		slonArray[idx-1].run();
	}
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - add tables");
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - subscribe nodes 2,3");
	/**
	 * Subscribe node 2,3
	 */
	this.subscribeSet(1,1,1,[2,3]);
	java.lang.Thread.sleep(10*1000);
	this.subscribeSet(1,1,3,[4,5]);
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - add load");
	//Generate some load.
	var populate=this.generateLoad();
	this.prepareDb(['db6']);
	
	var dumpFile = java.io.File.createTempFile('slon_HeavyLoadTest','.sql');
	dumpFile.deleteOnExit();
	//wait until db4 is subscribed before creating the dump
	this.slonikSync(1,1);	
	var dumpProcess = this.coordinator.createLogShippingDump('db4',dumpFile);
	dumpProcess.run();
	this.coordinator.join(dumpProcess);
	
	//Drop the DB, 
	//we want the dump to be restored into a clean state.
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - load db6 from dump");
	var loadInitial = this.coordinator.createPsqlCommand('db6',dumpFile);
	loadInitial.run();
	this.coordinator.join(loadInitial);
	this.testResults.assertCheck('initial load of db6 - HeavyLoadTest okay',
			loadInitial.getReturnCode(),0);
	
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - turn on log shipping daemon");
	//Invoke log shipping daemon.
	var logShippingDaemon = this.coordinator.createLogShippingDaemon('db6',this.logdirectoryFile);
	logShippingDaemon.run();
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - full tilt replication load test for 20 minutes");
	var lastCount=0;
	for(var minute=0; minute <=20;minute++) {
		java.lang.Thread.sleep(60*1000);
		var con = this.coordinator.createJdbcConnection('db2');
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
        this.coordinator.log("HeavyLoadTest.prototype.runTest - full tilt replication load test complete");
	
	populate.stop();
	this.coordinator.join(populate);	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - sync");
	this.slonikSync(1, 1);
        this.coordinator.log("HeavyLoadTest.prototype.runTest - compare databases");
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db2','db1');
	this.compareDb('db4','db5');
	
        this.coordinator.log("HeavyLoadTest.prototype.runTest - shut down slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
        this.coordinator.log("HeavyLoadTest.prototype.runTest - logshipping catchup - 30s");
	java.lang.Thread.sleep(30*1000);
	logShippingDaemon.stop();
	this.coordinator.join(logShippingDaemon);
	
	this.compareDb('db4','db6');
	this.dropDb(['db6']);
        this.coordinator.log("HeavyLoadTest.prototype.runTest - begin");
}

