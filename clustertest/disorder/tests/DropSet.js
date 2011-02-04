/**
 * Some tests of the DROP SET command.
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

DropSet=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription = 'This test exercises the DROP SET command.\n'
		+ ' It includes testing of DROP SET while a subsription to that set is\n'
		'possibly in progress.'
}
DropSet.prototype = new BasicTest();
DropSet.prototype.constructor = DropSet;


DropSet.prototype.runTest = function() {
	this.testResults.newGroup("Drop Set");
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	/**
	 * Subscribe the empty set (we have not added anything).
	 */
	this.subscribeSet(1,1,1,[2]);
	if(this.testResults.getFailureCount()== 0) {
		//No apparent errors.

		
	}
	
	this.testDropConcurrentSubscribe();
		
	
	
	//this.testStalledReplication();
	
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		
		slonArray[idx].stop();
		this.coordinator.join(slonArray[idx]);
	}
		

}



/**
 *  This test will setup set 2 to replicate form 1=>3
 *  It will then concurrently issue a subscribe 3=>4 and drop set(origin=1).
 *  
 *  Note currently this test triggers bug #133
 *  
 */
DropSet.prototype.testDropConcurrentSubscribe=function() {
	this.createSecondSet(1);
	this.subscribeSet(2,1,1,[3]);
	var subscribeArray = this.subscribeSetBackground(2,1,3,[4] );
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript='echo \'DropSet.prototype.testDropConcurrentSubscribe\';\n';
	slonikScript += 'drop set (id=2, origin=1);\n'
		+ 'wait for event(origin=1, confirmed=3,wait on=1);\n';
	var slonik=this.coordinator.createSlonik('drop set 2',slonikPreamble,slonikScript);
	
	slonik.run();
	subscribeArray[0].run();
	
	this.coordinator.join(slonik);
	this.coordinator.join(subscribeArray[0]);
	
	this.testResults.assertCheck('drop set succeeded',slonik.getReturnCode(),0);
	this.testResults.assertCheck('subscribe set succeeded',subscribeArray[0].getReturnCode(),0);
	
}