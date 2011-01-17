/**
 * Subscribe database sets while the provider (db1) is under transaction load.
 *
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


SubscribeUnderLoad=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Tests subscribing a set while '
	+'data is being added to the tables in that set';
}
SubscribeUnderLoad.prototype = new BasicTest();
SubscribeUnderLoad.prototype.constructor = SubscribeUnderLoad;

SubscribeUnderLoad.prototype.runTest = function() {
	
	this.testResults.newGroup("Subscribe Under Load");
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
	
	
	//Start a background client load.
	var seeding=this.generateLoad();
	
	
	/**
	 * Establish a transaction rate baseline
	 */
	
	
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();
	
	
	
	/**
	 * Subscribe the nodes.
	 */
	this.subscribeSet(1,1, 1, [2,3]);
	this.subscribeSet(1, 1,3, [4,5]);
	
	var checkProcesses=[];
	for(var idx=1; idx <=this.getNodeCount(); idx++) {
		checkProcesses[idx-1] = this.startDataChecks(idx);
	}
	
	
	seeding.stop();
	this.coordinator.join(seeding);
	this.slonikSync(1,1);
	
	for(var idx=1; idx <=this.getNodeCount(); idx++) {
		checkProcesses[idx-1].stop();
		this.coordinator.join(checkProcesses[idx-1]);
	}
	
	/**
	 * We need to validate that the transaction rate wasn't overly effected.
	 * Exactly what this means is hard to say, the db does more work during
	 * a subscription anyway, but shouldn't not stop. 
	 */
	
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	this.compareDb('db1', 'db5');

	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}

}

