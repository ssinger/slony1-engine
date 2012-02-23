/**
 * Tests a basic move set.
 */
coordinator.includeFile("disorder/tests/BasicTest.js");


function MoveSet(coordinator,results) {
	BasicTest.call(this,coordinator,results);
	this.syncWaitTime = 3 * 60;
	this.testDescription = 'This test exercises the move set command \n'
		+ ' it does this by taking set 1 and moving it between different \n'
		+ ' combinations of old origin/new origin.  After each move it verifies\n'
		+ ' that the old origin is read only.  It also runs transactions\n ' 
		+ ' against the new origin and verifies that they replicate back to\n' 
		+ ' the old origin.';
}

MoveSet.prototype = new BasicTest();
MoveSet.prototype.constructor=MoveSet;
MoveSet.prototype.getNodeCount = function() {
	return 5;
}

MoveSet.prototype.runTest = function() {
        this.coordinator.log("MoveSet.prototype.runTest - begin");

	this.testResults.newGroup("move set1");
	//this.prepareDb(['db1','db2']);

	
//First setup slony
        this.coordinator.log("MoveSet.prototype.runTest - set up replication");
	this.setupReplication();
	this.addCompletePaths();
	this.addTables();
	
        this.coordinator.log("MoveSet.prototype.runTest - start slons");
	//Start the slons.
	//These must be started before slonik runs or the subscribe won't happen
	//thus slonik won't finish.
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	
	this.subscribeSet(1,1,1,[2,3]);
	this.subscribeSet(1,1,3,[4,5]);
	
        this.coordinator.log("MoveSet.prototype.runTest - subscriptions complete");

	
	this.syncWaitTime = 60*5;
	this.slonikSync("1","1");
	this.syncWaitTime= 3 * 60;
        this.coordinator.log("MoveSet.prototype.runTest - sets subscribed, data synced");
	var pairings=[
	              [1,2]
	              ,[2,3]
	              ,[3,1]
	              ,[1,3]
	              ,[3,4]
	              ,[4,5]
	              ,[5,2]
	              ,[2,4]
	              ,[4,1]
	              ,[1,5]
	              ];
	              
	for(var idx = 0; idx < pairings.length; idx++) {
		var curMoveNodes=pairings[idx];
                this.coordinator.log("MoveSet.prototype.runTest - moving set from " + curMoveNodes[0] + ' to ' + 
				curMoveNodes[1]);
		var moveResult=this.moveSet(1,curMoveNodes[0],curMoveNodes[1])
		
		//Start up some load.
		this.currentOrigin='db' +curMoveNodes[1];
		var load = this.generateLoad();
		java.lang.Thread.sleep(5*1000);
		this.slonikSync(1,curMoveNodes[1]);
		this.slonikSync(1,curMoveNodes[0]);
		//Make sure that db1 is read only.
		
		this.verifyReadOnly(curMoveNodes[0]);
                this.coordinator.log("MoveSet.prototype.runTest - verification done");
		load.stop();
                this.coordinator.log("MoveSet.prototype.runTest - joining after load");
		this.coordinator.join(load);
                this.coordinator.log("MoveSet.prototype.runTest - syncing after load");
		this.slonikSync(1,curMoveNodes[1]);
                this.coordinator.log("MoveSet.prototype.runTest - syncing complete");
		if(moveResult==0) {			
			this.compareDb('db' + curMoveNodes[0],'db' + curMoveNodes[1]);
		}
	}
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
        this.coordinator.log("MoveSet.prototype.runTest - complete");
}

MoveSet.prototype.getSyncWaitTime = function () {
	return this.syncWaitTime;
}
