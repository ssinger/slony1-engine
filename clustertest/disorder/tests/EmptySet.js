coordinator.includeFile('disorder/tests/BasicTest.js');

EmptySet=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='This test subscribes an empty set. '
		+' The tests also merges two empty sets together ';
}
EmptySet.prototype = new BasicTest();
EmptySet.prototype.constructor = EmptySet;

EmptySet.prototype.runTest = function() {
	this.coordinator.log("EmptySet.prototype.runTest - begin");
	
	this.testResults.newGroup("Empty set");
	//this.prepareDb(['db1','db2','db3','db4','db5']);
	this.setupReplication();
	
	this.coordinator.log("EmptySet.prototype.runTest - start slons");
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	this.coordinator.log("EmptySet.prototype.runTest - subscribe empty set");
	/**
	 * Subscribe the empty set (we have not added anything).
	 */
	this.subscribeSet(1,1,1,[2,3]);
	if(this.testResults.getFailureCount()== 0) {
		//No apparent errors.
		this.subscribeSet(1,1,3,[4,5]);
		
	}
	
	
	this.coordinator.log("EmptySet.prototype.runTest - create second set");
	/**
	 * Create a second set
	 */
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'EmptySet.prototype.runTest\';\n';
	slonikScript += 'create set(id=2, origin=1);\n';
	slonik = this.coordinator.createSlonik('create set 2',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('second set created okay',slonik.getReturnCode(),0);
	
	this.coordinator.log("EmptySet.prototype.runTest - merge second set before subscription");
	/**
	 * Try merging the set.
	 * This SHOULD fail. set 1 and 2 have different subscribers. 
	 */
	slonikScript = 'merge set(id=1, ADD ID=2, ORIGIN=1);\n';
	slonik=this.coordinator.createSlonik('merge set try 1',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('merging unsubscribed set caused an error',slonik.getReturnCode()!=0,true);
	
	this.coordinator.log("EmptySet.prototype.runTest - subscribe second set");
	/**
	 * Subscribe the set(remember it is empty), then merge.
	 */
	slonikScript ='';
	for(var idx=2; idx <=3; idx++) {
		var forward='no';
		
		slonikScript+='subscribe set(id=2,provider=1,receiver='+idx+',forward=yes);\n'
			+ 'wait for event(origin=1,confirmed=all,wait on=1);\n' 
			+ 'sync(id=1);\n'
			+ 'wait for event(origin=1,confirmed=all,wait on=1);\n';
	}
	for(var idx=4; idx <=5; idx++) {
		slonikScript+='subscribe set(id=2,provider=3,receiver='+idx+',forward=yes);\n'
			+ 'wait for event(origin=1,confirmed=all,wait on=3);\n' 
			+ 'sync(id=3);\n'
			+ 'wait for event(origin=3,confirmed=all ,wait on=3);\n';
		slonikScript=='merge set(id=1, ADD ID=2, ORIGIN=1);\n';
	}
		
	slonik=this.coordinator.createSlonik('merge set',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	
	this.coordinator.log("EmptySet.prototype.runTest - merging second set");
	
	this.testResults.assertCheck('merging empty set',slonik.getReturnCode(),0);
	
	
	this.coordinator.log("EmptySet.prototype.runTest - test complete");
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
}
