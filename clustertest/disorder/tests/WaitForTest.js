/**
 * Tests implicit WAIT FOR behaviour.
 *
 */
coordinator.includeFile('disorder/tests/BasicTest.js');

WaitForTest = function(coordinator, testResults) {
	ExecuteScript.call(this, coordinator, testResults);
	this.testDescription='Tests the implicit wait for behaviour';
}
WaitForTest.prototype = new ExecuteScript();
WaitForTest.prototype.constructor = WaitForTest;

WaitForTest.prototype.runTest = function() {
	this.coordinator.log("WaitForTest.prototype.runTest - begin");
	
	this.testResults.newGroup("WaitForTest");
	this.setupReplication();

	/**
	 * Start the slons.
	 */
	var slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx - 1].run();
	}
	this.slonikSync(1,1);
		for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1].stop();
		this.coordinator.join(slonArray[idx - 1]);		
	}

   /**
	* add a second replication set
	*/
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'BasicTest.prototype.createSecondSet\';\n';
	slonikScript += 'create set(id=2, origin=2 ,comment=\'second set\');\n'
	+"create set(id=3, origin=3 ,comment=\'second set\');\n "
	+ "set add table(set id=2,origin=2," +
	"  tables='disorder\.do_item_*');";
	this.tableIdCounter++;
	var slonik = this.coordinator.createSlonik('create second set',
											   slonikPreamble,
											   slonikScript);
	slonik.run();
	/**
	 * slonik should not finish, it should be waiting in an implicit
	 * wait for.
	 */
	java.lang.Thread.sleep(60*1000);
	this.testResults.assertCheck('slonik waiting',slonik.isFinished(),false);
	
		
	/**
	 * Start the slons.
	 **/ 
	var slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx - 1].run();
	}
	
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik finished okay',slonik.getReturnCode(),
							 0);
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1].stop();
		this.coordinator.join(slonArray[idx - 1]);		
	}
	
	this.coordinator.log("WiatForTest.prototype.runTest - complete");
}