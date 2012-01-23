/**
 * Tests the behaviour of the resubscribe command.
 * 
 */


coordinator.includeFile('disorder/tests/BasicTest.js');




Resubscribe=function(coordinator,testResults) {
	Failover.call(this,coordinator,testResults);
	this.slonArray=[];
	this.testDescription='Tests a replication cluster with'
	+ ' multiple replication sets on the same origin and using'
	+ 'the resubscribe command to reshape the cluster';
}
Resubscribe.prototype = new Failover();
Resubscribe.prototype.constructor = Resubscribe;

Resubscribe.prototype.runTest = function() {
        this.coordinator.log("Resubscribe.prototype.runTest - begin");
		this.testResults.newGroup("Resubscribe ");
		this.setupReplication();
		this.addTables();
        this.coordinator.log("Resubscribe.prototype.runTest - configuration configured");
		/**
		 * Start the slons.
		 */
		
        this.coordinator.log("Resubscribe.prototype.runTest - start slons");
		for(var idx=1; idx <= this.getNodeCount(); idx++) {
			this.slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
			this.slonArray[idx-1].run();
		}
		
        this.coordinator.log("Resubscribe.prototype.runTest - subscribe empty set");
		/**
		 * Subscribe the empty set (we have not added anything).
		 */
		this.subscribeSet(1,1,1,[2,3]);
		
		if(this.testResults.getFailureCount()== 0) {
			//No apparent errors.
			this.subscribeSet(1,1,3,[4,5]);		
		}	
		
        this.coordinator.log("Resubscribe.prototype.runTest - subscribe empty set 2");
		this.addCompletePaths();
		this.createSecondSet(1);
		this.subscribeSet(2,1,1,[2]);
		
	
		this.coordinator.log("Resubscribe.prototype.runTest - generate load");
		
		var load = this.generateLoad(1);
		java.lang.Thread.sleep(10*1000);
		
		/**
		 * now issue the resubscribe command to make 2 read from 3.
		 * we expect this to fail because 3 is not yet subscribed to set 2.
		 **/
		this.resubscribe(1,3,2,true);
		this.subscribeSet(2,1,1,[3]);
		this.resubscribe(1,3,2,false);
		/**
		 * we expect this to fail because it is circular.
		 * (commented out until we have circular detection)
		 * this.resubscribe(1,2,3,true);
		 */

		this.resubscribe(1,1,2,false);
		this.resubscribe(1,2,3,false);
		load.stop();
		
		this.coordinator.join(load);

		for(var idx=1; idx <= this.getNodeCount(); idx++) {		
			this.slonArray[idx-1].stop();
			this.coordinator.join(this.slonArray[idx-1]);
		}
        this.coordinator.log("Resubscribe.prototype.runTest - complete");
}

Resubscribe.prototype.resubscribe=function(origin,provider,receiver,
										   expectFailure)
{
	var slonikScript = 'resubscribe node(origin=' + origin + 
	',provider=' + provider + ',receiver=' + receiver + ');';
	slonikScript+='wait for event(origin=' + origin + ',confirmed=all, wait on='
	+ origin + ');\n';
	var preamble=this.getSlonikPreamble();
	var slonik=this.coordinator.createSlonik('resubscribe ',preamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('resubscribe worked',
								 slonik.getReturnCode()==0,
								 expectFailure ? false : true );
	if(expectFailure)
	{
		return;
	}
	/**
	 * now verify that both the origin and receiver have a correct sl_subscribe
	 */
	var node_array=[origin,receiver];
	for(var idx=0; idx < node_array.length; idx++) 
	{
		var connection=this.coordinator.createJdbcConnection('db' + 
															 node_array[idx]);
		var stat = connection.createStatement();
		try 
		{
			var rs = stat.executeQuery("select sub_provider from \"_" + 
									   this.getClusterName()  +
									    "\".sl_subscribe, \"_" + 
									   this.getClusterName() + 
									   "\".sl_set where sub_set=set_id" +
									   " and sub_origin=" + origin + 
									   " and sub_receiver=" + receiver);
			while(rs.next())
			{
				this.testResults.assertCheck("provider matches on node " + 
											 node_array[idx] ,
											 rs.getInteger(0),provider);
			}
			rs.close();
		}		
		catch(error) {
			this.testResults.assertCheck('review populate failed',true,true);
			
		}
		finally {
			stat.close();
			connection.close();
		}
	}//for
	this.coordinator.log("resubscribing node " + origin + "=> " + receiver);	
			
}