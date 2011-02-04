coordinator.includeFile('disorder/tests/BasicTest.js');

AddPathsAfterSubscribe=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription = 'This test sets up the cluster minus paths then \n'
		+ 'starts the slon daemons.  It then adds the  paths requried for\n'
		+ 'subscriptions  and subscribes.';
}
AddPathsAfterSubscribe.prototype = new BasicTest();
AddPathsAfterSubscribe.prototype.constructor = EmptySet;




AddPathsAfterSubscribe.prototype.runTest = function() {


	this.testResults.newGroup("Add Paths After Subscribe");
	//Note, we are calling the overloaded version of setupReplication
	this.setupReplication();
	this.addTables();
	
	//Start the slons.
	//These must be started before slonik runs or the subscribe won't happen
	//thus slonik won't finish.
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}


	//Now add the Paths.
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript='echo \'AddPathsAfterSubscribe.prototype.runTest\';\n';
	for(var serverId=1; serverId <= this.getNodeCount(); serverId++) {
		for(var clientId=1; clientId <= this.getNodeCount(); clientId++) {
			if(serverId!=clientId) {
				slonikScript +='store path(server=' + serverId + ',client=' + clientId 
					+', conninfo=@CONNINFO' + serverId + ');\n'; 
			}
			
		}
	}
	var slonik=this.coordinator.createSlonik('add paths',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.subscribeSet(1,1,1,[2,3]);
	this.subscribeSet(1,1,3,[4,5]);
	

	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
	
}

AddPathsAfterSubscribe.prototype.setupReplication = function() {
	
	var result = 0;
	var slonikPre = this.getSlonikPreamble();
	var slonikScript='echo \'AddPathsAfterSubscribe.prototype.setupReplication\';\n';
	// slonikScript += 'sleep(seconds=60);'
	slonikScript += 'init cluster(id=1);\n';

	for ( var idx = 2; idx <= this.getNodeCount(); idx++) {
		slonikScript += 'store node(id=' + idx + ',event node=1);\n';
	}
	slonikScript += ' create set(id=1,origin=1);\n';
	var slonik = this.coordinator.createSlonik('init', slonikPre, slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('initialize succeeded',slonik.getReturnCode(),0);
	
}