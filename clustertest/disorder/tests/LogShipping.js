/**
 * This test checks out basic log shipping functionality.
 * 
 * We replace node 5 with a log shipping node that gets its data from node 4.
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


LogShipping=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription="This test will test logshipping";
}
LogShipping.prototype = new BasicTest();
LogShipping.prototype.constructor = LogShipping;

LogShipping.prototype.getNodeCount=function() {
	
	//Say there is only 4 nodes,
	//since we make node 5 a log shipping nodevi
	//it has no slon (of its own) and needs no 
	//node configuration.
	return 4;
}

LogShipping.prototype.runTest = function() {	
        this.coordinator.log("LogShipping.prototype.runTest - begin");
	this.testResults.newGroup("Log Shipping");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	this.logdirectoryFile = java.io.File.createTempFile('logshipping','_spool');
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
	
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
        this.coordinator.log("LogShipping.prototype.runTest - set up subscription");
	/**
	 * Subscribe node 3
	 */
	this.subscribeSet(1,1,1,[3]);
	java.lang.Thread.sleep(10*1000);
	this.subscribeSet(1,1,3,[4]);
	this.slonikSync(1,1);
        this.coordinator.log("LogShipping.prototype.runTest - generate load");
	//Generate some load.
	var populate=this.generateLoad();
	
        this.coordinator.log("LogShipping.prototype.runTest - dump for log shipping");
	var dumpFile = java.io.File.createTempFile('slon_logshipping','.sql');
	//dumpFile.deleteOnExit();
	var dumpProcess = this.coordinator.createLogShippingDump('db4',dumpFile);
	dumpProcess.run();
	this.coordinator.join(dumpProcess);
	
        this.coordinator.log("LogShipping.prototype.runTest - drop DB");
	//Drop the DB, 
	//we want the dump to be restored into a clean state.
	this.prepareDb(['db6']);
	var loadInitial = this.coordinator.createPsqlCommand('db6',dumpFile);
	loadInitial.run();
	this.coordinator.join(loadInitial);
	this.testResults.assertCheck('initial load of db6 - logshipping okay',
			loadInitial.getReturnCode(),0);
	
	
        this.coordinator.log("LogShipping.prototype.runTest - start log shipping daemon");
	//Invoke log shipping daemon.
	var logShippingDaemon = this.coordinator.createLogShippingDaemon('db6',this.logdirectoryFile);
	logShippingDaemon.run();
	
	java.lang.Thread.sleep(30*1000);
	populate.stop();
	this.coordinator.join(populate);	
	this.slonikSync(1, 1);
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	
	
        this.coordinator.log("LogShipping.prototype.runTest - shut down slons");
	this.coordinator.log("Shutting down slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
	
	java.lang.Thread.sleep(30*1000);
	logShippingDaemon.stop();
	this.coordinator.join(logShippingDaemon);
        this.coordinator.log("LogShipping.prototype.runTest - compare db4,6");
	this.compareDb('db4','db6');
	
	this.dropDb(['db6']);
        this.coordinator.log("LogShipping.prototype.runTest - complete");
}

