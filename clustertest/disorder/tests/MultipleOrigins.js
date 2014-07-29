/**
 * Tests the behaviour of the application with multiple origin nodes
 * (for different sets)
 * 
 */


coordinator.includeFile('disorder/tests/BasicTest.js');




MultipleOrigins=function(coordinator,testResults) {
	Failover.call(this,coordinator,testResults);
	this.slonArray=[];
	this.testDescription='Tests a replication cluster with'
	+ ' multiple replication sets on different origins'
	+' sets are moved and nodes are failed.';
}
MultipleOrigins.prototype = new Failover();
MultipleOrigins.prototype.constructor = MultipleOrigins;

MultipleOrigins.prototype.runTest = function() {
        this.coordinator.log("MultipleOrigins.prototype.runTest - begin");
	this.testResults.newGroup("Multiple Origins");
	this.setupReplication();
	this.addTables();
        this.coordinator.log("MultipleOrigins.prototype.runTest - configuration configured");
	/**
	 * Start the slons.
	 */
	
        this.coordinator.log("MultipleOrigins.prototype.runTest - start slons");
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		this.slonArray[idx-1].run();
	}
	
        this.coordinator.log("MultipleOrigins.prototype.runTest - subscribe empty set");
	/**
	 * Subscribe the empty set (we have not added anything).
	 */
	this.subscribeSet(1,1,1,[3]);
	
	if(this.testResults.getFailureCount()== 0) {
		//No apparent errors.
		this.subscribeSet(1,1,3,[4,5]);		
	}	
	
        this.coordinator.log("MultipleOrigins.prototype.runTest - subscribe empty set 2");
	this.addCompletePaths();
	this.createSecondSet(2);
	this.subscribeSet(2,2,2,[4]);

	
        this.coordinator.log("MultipleOrigins.prototype.runTest - generate load");
	/**
	 * 
	 * 1    2
	 * \\   ||
	 *  3===4 
	 *  ||
	 *  5
	 *  
	 *  where set 1 is origin on 1, set 2 is origin on 2.
	 */
	var load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);
	this.slonikSync(1,1);
	this.compareDb('db1','db3');	
	this.compareDb('db1','db4');
	this.compareSecondSet('db2','db4');
        this.coordinator.log("MultipleOrigins.prototype.runTest - move set 1-->3");
	/**
	 * MOVE SET 1===>3
	 */	
	this.moveSet(1,1,4);
	this.currentOrigin='db4';
	load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);
        this.coordinator.log("MultipleOrigins.prototype.runTest - sync, compare");
	this.slonikSync(1,4);
	this.compareDb('db1','db3');
	
	
	this.moveSet(1,4,1);
	this.slonikSync(1,1);
	this.slonikSync(1,4);
	this.failNode(1,4,true);
	load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);

	this.slonikSync(1,4);
		//exit(-1);
		//	this.compareDb('db2','db3');	
	this.compareDb('db3','db4');
	this.compareDb('db4','db5');
	this.compareSecondSet('db2','db4');
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		this.slonArray[idx-1].stop();
		this.coordinator.join(this.slonArray[idx-1]);
	}
        this.coordinator.log("MultipleOrigins.prototype.runTest - complete");
}

MultipleOrigins.prototype.compareSecondSet=function(a,b) 
{

	oldCompare = this.compareQueryList;
	this.compareQueryList=[['select i_id,comments from disorder.do_item_review order by i_id','i_id']];
	this.compareDb(a,b);
	this.compareQueryList=oldCompare;
}