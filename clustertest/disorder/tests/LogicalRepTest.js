/**
 * A basic test of the logical replication functions
 *
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


LogicalRepTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Basic logical replication test';
}
LogicalRepTest.prototype = new BasicTest();
LogicalRepTest.prototype.constructor = LogicalRepTest;

LogicalRepTest.prototype.getNodeCount = function() {
	return 2;
}

LogicalRepTest.prototype.isLogical=function(node_id) {
	if (node_id == 1) {
		return true;		
	}
	return false;
}

LogicalRepTest.prototype.runTest = function() {
        this.coordinator.log("LogicalRepTest.prototype.testActions - begin");	
	this.testResults.newGroup("Logical Rep Test");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	//First generate a baseline transaction rate.
	
	
        this.coordinator.log("LogicalRepTest.prototype.testActions - impose load");	
	//Start a background client load.
	var seeding=this.generateLoad();
	java.lang.Thread.sleep(60);
	seeding.stop();
	this.coordinator.join(seeding);
	
	/**
	 * Establish a transaction rate baseline
	 */
	
	
    this.coordinator.log("LogicalRepTest.prototype.testActions - add tables");	
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
	
	
    this.coordinator.log("LogicalRepTest.prototype.testActions - subscribe nodes");	
	/**
	 * Subscribe the nodes.
	 */
	this.subscribeSet(1,1, 1, [2]);
	this.slonikSync(1,1);

	this.generateLoad();
	java.lang.Thread.sleep(60);
	seeding.stop();
	this.coordinator.join(seeding);
	
	this.slonikSync(1,1);


	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
    this.coordinator.log("SubscribeUnderLoad.prototype.testActions - begin");	
	
	
	/**
	 * We need to validate that the transaction rate wasn't overly effected.
	 * Exactly what this means is hard to say, the db does more work during
	 * a subscription anyway, but shouldn't not stop. 
	 */
	
    this.coordinator.log("LogicalRepTest.prototype.testActions - compare db1,2");	
	this.compareDb('db1', 'db2');
	exit(-1);
	

}

