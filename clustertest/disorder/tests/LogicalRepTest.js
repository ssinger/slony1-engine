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
	return 3;
}

LogicalRepTest.prototype.isLogical=function(node_id) {
	if (node_id == 1) {
		return true;		
	}
	return true;
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
	this.currentOrigin='db1';
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


	seeding=this.generateLoad();
	java.lang.Thread.sleep(60*1000);
	seeding.stop();
	this.coordinator.join(seeding);
	
	this.slonikSync(1,1);


    this.compareDb('db1', 'db2');
    this.moveSet(1,1,2);	
	this.currentOrigin='db2';

	this.subscribeSet(1,2, 1, [3]);
	
    seeding=this.generateLoad();
    java.lang.Thread.sleep(60*1000);
    seeding.stop();

    this.coordinator.join(seeding);
    this.slonikSync(1,2);

	java.lang.Thread.sleep(10*1000);
	this.compareDb('db1', 'db2');
    this.compareDb('db1','db3');

	


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
    this.compareDb('db1','db3');

	
	

}

LogicalRepTest.prototype.executeTest=function() {
	/**
	 * use execute script.
	 */
	var slonikScript = 'execute script(event node=1, SQL=\'create table disorder.test_table_a(a int4 primary key);\');\n';
	var slonikPreamble = this.getSlonikPreamble();
	var slonik = this.coordinator.createSlonik('execute script - add ', slonikPreamble,
			slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	exit(-1);
	this.testResults.assertCheck('slonik executed add table okay', slonik
			.getReturnCode(), 0);
	this.slonikSync(1,1);
	slonikScript = 'execute script(event node=2, SQL=\'drop table disorder.test_table_a;\');\n';
	slonik = this.coordinator.createSlonik('execute script - drop', slonikPreamble,
			slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik executed drop table okay', slonik
			.getReturnCode(), 0);	
	this.slonikSync(1,2);
}