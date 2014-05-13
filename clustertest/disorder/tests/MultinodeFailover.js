

coordinator.includeFile('disorder/tests/FailNodeTest.js');

MultinodeFailover = function(coordinator, testResults) {
	Failover.call(this, coordinator, testResults);
	this.testDescription='Test the FAILOVER command.  This test will try FAILOVER'
		+' with multiple nodes failing';
	this.compareQueryList.push(['select i_id,comments from disorder.do_item_review order by i_id','i_id']);
	this.nodeCount=5;
}
MultinodeFailover.prototype = new Failover();
MultinodeFailover.prototype.constructor = MultinodeFailover;

/**
 * Returns the number of nodes used in the test.
 */
MultinodeFailover.prototype.getNodeCount = function() {
	return this.nodeCount;
}

MultinodeFailover.prototype.runTest = function() {
    this.coordinator.log("MultinodeFailover.prototype.runTest - begin");
	this.testResults.newGroup("Multinode Fail Over Test");
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
	 */
	this.subscribeSet(1,1, 1, [ 2, 3 ]);
	this.subscribeSet(1,1, 3, [ 4, 5 ]);
	this.slonikSync(1,1);
	this.createSecondSet(2);
	this.subscribeSet(2,2,2,[3,4,5]);
	this.slonikSync(2,2);	
	var load = this.generateLoad();
	java.lang.Thread.sleep(10*1000);
	this.slonikSync(1,1);
	this.populateReviewTable(2);
	/**
	 * make sure the _review data makes it to
	 * all slaves, then let some SYNC events get
	 * genereated.  Next we FAILOVER.
	 */
	this.slonikSync(2,2);
	java.lang.Thread.sleep(10*1000);
	this.failover(1,3,2,3);		
	load.stop();
	this.coordinator.join(load);
	/**
	 * rebuild the nodes.
	 */
	this.dropTwoNodes(1,2,3);
	this.slonikSync(1,3);
	this.compareDb('db3','db4');
	this.compareDb('db3','db5');
	this.reAddNode(1,3,3);		

	/**
	 * perform some updates on node3 to the review table
	 */
	this.updateReviewTable(3,'From node 3');
	this.moveSet(1,3,1);
	
	this.reAddNode(2,1,1);
	this.addCompletePaths();
	this.subscribeSet(2,3,3,[2]);
	this.moveSet(2,3,2);


	/**
	 * generate some load (node1) and
	 * reviews on node2. Let multiple txn snapshots be
	 * generated.
	 */
	load=this.generateLoad();
	for(var idx=0; idx < 20; idx++)
	{
		this.updateReviewTable(2,'From node 2.' + idx);
		java.lang.Thread.sleep(1000);
	}
	/**
	 * failover.  Node 1=>3, node2=>4
	 */
	this.failover(1,3,2,4);
	load.stop(); 
	this.coordinator.join(load);
	this.dropTwoNodes(1,2,3);
	this.slonikSync(2,4);
	this.compareDb('db3','db4');
		//exit(-1);
	//auto wait for should not require the
		//sync but we have a race condition.
		//		this.slonikSync(1,3);
		//	this.slonikSync(1,4);
	this.reAddNode(1,3,3);		
	this.reAddNode(2,3,3);
	this.addCompletePaths();
	this.subscribeSet(1,3,3,[1,2]);
	this.subscribeSet(2,4,4,[2,1]);
	this.slonikSync(1,1);	
	this.moveSet(1,3,1);
	this.moveSet(2,4,2);


	this.slonikSync(1,1);
	this.slonikSync(2,2);
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1].stop();
		this.coordinator.join(this.slonArray[idx - 1]);
	}		
	this.compareDb('db1','db3');
	this.compareDb('db2','db3');
	this.compareDb('db3','db4');
	this.compareDb('db3','db5');
	
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		this.slonArray[idx - 1].run();
	}
	
    /**
	 * Now configure the cluster as 
	 * 1 <---- 2 ----> 3 ----> 4
	 */
    this.moveSet(1,1,2);
	this.dropNode(5,1);
	this.nodeCount=4;
	var resubscribeSlonik = 'resubscribe node(origin=2,provider=2,receiver=1);\n'
		+ 'resubscribe node(origin=2,provider=2,receiver=3);\n'
	    + 'resubscribe node(origin=2,provider=3,receiver=4);\n';
	var slonikPreamble = this.getSlonikPreamble();
	var slonik=this.coordinator.createSlonik('failover',slonikPreamble,resubscribeSlonik);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('failover passes',slonik.getReturnCode(),0);	
	this.currentOrigin='db2';
	load=this.generateLoad();
	java.lang.Thread.sleep(1000);
	/**
	 * failover.  Node 2=>1, node3=>1
	 */
	this.failover(2,1,3,1);
	load.stop(); 
	this.coordinator.join(load);
	this.slonikSync(1,1);
	java.lang.Thread.sleep(1000);
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1].stop();
		this.coordinator.join(this.slonArray[idx - 1]);
	}	
	this.compareDb('db1','db4');
}

MultinodeFailover.prototype.failover=function(originA,backupA,originB,backupB) 
{
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'MultinodeFailover.prototype.failover\';\n';
	slonikScript += 'FAILOVER( node=(id='  + originA  + ',backup node=' + backupA +')'
	+ ', node=(id=' + originB + ',backup node=' + backupB + '));\n';
	var slonik=this.coordinator.createSlonik('failover',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('failover passes',slonik.getReturnCode(),0);	

}

	MultinodeFailover.prototype.dropTwoNodes=function(node1,node2,event_node)
{
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'MultinodeFailover.prototype.dropTwoNodes\';\n';
	slonikScript+= 'drop node(id=\'' + node1 + ',' + node2 + '\',event node = ' + event_node + ');\nuninstall node(id='+node1+');\nuninstall node(id='+node2+');\n'

	var slonik=this.coordinator.createSlonik('drop node',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop 2 nodes passes',slonik.getReturnCode(),0);	

}
