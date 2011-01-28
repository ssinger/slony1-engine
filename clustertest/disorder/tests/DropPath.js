/**
 * Some tests of the DROP PATH command.
 *
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

DropPath=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='This test exercises the DROP PATH command\n'
		+ ' it issues DROP PATH commands and then queries the sl_path table\n'
		+ 'to verify that the path has been removed';
}
DropPath.prototype = new BasicTest();
DropPath.prototype.constructor = DropPath;

DropPath.prototype.runTest = function() {
	this.testResults.newGroup("Drop Path");
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
	
	/**
	 * Subscribe the empty set (we have not added anything).
	 */
	this.subscribeSet(1,1,1,[2,3]);
	if(this.testResults.getFailureCount()== 0) {
		//No apparent errors.
		this.subscribeSet(1,1,3,[4,5]);
		
	}
	
	/**
	 * Drop the path between node 1 and 3.
	 * It should fail, since a subscription requires the path.
	 */
	 this.dropPath(1,3,1,true);
	 this.verifyPath(1,3,true);
	
	 
	 /**
	  * Make sure paths exist.
	  */
	 this.addCompletePaths();
	 
	 /**
	  * DROP the path between nodes 4 and nodes 5.
	  */
	 this.dropPath(4,5,4,false);
	 this.verifyPath(4,5,false);

	 
	 
		
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		
		slonArray[idx].stop();
		this.coordinator.join(slonArray[idx]);
	}
	
	
	
	
}

DropPath.prototype.dropPath=function(server_id, client_id,event_node,expectFailure) {
	this.coordinator.log('dropPath ' + server_id + ',' + client_id + ',' + event_node);
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript='echo \'DropPath.prototype.dropPath\';\n';
        slonikScript += 'drop path(server=' + server_id + ',client = ' + client_id 
		+ ',event node=' + event_node + ');\n'
		+ 'wait for event(origin=' + event_node  + ',wait on=' + event_node 
		+ ',confirmed=all);\n';
	var slonik = this.coordinator.createSlonik('drop path', slonikPreamble, slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop path returned as expected',slonik.getReturnCode(),
			expectFailure ? 255 :  0);
	
}

/**
 *  Verify that no path exists on client DB that leads 
 *  client=client to server=server.
 *  
 *  expectPath: true if the path should exists or false if the path should not
 *              exist.
 */
DropPath.prototype.verifyPath=function(server,client,expectPath) {
	var connection = this.coordinator.createJdbcConnection('db' + client);
	
	var stat = connection.createStatement();
	var rs=undefined;
	try {
		rs = stat.executeQuery("SELECT COUNT(*) FROM _" + this.getClusterName()
				+ '.sl_path where pa_client=' + client + 'AND pa_server=' + server);
		rs.next();
		var count = rs.getInt(1);
		this.testResults.assertCheck('no path exists between ' + client + ' and ' + server
				,count,expectPath ? 1 : 0);
	}
	finally {
		if(rs != undefined) {
			rs.close();
		}
		stat.close();
		connection.close();
	}
}