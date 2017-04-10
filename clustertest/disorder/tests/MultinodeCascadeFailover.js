

coordinator.includeFile('disorder/tests/FailNodeTest.js');

MultinodeCascadeFailover = function(coordinator, testResults) {
	Failover.call(this, coordinator, testResults);
	this.testDescription='Test the FAILOVER command.  This test will try FAILOVER'
		+' with multiple nodes failing and cascading';
	this.compareQueryList.push(['select i_id,comments from disorder.do_item_review order by i_id','i_id']);
	this.nodeCount=5;
}
MultinodeCascadeFailover.prototype = new Failover();
MultinodeCascadeFailover.prototype.constructor = MultinodeCascadeFailover;

/**
 * Returns the number of nodes used in the test.
 */
MultinodeCascadeFailover.prototype.getNodeCount = function() {
	return this.nodeCount;
}

MultinodeCascadeFailover.prototype.runTest = function() {
    this.coordinator.log("MultinodeCascadeFailover.prototype.runTest - begin");
	this.testResults.newGroup("Multinode Cascade Fail Over Test");
	this.prepareDb(['db6']);
	this.setupReplication();

	this.addCompletePaths();
	/**
	 * Start the slons.
	 */
	this.slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		this.slonArray[idx - 1].run();
	}
	this.addCompletePaths();
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();

	/**
	 * Subscribe the first node.
	 *           1
	 *       2   3   4
	 *           |
	 *       5      6
	 */
	this.subscribeSet(1,1, 1, [ 2, 3,4 ]);
	this.subscribeSet(1,1, 3, [ 5, 6 ]);
	this.slonikSync(1,1);
	var load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	this.slonikSync(1,1);
	//stop slon 3, to make sure
	//3 and 5,6 aren't ahead of 2
	//If that happens nodes 5 might get unsubscribed
	//
	this.slonArray[3-1].stop();
	this.coordinator.join(this.slonArray[3-1]);
	this.failover(1,2,3,5);
	/**
	 * At the end of this we should have
	 *   2-->5-->6
	 *   |
	 *   4
	 */
	load.stop();
	this.coordinator.join(load);
	this.slonikSync(1,2);
	this.currentOrigin='db2';
	load=this.generateLoad();
	java.lang.Thread.sleep(1000*10);
	load.stop();	
	this.coordinator.join(load);
	this.slonikSync(1,2);
	this.compareDb('db2','db5');
	this.compareDb('db2','db6');
	this.compareDb('db2','db4');
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1].stop();
		this.coordinator.join(this.slonArray[idx - 1]);		
	}
	this.dropDb(['db6']);

	
}

MultinodeCascadeFailover.prototype.failover=function(originA,backupA,originB,backupB) 
{
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'MultinodeCascadeFailover.prototype.failover\';\n';
	slonikScript += 'FAILOVER( node=(id='  + originA  + ',backup node=' + backupA +')'
	+ ', node=(id=' + originB + ',backup node=' + backupB + '));\n';
	var slonik=this.coordinator.createSlonik('failover',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('failover passes',slonik.getReturnCode(),0);	

}

	MultinodeCascadeFailover.prototype.dropTwoNodes=function(node1,node2,event_node)
{
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'MultinodeCascadeFailover.prototype.dropTwoNodes\';\n';
	slonikScript+= 'drop node(id=\'' + node1 + ',' + node2 + '\',event node = ' + event_node + ');\nuninstall node(id='+node1+');\nuninstall node(id='+node2+');\n'

	var slonik=this.coordinator.createSlonik('drop node',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop 2 nodes passes',slonik.getReturnCode(),0);	

}

MultinodeCascadeFailover.prototype.getNodeCount = function() {
	return 6;
}
