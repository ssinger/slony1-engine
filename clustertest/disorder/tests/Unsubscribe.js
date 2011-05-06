/**
 * Tests the UNSUBSCRIBE command.
 *
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');


Unsubscribe=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Tests the UNSUBSCRIBE command';
}
Unsubscribe.prototype = new BasicTest();
Unsubscribe.prototype.constructor = Unsubscribe;

Unsubscribe.prototype.runTest = function() {
        this.coordinator.log("Unsubscribe.prototype.runTest - begin");	
	this.testResults.newGroup("Unsubscribe");
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
	this.subscribeSet(1, 1,1, [2,3]);
	this.subscribeSet(1, 1,3, [4,5]);
	
	var checkProcesses=[];
	for(var idx=1; idx <=this.getNodeCount(); idx++) {
		checkProcesses[idx-1] = this.startDataChecks(idx);
	}
	this.coordinator.log('sleeping while load is being applied');
	java.lang.Thread.sleep(10*1000);
	
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
	this.coordinator.log('validating the databases are the same');
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	this.compareDb('db1', 'db5');

	
	/**
	 * Now we unsubscribe node 2
	 * It should work.
	 */
	this.unsubscribe(2,1,true);
	
	/**
	 * unsubscribe node 3,
	 * it should fail since 3 is a forwarder to 4,5
	 */
	this.unsubscribe(3,1,false);
	
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
        this.coordinator.log("Unsubscribe.prototype.runTest - complete");	
}

Unsubscribe.prototype.unsubscribe=function(node_id,set_id,expect_success) {
        this.coordinator.log("Unsubscribe.prototype.unsubscribe - begin");	
	var slonikPreamble = this.getSlonikPreamble();
        var slonikScript = 'echo \'Unsubscribe.prototype.unsubscribe\';\n';
        slonikScript +='unsubscribe set(id=' + set_id + ',receiver=' + node_id + ');\n';

	var slonik = this.coordinator.createSlonik('unsubscribe ' , slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck("unsubscribe node " + node_id,slonik.getReturnCode()==0,expect_success);
        this.coordinator.log("Unsubscribe.prototype.unsubscribe - complete");	
}
