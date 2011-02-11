coordinator.includeFile('disorder/tests/BasicTest.js');

CleanupTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='This test checks to see if the. '
		+'cleanup thread is working as expected. ';
}
CleanupTest.prototype = new BasicTest();
CleanupTest.prototype.constructor = CleanupTest;

CleanupTest.prototype.runTest = function() {
	
	this.testResults.newGroup("Cleanup Test");
	//this.prepareDb(['db1','db2','db3','db4','db5']);
	this.setupReplication();
	this.addCompletePaths();
	this.addTables();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
	    var confParams=this.getSlonConfFileMap('db'+idx);
	    confParams.put('cleanup_interval','20');
	    slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx,
								   confParams);
		slonArray[idx-1].run();
	}
	
	/**
	 * Subscribe the set.
	 */
	this.subscribeSet(1,1,1,[2,3]);
	var load = this.generateLoad();
	Packages.java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);
	/**
	 * move the set to node 2.	 
	 * drop node 1.
	 * 
	 */
	this.moveSet(1,1,2);
	this.subscribeSet(1,2,2,[3]);
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript='drop node(id=1,event node=2);\n'
	+ 'wait for event(origin=2,confirmed=all, wait on=2);\n';
	var slonik=this.coordinator.createSlonik('drop node',slonikPreamble,
	      				 slonikScript);

	this.verifyLogHasEvents(3,1,true);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop node 1 succeeded',
				     slonik.getReturnCode(),0);
	/**
	 * now restart slon 3 with a smaller cleanup interval.
	 */
	slonArray[3-1].stop();
	this.coordinator.join(slonArray[3-1]);
	var confParams=this.getSlonConfFileMap('db'+idx);
	confParams.put('cleanup_interval','1');
	slonArray[3-1] = this.coordinator.createSlonLauncher('db3',
							     confParams);
	slonArray[3-1].run();
	
	/**
	 * wait 170 seconds twice for good measure.
	 * two calls to cleanupEvent must take place
	 */
	Packages.java.lang.Thread.sleep(2*(70*1000+100*1000));
	this.verifyLogHasEvents(3,1,false);       

	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}

}

/**
 * this function checks to see if node_id has events from (orign_id equals)
 * events_from_id.
 *
 * The function checks both sl_event and the union of sl_log_1 and sl_log_2. 
 *
 */
CleanupTest.prototype.verifyLogHasEvents=function(node_id,events_from_id,
						      expect_events)
{
    var con = this.coordinator.createJdbcConnection('db' + node_id);
    var stat = con.createStatement();
    var rs = stat.executeQuery("SELECT COUNT(*) FROM _" + 
			       this.getClusterName() + ".sl_event " +
			       "WHERE ev_origin=" + events_from_id);
    rs.next();
    this.testResults.assertCheck('sl_event has events from ' + 
				 events_from_id, rs.getInt(1)!=0,
				 expect_events);

    rs.close();
    rs=stat.executeQuery("SELECT sum(count) FROM ( SELECT count(*) FROM _" + 
			 this.getClusterName() 
			 + ".sl_log_1 WHERE log_origin=" + events_from_id
			 + " UNION SELECT COUNT(*) FROM _" + this.getClusterName() + ".sl_log_2"
			 + " WHERE log_origin=" + events_from_id
			 + " ) as comb");
    rs.next();
    this.testResults.assertCheck('sl_log has events from ' + 
				 events_from_id,rs.getInt(1)!=0,
				 expect_events);
    rs.close();
    stat.close();
    con.close();

}