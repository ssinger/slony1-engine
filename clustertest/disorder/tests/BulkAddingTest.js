coordinator.includeFile('disorder/tests/BasicTest.js');

BulkAddingTest=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='This test uses the bulk adding features to add'
		+ 'tables and sequences';
}
BulkAddingTest.prototype = new BasicTest();
BulkAddingTest.prototype.constructor = BulkAddingTest;

/**
 * 
 */
BulkAddingTest.prototype.getAddTableSlonikScript = function() {
	return this.tableAddCommands;
}

BulkAddingTest.prototype.runTest = function() {
	
	this.testResults.newGroup("Bulk Adding");
	
	
	
	//this.prepareDb(['db1','db2','db3','db4','db5']);
	this.setupReplication();
	
	/**
	 * 
	 *  before the slons are started we will 
	 *  add some tables on multiple event nodes
	 *  to confirm that slonik checks other nodes
	 *  for he next available id.
	 */
	this.tableAddCommands = "set add table(set id=1,tables='disorder.do_customer',add sequences=true);";
	this.addTables();
	
	this.validateNode1();
	
	/**
	 * create a second set (id=3) on node 2, add some tables there
	 */
	var slonikScript = 'create set(id=3, origin=2);\n ' 
		+ "set add table(set id=3,tables='disorder\\.do_[iro].*');\n"
		+ "set add sequence(set id=3,sequences='disorder\\.do_[iro].*');\n"
		+ "set add table(set id=3, tables='disorder\\.do_config*',add sequences=true);\n";
	var slonikPreamble = this.getSlonikPreamble();
	slonik = this.coordinator.createSlonik('add on node 2',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('second batch of adds okay',slonik.getReturnCode(),0);

	/**
	 * validate that all sequence and tables on node 2 are > 1
	 *  
	 */
	this.validateNode2();
	
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	
	/**
	 * Now subscribe node 1 to set 3.
	 */
	this.subscribeSet(3,2,2,[1]);
	java.lang.Thread.sleep(60*1000);
	/**
	 * move set 3 to node 1.
	 */
	this.moveSet(3,2,1);
	/**
	 * subscribe node 3 to set 1, so the subscriptions are the same.
	 */
	this.subscribeSet(1,1,1,[2]);

	/**
	 * merge the sets.
	 * 
	 * 
	 */
	slonikScript='merge set(id=1,add id=3,origin=1);\n' +
		'wait for event(origin=1,confirmed=all,wait on=1);\n';
	slonik = this.coordinator.createSlonik('merge set 1 and 3',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('merge set is okay',slonik.getReturnCode(),0);
	this.seedData(1);
	
	/**
	 * run some load through the system to make sure
	 * everything replicates as expected.
	 */
	var populate=this.generateLoad();
	this.subscribeSet(1,1,1,[3]);
	java.lang.Thread.sleep(3*1000);
	populate.stop();
	this.coordinator.join(populate);	
	this.slonikSync(1, 1);
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	slonikScript="drop set(id=1,origin=1);";
	slonik = this.coordinator.createSlonik('drop set 1',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop set okay',slonik.getReturnCode(),0);

	for(var idx=1; idx <= this.getNodeCount(); idx++) {		
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);
	}
	
	/**
	 * try a slonik script with an admin conninfo node
	 * to a database that does not exist. 
	 * 
	 * since slonik uses libpq we can declare a 
	 * unix socket style hostname to something
	 * that is unlikely to exist.
	 */
	slonikPreamble = this.getSlonikPreamble();
	slonikPreamble += "node 99 admin conninfo='dbname=none host=/tmp/no_socket';\n"
	slonikScript="create set(id=4, origin=1);\n" +
	"set add table(set id=4, fully qualified name='disorder.do_config');\n";
	slonik=this.coordinator.createSlonik('add table - bad admin node',
										 slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik detected a non-existant conninfo',
								 slonik.getReturnCode()!=0,true);
	

	/**
	 * try it with a conninfo that points to a database that does not
	 * have slony installed - yet.
	 */
	this.createDb(['db6']);
	this.getNodeCount=function() {
		return 6;
	}
	slonikPreamble=this.getSlonikPreamble();
	slonikScript="set add table(set id=4, fully qualified name='disorder.do_config');\n";
	slonik=this.coordinator.createSlonik('add table - node with slony not installed',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('add table worked - conninfo for a node without slony',slonik.getReturnCode(),0);

	/**
	 * try adding the table again via the bulk command.
	 * this should fail.
	 */
	slonikScript="set add table(set id=4, tables='disorder.do_config*');\n";
	slonik=this.coordinator.createSlonik('add table - duplicate',
										 slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('add table duplicate failed',
								 slonik.getReturnCode()!=0,true);

	this.dropDb(['db6']);


}




BulkAddingTest.prototype.validateNode1=function()
{
	var connection = this.coordinator.createJdbcConnection('db1');
	var statement = connection.createStatement();
	var rs=undefined;
	try {
		rs = statement.executeQuery('SELECT tab_id,tab_relname FROM _' + this.getClusterName()
				+ '.sl_table');
		var hasRow=rs.next();
		this.testResults.assertCheck('table added to sl_table',hasRow,true);
		if(hasRow) {
			this.testResults.assertCheck('first table was assigned tab_id=1',rs.getInt(1),1);
			this.testResults.assertCheck('only one table was added for do_customer',rs.next(),false);
		}		
		rs.close();
		rs = statement.executeQuery('SELECT seq_id,seq_relname FROM _' + this.getClusterName()
				+ '.sl_sequence');
		hasRow = rs.next();
		this.testResults.assertCheck('sequence added to sl_sequence',hasRow,true);
		if(hasRow) {
			this.testResults.assertCheck('first sequence was assigned seq_id=1',rs.getInt(1),1);
			this.testResults.assertCheck('only one sequence was added',rs.next(),false);
		}
		
	}
	catch(e) {
		this.testResults.assertCheck('error checking slony state on db1 ' + ':'+e,true,false);
	}
	finally {
		if(rs != undefined) {
			rs.close();
		}
		statement.close();
		connection.close();
	}

}	

BulkAddingTest.prototype.validateNode2=function()
{
	var connection = this.coordinator.createJdbcConnection('db2');
	var statement = connection.createStatement();
	var rs=undefined;
	try {
		rs = statement.executeQuery('SELECT tab_id,tab_relname FROM _' + this.getClusterName()
				+ '.sl_table order by tab_id');
		var hasRow=rs.next();
		this.testResults.assertCheck('table added to sl_table',hasRow,true);
		if(hasRow) {
			this.testResults.assertCheck('first table was assigned tab_id=2',rs.getInt(1),2);
			this.testResults.assertCheck('more than one table was added on node 2',rs.next(),true);
		}		
		rs.close();
		rs = statement.executeQuery('SELECT seq_id,seq_relname FROM _' + this.getClusterName()
				+ '.sl_sequence order by seq_id');
		hasRow = rs.next();
		this.testResults.assertCheck('sequence added to sl_sequence',hasRow,true);
		if(hasRow) {
			this.testResults.assertCheck('first sequence was assigned seq_id=2',rs.getInt(1),2);
			this.testResults.assertCheck('more than one sequence was added',rs.next(),true);
		}
		
	}
	catch(e) {
		this.testResults.assertCheck('error checking slony state on db2 ' + ':'+e,true,false);
	}
	finally {
		if(rs != undefined) {
			rs.close();
		}
		statement.close();
		connection.close();
	}

}	
