/**
 * Some tests of the DROP SET command when we
 * create a new set(on a different node after)
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

RecreateSet=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription = 'This test exercises the DROP SET command.\n'
		+ ' It includes testing of DROP SET while a subsription to that set is\n'
		'possibly in progress and then recreating the set on a different origin.'
}
RecreateSet.prototype = new BasicTest();
RecreateSet.prototype.constructor = RecreateSet;


RecreateSet.prototype.runTest = function() {
	this.testResults.newGroup("Recreate Set");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	

	this.addCompletePaths();
	
	this.addTables();
	var load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join (load);
	
	this.subscribeSet(1,1,1,[2]);
	//Create + subscribe a second set to nodes 2 + 3.
	this.createSecondSet(1);
	this.subscribeSet(2,1,1,[2,3]);
	
	/**
	 * take a lock out on a table on db3 to prevent the subscribe
	 * from finishing (for set 1).
	 */
	var conn = this.coordinator.createJdbcConnection('db3');
	var stat = conn.createStatement();
	conn.setAutoCommit(false);
	stat.execute("LOCK TABLE disorder.do_customer");
	var subscribeArray = this.subscribeSetBackground(1,1,1,[3]);
	subscribeArray[0].run();
	/**
	 * sleep long enough to make sure the
	 * slon for node 3 sees the list of tables on node 1.
	 */
	java.lang.Thread.sleep(10*1000);


	/**
	 * now we DROP set 2 and recreate it on node 2.
	 * The idea is that the CREATE SET from node 2
	 * will reach node 3 before the DROP SET from node 1.
	 * This is because the node 3 remote worker is busy
	 * due to the lock on set 1.
	 */
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'drop set (id=2, origin=1);\n';

	var slonik=this.coordinator.createSlonik('drop set 1',slonikPreamble,slonikScript);	
	slonik.run();


	
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop set 1 worked',slonik.getReturnCode(),
								0);
	this.createSecondSet(2);
	java.lang.Thread.sleep(60*1000);	
	conn.rollback();

	this.subscribeSet(2,2,2,[3]);	
	// The subscribe on node 3 won't finish with that lock
	// due to internal slon mutexes.  We just need
	// to make sure the remoteWorkerThread_2 on
	// node 3 gets ahead of remoteWorkerThread_1 on
	// node 3.
	//java.lang.Thread.sleep(60*1000);
	
	this.coordinator.join(subscribeArray[0]);
	
	//verify things.

	for(var idx=0; idx < this.getNodeCount(); idx++) {
		
		slonArray[idx].stop();
		this.coordinator.join(slonArray[idx]);
	}
	var rs = stat.executeQuery("select set_origin from _" + 
							   this.getClusterName()+ ".sl_set where"
							   + " set_id=2");
	if(rs.next()) {
		this.testResults.assertCheck('set 2 has node 2 as origin',
									 rs.getInt(1),2);
	}
	else {
		this.testResults.assertCheck('set 2 is missing from node 3',
									 true,false);
	}
	rs.close();
	stat.close();
	conn.close();

}

