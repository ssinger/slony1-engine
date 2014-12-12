/**
 * Some tests of the DROP PATH command.
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

DropNode=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='This test exercises the DROP Node command\n';		
}
DropNode.prototype = new BasicTest();
DropNode.prototype.constructor = DropNode;

DropNode.prototype.runTest = function() {
	this.testResults.newGroup("Drop Node");
	//this.prepareDb(['db1','db2','db3','db4','db5']);
	this.setupReplication();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	this.addTables();
	this.subscribeSet(1,1,1,[2,3]);
	if(this.testResults.getFailureCount()== 0) {
		//No apparent errors.
		this.subscribeSet(1,1,3,[4,5]);
		
	}
	this.dropNode(1);
	this.checkNodeDeleted(4,2);
	this.checkNodeDeleted(4,5);
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		
		slonArray[idx].stop();
		this.coordinator.join(slonArray[idx]);
	}
	
	
	
	
}

DropNode.prototype.dropNode=function(event_node) {
	this.coordinator.log('dropNode '   + event_node);
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript='echo \'DropNode.prototype.dropNode\';\n';
        slonikScript += "drop node(id='2,5',event node=" + event_node +");\n"
		+ 'wait for event(origin=' + event_node  + ',wait on=' + event_node 
		+ ',confirmed=all);\n';
	var slonik = this.coordinator.createSlonik('drop path', slonikPreamble, slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop path returned as expected',slonik.getReturnCode(),
								 0);
	
}


DropNode.prototype.checkNodeDeleted=function(client,droppedNode) {
	var connection = this.coordinator.createJdbcConnection('db' + client);
	
	var stat = connection.createStatement();
	var rs=undefined;
	try {
		rs = stat.executeQuery("SELECT COUNT(*) FROM _" + this.getClusterName()
				+ '.sl_node where no_id=' + droppedNode );
		rs.next();
		var count = rs.getInt(1);
		this.testResults.assertCheck('node still exists' + droppedNode + ' on  ' + client
				,count, 0);
	}
	finally {
		if(rs != undefined) {
			rs.close();
		}
		stat.close();
		connection.close();
	}
}