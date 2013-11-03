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

LogShipping.prototype.getSyncWaitTime = function()
{

	return 3*120;
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
	
	java.lang.Thread.sleep(10*1000);
	 // create the second set and
	 // subscribe it.
	 // we then check that the log slave
	 // is populated.
	this.createSecondSet(1);
	this.subscribeSet(2,1,1,[2,3]);
	this.subscribeSet(2,1,3,[4]);
	this.populateReviewTable(1);	
	java.lang.Thread.sleep(10*1000);
	populate.stop();
	this.coordinator.join(populate);			
	this.slonikSync(1, 1);
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	
	java.lang.Thread.sleep(60*1000);
	this.coordinator.log("LogShipping.prototype.runTest - compare db4,6");
	this.compareDb('db4','db6');		
	this.truncateTest();
	this.ddlTest();

	
    this.coordinator.log("LogShipping.prototype.runTest - shut down slons");
	this.coordinator.log("Shutting down slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
	   
	logShippingDaemon.stop();
	this.coordinator.join(logShippingDaemon);
	this.dropDb(['db6']);
        this.coordinator.log("LogShipping.prototype.runTest - complete");
}


/**
 * Tests the behaviour of truncate through log shipping.
 * this is only done if the master is running on PG 8.4 or 
 * higher.
 *
 */
LogShipping.prototype.truncateTest = function() {
	var connection=this.coordinator.createJdbcConnection('db1');
	var db6Con = this.coordinator.createJdbcConnection('db6');

	var metaData = connection.getMetaData();
	if( metaData.getDatabaseMajorVersion()< 8
		|| (metaData.getDatabaseMajorVersion()==8
			&& metaData.getDatabaseMinorVersion()<4))
	 {
		 this.coordinator.log('skipping truncate test. PG version too old');
	 }


	
	this.slonikSync(1,1);
	var stat = db6Con.createStatement();
	try {
		var rs = stat.executeQuery("select count(*) FROM disorder.do_item_review;");
		rs.next();
		this.testResults.assertCheck('logshipping node is truncated',
									 rs.getInt(1),100);
		rs.close();
	}
	finally {
		stat.close();
	}

	stat = connection.createStatement();
	try {
		stat.execute("TRUNCATE disorder.do_item_review;");
	}
	catch(error) {
		this.testResults.assertCheck('truncate failed',true,true);
		
	}
	finally {
		stat.close();
		connection.close();
	}
	this.slonikSync(1,1);
	java.lang.Thread.sleep(30*1000);
	var stat = db6Con.createStatement();
	try {
		var rs = stat.executeQuery("select count(*) FROM disorder.do_item_review;");
		rs.next();
		this.testResults.assertCheck('logshipping node is truncated',
									 rs.getInt(1),0);
		rs.close();
	}
	finally {
		stat.close();
		db6Con.close();
	}
}

/**
 * test DDL - EXECUTE SCRIPT.
 * We want to make sure that EXECUTE SCRIPT, scripts get applied to log shipping nodes.
 */
LogShipping.prototype.ddlTest = function() {
	var db6Con = this.coordinator.createJdbcConnection('db6');
	
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'LogShipping.prototype.ddlTest\';\n' + 
		"EXECUTE SCRIPT(event node=1, SQL='CREATE TABLE not_replicated(a int4); INSERT INTO not_replicated values (1);');";
	
	var slonik = this.coordinator.createSlonik('CREATE TABLE', slonikPreamble,
											   slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik executed create table okay', slonik
								 .getReturnCode(), 0);
	this.slonikSync(1,1);
	java.lang.Thread.sleep(30*1000);

	var stat = db6Con.createStatement();
	try {
		var rs = stat.executeQuery("select count(*) FROM not_replicated;");
		rs.next();
		this.testResults.assertCheck('logshipping ddl replicated',
									 rs.getInt(1),1);
		rs.close();
	}
	catch (e) {
		this.testResults.assertCheck('select count threw an exception:' + e.getMessage(),true,false);
	}
	finally {
		stat.close();
		db6Con.close();
	}

	slonikScript = 'echo \'LogShipping.prototype.ddlTest\';\n' + 
		"EXECUTE SCRIPT(event node=1, SQL='DROP TABLE not_replicated;');";
	slonik = this.coordinator.createSlonik('DROP TABLE', slonikPreamble,
											   slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik executed drop table okay', slonik
								 .getReturnCode(), 0);
}
