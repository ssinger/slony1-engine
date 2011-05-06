/**
 * Tests a basic merge set.
 */
coordinator.includeFile("disorder/tests/BasicTest.js");


function MergeSet(coordinator,results) {
	BasicTest.call(this,coordinator,results);
	this.syncWaitTime = 60;
	this.testDescription = 'This test exercises the merge set command \n';
}

MergeSet.prototype = new BasicTest();
MergeSet.prototype.constructor=MergeSet;
MergeSet.prototype.getNodeCount = function() {
	return 5;
}

MergeSet.prototype.runTest = function() {
        this.coordinator.log("MergeSet.prototype.runTest - begin");

	this.testResults.newGroup("merge set");
	//this.prepareDb(['db1','db2']);

	
//First setup slony
        this.coordinator.log("MergeSet.prototype.runTest - set up replication");
	this.setupReplication();
	this.addCompletePaths();
	this.addTables();
	
        this.coordinator.log("MergeSet.prototype.runTest - start slons");
	//Start the slons.
	//These must be started before slonik runs or the subscribe won't happen
	//thus slonik won't finish.
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	this.createSecondSet(1);
	var load = this.generateLoad();
	java.lang.Thread.sleep(5*1000);
	this.subscribeSet(1,1,1,[2,3]);
	this.subscribeSet(1,1,3,[4,5]);
	this.subscribeSet(2,1,1,[2,3]);
	this.subscribeSet(2,1,3,[4,5]);
	var mergeScript="merge set(id=1,add id=2,origin=1);";
	var slonikPreamble=this.getSlonikPreamble();
	var slonik = this.coordinator.createSlonik('merge set',slonikPreamble,mergeScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('merge set okay',slonik.getReturnCode(),
				     0);

	load.stop();
	this.coordinator.join(load);
        this.coordinator.log("MergeSet.prototype.runTest - subscriptions complete");
	this.slonikSync(1,1);
	this.coordinator.log("MergeSet.prototype.runTest - syncing complete");
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db1','db5');
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
        this.coordinator.log("MergeSet.prototype.runTest - complete");
}

MergeSet.prototype.getSyncWaitTime = function () {
	return this.syncWaitTime;
}
