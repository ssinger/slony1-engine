/**
 * 
 * This tests the OMIT COPY option of subscribe set.
 * 
 * The test configures a standard cluster (per BasicTest) and then
 * subscribes node 2 per normal.   Nodes 3 and 4 then have a pg_dump restored into
 * them.   
 * 
 * The test will then adjust the data on nodes 3 and 4 (so we can validate that
 * no COPY overwrites the data).
 * 
 * Next nodes 3 and 4 are subscribed with OMIT copy.
 * 
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


OmitCopy=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Tests the OMIT COPY feature of subscribe set';

}
OmitCopy.prototype = new BasicTest();
OmitCopy.prototype.constructor = OmitCopy;

OmitCopy.prototype.runTest = function() {
        this.coordinator.log("OmitCopy.prototype.runTest - begin");
	
	this.testResults.newGroup("Omit Copy");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
	/**
	 * Subscribe the first node.
	 */
	this.subscribeSet(1,1,1,[2]);
	
	//Generate some load.
	var populate=this.generateLoad();
	
	var checkProcesses=[];
	checkProcesses[1]=this.startDataChecks(2);
	
	
	/**
	 * Now subscribe node 3 from node 1 (the origin) with omit copy.
	 * 1. We stop the load generation.
	 * 2. We copy via pg_dump the table to the node.
	 * 
	 */
	java.lang.Thread.sleep(10*1000);
	populate.stop();
	this.coordinator.log('data load stop requested');
	checkProcesses[1].stop();
	this.coordinator.join(checkProcesses[1]);	
	this.coordinator.log('data checking has stopped');
	this.coordinator.join(populate);
	this.coordinator.log('data load has stopped');
	var outFile = java.io.File.createTempFile("db1_dump",".sql");
	outFile.deleteOnExit();
	var pgdump = this.coordinator.createPgDumpCommand("db1",outFile.getAbsoluteFile(),["disorder.do_restock",
	                                                                                   "disorder.do_item",
	                                                                                   "disorder.do_customer",
	                                                                                   "disorder.do_inventory",
	                                                                                   "disorder.do_order",
	                                                                                   "disorder.do_order_line",	                                                                                   	                                                                                   
	                                                                                   "disorder.do_config"
	                                                                                   ],true);
	pgdump.run();
	this.coordinator.join(pgdump);
	this.subscribeOmitCopy(1,1,3,outFile);
	
	
	
	this.subscribeOmitCopy(1,3,4,outFile);
	
	//todo: test a subscribe from 2,5 where no path exists
	//This should fail gracefully?

	
	//Now verify that data actually replicates.
	//We touch every row on db1 to force the filler to be overwritten on db2.
	var jdbcConnection = this.coordinator.createJdbcConnection("db1");			
	var jdbcStatement = jdbcConnection.createStatement();
	jdbcStatement.execute("UPDATE disorder.do_customer SET c_name='reset' ");
	jdbcStatement.close();
	jdbcConnection.close();
	this.coordinator.log('Stopping load');
	
	this.slonikSync(1, 1);
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	
	
	this.coordinator.log("Shutting down slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
        this.coordinator.log("OmitCopy.prototype.runTest - complete");
}

OmitCopy.prototype.subscribeOmitCopy=function(origin,provider,subscriberNodeId,outFile) {
	var psqlInsert = this.coordinator.createPsqlCommand("db" + subscriberNodeId,outFile);
	psqlInsert.run();
	this.coordinator.join(psqlInsert);
	
	
	
	//UPDATE a row on the replica
	//We want to verify that the copy does NOT happen.
	//The only way we can do this is to alter the data on the replica
	//and validate our alterations are still present.
	var jdbcConnection = this.coordinator.createJdbcConnection("db"+subscriberNodeId);
	var jdbcStatement = jdbcConnection.createStatement();
	jdbcStatement.execute("UPDATE disorder.do_customer SET c_name='fooobar' ");
	var updateCnt = jdbcStatement.getUpdateCount();
	
	this.testResults.assertCheck('history rows updated', updateCnt > 0,true);
	
	var slonikPreamble = this.getSlonikPreamble();
        var slonikScript = 'echo \'OmitCopy.prototype.subscribeOmitCopy\';\n';
	slonikScript += "subscribe set(id=1, provider=" + provider+", receiver=" + subscriberNodeId+", omit copy=true, forward=yes);\n";
	
	var slonik=this.coordinator.createSlonik('omit copy subscribe',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('omit copy slonik result okay',slonik.getReturnCode(),0);
	this.slonikSync(1,1);
	
	var rs = jdbcStatement.executeQuery("SELECT COUNT(*) FROM disorder.do_customer  where c_name='fooobar'");
	if(rs.next() ) {
		var	 cnt= rs.getInt(1);
		this.testResults.assertCheck("data not overwritten by copy",cnt >0,true);		
	}
	else {
		this.testResults.assertCheck("data not overwritten by copy",false,true);
	}
	rs.close();
	jdbcStatement.close();
	jdbcConnection.close();
	
		
	
	
}
