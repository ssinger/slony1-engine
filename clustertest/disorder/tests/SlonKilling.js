/**
 * This test involves killing the slon processes at random times during the
 * subscription, replication and move sets.
 * 
 * 
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

SlonKilling=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Slon processes are killed at random'
	+ ' intervals.';
}
SlonKilling.prototype = new BasicTest();
SlonKilling.prototype.constructor = SlonKilling;

SlonKilling.prototype.runTest = function() {
        this.coordinator.log("SlonKilling.prototype.runTest - begin");
	
	this.testResults.newGroup("Slon killing");
	for(var idx=0; idx < 10; idx++) {
		this.coordinator.log('starting iteration ' + idx);
		this.testActions();
		this.teardownSlony();
	}
        this.coordinator.log("SlonKilling.prototype.runTest - compare db1,2,3,4,5");
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db1','db5');
	
        this.coordinator.log("SlonKilling.prototype.runTest - complete");
}

SlonKilling.prototype.testActions=function() {
        this.coordinator.log("SlonKilling.prototype.testActions - begin");
	
	this.setupReplication();
	this.addTables();
	/**
	 * Start the slons.
	 */
        this.coordinator.log("SlonKilling.prototype.testActions - start slons");
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
        this.coordinator.log("SlonKilling.prototype.testActions - subscribe sets");
	//Now subscribe the sets in the background
	this.addCompletePaths();
	var slonikList = this.subscribeSetBackground(1,1,1,[2,3,4,5]);
	var load=this.generateLoad();
	for(var idx=0; idx < slonikList.length; idx ++)  {
		slonikList[idx].run();
	}
	var random = new java.util.Random();
	var sleepTime = random.nextInt(60);
        this.coordinator.log("SlonKilling.prototype.testActions - sleeping for " + sleepTime + ' seconds before killing a slon');
	java.lang.Thread.sleep(sleepTime);
	var slonToKill = random.nextInt(slonArray.length-1);
	slonArray[slonToKill].stop();
	this.coordinator.join(slonArray[slonToKill]);
	//Now wait a while
	sleepTime = random.nextInt(60);
	java.lang.Thread.sleep(sleepTime);
	slonArray[slonToKill] = this.coordinator.createSlonLauncher('db' + (slonToKill+1));
	slonArray[slonToKill].run();
	for(var idx = 0; idx < slonikList.length; idx++) {
		this.coordinator.join(slonikList[idx]);
	}
	load.stop();
	this.coordinator.join(load);
        this.coordinator.log("SlonKilling.prototype.testActions - sync");
	this.slonikSync(1,1,60*5);
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
	
        this.coordinator.log("SlonKilling.prototype.testActions - complete");
}
