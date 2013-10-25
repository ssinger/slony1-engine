/**
 * Tests the EXECUTE SCRIPT command.
 * 
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

ExecuteScript = function(coordinator, testResults) {
	BasicTest.call(this, coordinator, testResults);
	this.testDescription='This test uses the EXECUTE SCRIPT functions'
		+ ' it calls execute script on ddl that works, ddl that fails'
		+ ' nodes using the ONLY ON feature';
}
ExecuteScript.prototype = new BasicTest();
ExecuteScript.prototype.constructor = ExecuteScript;

ExecuteScript.prototype.runTest = function() {

	this.coordinator.log("ExecuteScript.prototype.runTest - begin");
	this.testResults.newGroup("Execute Script");
	this.setupReplication();

	/**
	 * Start the slons.
	 */
	this.coordinator.log("ExecuteScript.prototype.runTest - start slons");
	var slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx - 1].run();
	}

	this.coordinator.log("ExecuteScript.prototype.runTest - add tables");
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();

	this.coordinator.log("ExecuteScript.prototype.runTest - subscribe first set");
	/**
	 * Subscribe the first node.
	 */
	this.subscribeSet(1, 1,1, [ 2, 3 ]);
	this.subscribeSet(1, 1,3, [ 4, 5 ]);
	this.slonikSync(1,1);

	this.testAddDropColumn(1, false,false);

	this.coordinator.log("ExecuteScript.prototype.runTest - subscribe first set");
	/**
	 * Now create a second replication set.
	 * 
	 */
	this.createSecondSet(2);

	/**
	 * Replicate the set to nodes 1 and 3.
	 */
	this.subscribeSet(2,2, 2, [ 1 ]);
	this.subscribeSet(2,2, 1, [ 3 ]);
	/**
	 * Now send it to node 4,5
	 */
	this.subscribeSet(2, 2,3, [ 4, 5 ]);

	this.coordinator.log("ExecuteScript.prototype.runTest - move set to node 1");


	/**
	 * Move the set to node 1. We want to do this for the next test.
	 */
	this.moveSet(2, 2, 1);

	/**
	 * Now try the setAddDrop execute scripts again. This time we tell slonik we
	 * are doing the DDL on set 2 even though the table it changes is part of
	 * set 1
	 */
	
	/**
	 * First use node 2 as the event node. 
	 * Since set 2 has set 1 as the origin (we just moved it above)
	 * we EXPECT this to fail.
	 */
	//this.testAddDropColumn(2, true,false);
	//exit(-1);
	
	/**
	 * Now try it against node 1
	 */
	this.testAddDropColumn(1, false,false);
	
	/**
	 * Try it again but batching drop statements with ONLY ON
	 */
	this.testAddDropColumn( 1, false,true);

	/**
	 * Move it back to node 2.
	 */
	this.moveSet(2, 1, 2);

	
	/**
	 * DDL failure tests
	 */
	this.testDDLFailure();
	
	
	/**
	 * Now create a table and add it to set 1.
	 * 
	 */
	this.createAddTestTable();

	
	
	/**
	 * Now we generate READ only loads on all nodes.
	 * We then start doing execute scripts. 
	 */
	this.coordinator.log("ExecuteScript.prototype.runTest - starting execute script with read queries on all nodes");
	var readLoad=[];
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		readLoad[idx] = this.startDataChecks(idx+1);		
	};
	for(var iteration=0; iteration < 100; iteration++) {
		if(iteration % 10 ==0) {
			this.coordinator.log("execute script with read load, iteration " + iteration);
			//Drop the replication set (3) created before.
			this.dropSet3(3);
			//Perform the DDL.
			this.createAddTestTable();
		}
	}	
	this.coordinator.log('stopping read queries');
	for(var idx=0; idx < this.getNodeCount(); idx++) {
		readLoad[idx].stop();
		this.coordinator.join(readLoad[idx]);
	}
	
	this.performInsert(1);
	
	
	
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1].stop();
		this.coordinator.join(slonArray[idx - 1]);		
	}
	this.coordinator.log("ExecuteScript.prototype.runTest - complete");

}

ExecuteScript.prototype.validateDdl = function(dbname) {
	var connection = this.coordinator.createJdbcConnection(dbname);
	var statement = connection.createStatement();
	var rs;
	try {
		rs = statement
				.executeQuery('SELECT MAX(testcolumn) FROM disorder.do_order');
		rs.next();

		return true;
	} catch (e) {
		return false;
	} finally {
		if (rs != undefined) {
			rs.close();
		}
		statement.close();
		connection.close();

	}

}
ExecuteScript.prototype.testAddDropColumn = function(eventNode,
													 expectFailure,
													 batchOnlyOn) {
	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - begin");
	/**
	 * start up some load.
	 */
	var load = this.generateLoad();

	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - add column to orders - expecting failure:" + expectFailure);
	/**
	 * Now add a column to orders. We will do this via EXECUTE SCRIPT.
	 */

	var scriptFile = java.io.File.createTempFile('executeScript', '.sql');
	scriptFile.deleteOnExit();
	var fileWriter = new java.io.FileWriter(scriptFile);
	fileWriter
			.write('ALTER TABLE disorder.do_order ADD COLUMN testcolumn int4 default 7;');
	fileWriter.close();
        var slonikScript = 'echo \'ExecuteScript.prototype.testAddDropColumn\';\n';
    
	slonikScript += 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile.getAbsolutePath() + '\' , EVENT NODE=' + eventNode
			+ ' );\n' + 'SYNC(id=' + eventNode + ');\n'
			+ 'wait for event(origin=' + eventNode
			+ ', confirmed=all, wait on=' + eventNode + ');\n';

	var slonikPreamble = this.getSlonikPreamble();
	var slonik = this.coordinator.createSlonik('add testcolumn',
			slonikPreamble, slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	if (!expectFailure) {
		this.testResults.assertCheck('execute script for add column okay',
				slonik.getReturnCode(), 0);

		this.testResults.assertCheck('column was added', this
				.validateDdl('db1'), true);
		this.testResults.assertCheck('column was added', this
				.validateDdl('db2'), true);
		this.testResults.assertCheck('column was added', this
				.validateDdl('db4'), true);
	} else {
		this.testResults.assertCheck(
				'execute script for add column failed as expected', slonik
						.getReturnCode() != 0, expectFailure);
		this.testResults.assertCheck('column was added', this
				.validateDdl('db1'), false);
		this.testResults.assertCheck('column was added', this
				.validateDdl('db2'), false);
		this.testResults.assertCheck('column was added', this
				.validateDdl('db4'), false);
		load.stop();
		this.coordinator.join(load);
		return;
	}

	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - terminate load");
	load.stop();
	this.coordinator.join(load);

	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - synchronize");
	this.slonikSync(1, 1);
	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - compare databases");
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');

	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - execute ONLY ON");
	// Now execute ONLY ON.
	// We DROP the column, on the master first then moving out.
	load = this.generateLoad();

	
	var scriptFile_drop = java.io.File.createTempFile('executeScript', '.sql');
	scriptFile_drop.deleteOnExit();
	fileWriter = new java.io.FileWriter(scriptFile_drop);
	fileWriter.write('ALTER TABLE disorder.do_order DROP COLUMN testcolumn;');
	fileWriter.close();
	if ( batchOnlyOn) 
	{
			slonikScript = 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile_drop.getAbsolutePath() + '\' , EVENT NODE=' + eventNode
				+ ', EXECUTE ONLY ON=\'1';

			for(var idx=2; idx <=5; idx++)
			{
				slonikScript+="," + idx;
			}
			slonikScript+= "');";
			slonik = this.coordinator.createSlonik(' test drop column',
												   slonikPreamble, slonikScript);
			slonik.run();
			this.coordinator.join(slonik);
			this.testResults.assertCheck('execute script for drop column okay',
										 slonik.getReturnCode(), 0);
			// Sync all nodes.
			
			this.slonikSync(1, 1);
	}
	else
	{
		for ( var idx = 1; idx <= 5; idx++) {
			
			slonikScript = 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile_drop.getAbsolutePath() + '\' , EVENT NODE=' + idx
			+ ', EXECUTE ONLY ON=' + idx + ');';
			
			slonik = this.coordinator.createSlonik(' test drop column',
												   slonikPreamble, slonikScript);
			slonik.run();
			this.coordinator.join(slonik);
			this.testResults.assertCheck('execute script for drop column okay',
										 slonik.getReturnCode(), 0);
			// Sync all nodes.
			
			this.slonikSync(1, 1);

		}
	}

	load.stop();
	this.coordinator.join(load);
	this.slonikSync(1, 1);
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	
	this.coordinator.log("ExecuteScript.prototype.testAddDropColumn - complete");
}

ExecuteScript.prototype.createAndReplicateTestTable=function() {
	this.coordinator.log("ExecuteScript.prototype.createAndReplicateTestTable - begin");
	var scriptFile = java.io.File.createTempFile('executeScript', '.sql');
	scriptFile.deleteOnExit();
	fileWriter = new java.io.FileWriter(scriptFile);
	fileWriter
			.write('CREATE TABLE disorder.test_transient(id serial,data text, primary key(id));');
	fileWriter.close();
        var slonikScript = 'echo \'ExecuteScript.prototype.createAndReplicateTestTable\';\n';
	slonikScript += 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile.getAbsolutePath() + '\' , EVENT NODE=1 );\n'
			+ 'SYNC(id=1);\n'
			+ 'wait for event(origin=1, confirmed=all, wait on=1);\n'
			+ 'create set(id=3, origin=1);\n'
			+ 'set add table(origin=1,set id=3, id=' + this.tableIdCounter
			+ ',fully qualified name=\'disorder.test_transient\');\n';
			
	var tempTableId=this.tableIdCounter;
	this.tableIdCounter++;
	var slonikPreamble = this.getSlonikPreamble();
	var slonik = this.coordinator.createSlonik('add temp table',
			slonikPreamble, slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('CREATE TABLE worked', slonik.getReturnCode(),
			0);
	//
	// Explicitly leave node 2 out of this.
	this.subscribeSet(3,1, 1, [ 3 ]);
	this.subscribeSet(3,1, 3, [ 4, 5 ]);
	this.coordinator.log("ExecuteScript.prototype.createAndReplicateTestTable - complete");
}

ExecuteScript.prototype.createAddTestTable = function() {
	this.coordinator.log("ExecuteScript.prototype.createAddTestTable - begin");
	this.coordinator.log('createAddTestTable');
	this.createAndReplicateTestTable();
	
	// Now we DROP the table.
	this.dropTestTable(1,3,true);
	// Did the table get dropped from node 2?
	// It should have because as of 2.2 DDL goes against nodes
	// that are not subscribers to a particular set.
	var connection = this.coordinator.createJdbcConnection('db2');
	var statement = connection.createStatement();

	var rs = statement
			.executeQuery('SELECT COUNT(*) FROM pg_tables WHERE tablename=\'test_transient\';');
	rs.next();
	this.testResults.assertCheck('test_transient did get deleted from db2',
			rs.getInt(1), 0);
	if (rs.getInt(1) > 0) {
		statement.execute('DROP TABLE disorder.test_transient;');
	}
	statement.close();
	connection.close();
	/**
	 * Now move set 3 from node 1 to 3
	 */
	//slonikScript='set drop table(id='+ tempTableId + ',origin=1);\n';
	//slonik=this.coordinator.createSlonik('drop removed table',slonikPreamble,slonikScript);
	//slonik.run();
	//this.coordinator.join(slonik);
	//this.testResults.assertCheck('drop removed table worked okay',slonik.getReturnCode(),0);
	this.moveSet(3, 1, 3);
	this.coordinator.log("ExecuteScript.prototype.createAddTestTable - complete"); 
}

/**
 * Tests executing a DDL script that will fail.
 * 
 * First 1 test the script such that it will fail on the EVENT NODE.
 * 
 * This should fail at the slonik stage.
 * 
 * Next we test a DDL script that is excepted on the event node but fails at a later
 * node.   This should stop replication. (well that is what we expect it to do
 * what it 'should' do is open to debate).
 * 
 */
ExecuteScript.prototype.testDDLFailure = function() {
	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - begin");
	var scriptFile = java.io.File.createTempFile('executeScript', '.sql');
	scriptFile.deleteOnExit();
	fileWriter = new java.io.FileWriter(scriptFile);
	fileWriter.write('CREATE TABLE disorder.test_transient(id serial,data text, primary key(id));');
	fileWriter.close();
	
	var connection = this.coordinator.createJdbcConnection('db2');
	var statement = connection.createStatement();
	statement.execute('CREATE TABLE disorder.test_transient(id int4, data text,primary key(id));');
	
	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - failure expected on node 2");
	/**
	 * First execute the script against node 2, it should fail.
	 */
        var slonikScript = 'echo \'ExecuteScript.prototype.testDDLFailure\';\n';
	slonikScript += 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile.getAbsolutePath() + '\' , EVENT NODE=2 );\n';
	var slonikPreamble = this.getSlonikPreamble();
	var slonik = this.coordinator.createSlonik('slonik, ddlFailure',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('ddl failed as expected',slonik.getReturnCode(),255);

	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - failure expected on event node");
	/**
	 *  Now try it against an event node that it should succeed.
	 */
	slonikScript = 'EXECUTE SCRIPT(  FILENAME=\''
	+ scriptFile.getAbsolutePath() + '\' , EVENT NODE=1 );\n'
	slonik = this.coordinator.createSlonik('slonik, ddlFailure',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('ddl accepted on node 1',slonik.getReturnCode(),0);
	
	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - paused replication to node 2");
	/**
	 * Replication to node 2 should now be 'paused', 
	 */
	java.lang.Thread.sleep(10*1000);
	var lag = this.measureLag(1,2);
	this.coordinator.log('we expect lag, we measure it as ' + lag);
	this.testResults.assertCheck('node is lagged', lag >= 10,true);

	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - lag analysis");
	/**
	 * Allow the DDL to work on node 2. 
	 */
	statement.execute('DROP TABLE disorder.test_transient');
	this.slonikSync(1, 1);
	/**
	 * sl_status showing some lag of 10 seconds or more is not
	 * unusual because of the way confirms propagate.
	 */
	// var lag = this.measureLag(1,2);
	// this.coordinator.log('we do not expect lag, we measure it as ' + lag);
	// this.testResults.assertCheck('node is not lagged', lag < 10,true);
	this.slonikSync(1,1);
	this.dropTestTable(1,1,false);
	
	statement.close();
	connection.close();
	this.coordinator.log("ExecuteScript.prototype.testDDLFailure - complete");
}

/**
 * Remove the test table 'disorder_transient' by performing a DROP table via 
 * EXECUTE SCRIPT.
 * 
 * if removeFromReplication is true then a 'set drop node' is also performed.
 * 
 */
ExecuteScript.prototype.dropTestTable=function(node_id,set_id,removeFromReplication) {
	this.coordinator.log('ExecuteScript.prototype.dropTestTable ' + node_id + ',' + set_id);
	var slonikPreamble = this.getSlonikPreamble();
	var scriptFile_drop = java.io.File.createTempFile('executeScript', '.sql');
	var fileWriter = new java.io.FileWriter(scriptFile_drop);
	fileWriter.write('DROP TABLE disorder.test_transient;');
	fileWriter.close();
	var slonikScript ='echo \'ExecuteScript.prototype.dropTestTable\';\n';
	if(removeFromReplication) {
		slonikScript+='set drop table(id=' + (this.tableIdCounter-1) + ',origin=' + node_id + ');\n';
	}
	slonikScript+=	'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile_drop.getAbsolutePath() + '\' , EVENT NODE=' + node_id +' );\n'
			+ 'SYNC(id=' + node_id + ');\n'
			+ 'wait for event(origin=' + node_id + ', confirmed=all, wait on=' + node_id+');\n';
	var slonik = this.coordinator.createSlonik('DROP TABLE', slonikPreamble,
			slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik executed drop table okay', slonik
			.getReturnCode(), 0);

	this.coordinator.log('ExecuteScript.prototype.dropTestTable ' + node_id + ',' + set_id+ " complete");
}
ExecuteScript.prototype.dropSet3 = function(set_origin) {
        this.coordinator.log('ExecuteScript.prototype.dropSet3 ' + set_origin + " - begin");
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript ='echo \'ExecuteScript.prototype.dropSet3\';\n' 
		+ 'drop set (id=3,origin=' + set_origin + ');'
		+ 'wait for event(origin=' + set_origin + ', wait on=' + set_origin + ', confirmed=all);\n';
	var slonik=this.coordinator.createSlonik('DROP SET',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop set 3 worked as expected',slonik.getReturnCode(),0);
        this.coordinator.log('ExecuteScript.prototype.dropSet3 ' + set_origin + " - complete");
}


/**
 * Performs an INSERT on all nodes via EXECUTE script and verifies that
 * changes to the replica databases are allowed via execute script.
 * 
 * 
 */
ExecuteScript.prototype.performInsert=function(node_id) {
	this.coordinator.log("ExecuteScript.prototype.performInsert on node " + node_id + " - begin");
	var slonikPreamble = this.getSlonikPreamble();
	var scriptFile = java.io.File.createTempFile('executeScript', '.sql');
	var fileWriter = new java.io.FileWriter(scriptFile);
	fileWriter.write('INSERT INTO disorder.do_customer(c_name) VALUES (disorder.digsyl(' + new java.util.Date().getTime() +',16));');
	fileWriter.close();
	var slonikScript = 'echo \'ExecuteScript.prototype.performInsert\';\n';
	slonikScript += 'EXECUTE SCRIPT(  FILENAME=\''
			+ scriptFile + '\' , EVENT NODE=' + node_id +' );\n'
			+ 'SYNC(id=' + node_id + ');\n'
			+ 'wait for event(origin=' + node_id + ', confirmed=all, wait on=' + node_id+');\n';
	var slonik = this.coordinator.createSlonik('DROP TABLE', slonikPreamble,
			slonikScript);
	
	// Generate some WRITE load.
	// We want the EXECUTE script to go in sequence.
	
	this.coordinator.log("ExecuteScript.prototype.performInsert on node " + node_id + " - imposing load");

	var disorderClientJs = this.coordinator.readFile('disorder/client/disorder.js');
	disorderClientJs+= this.coordinator.readFile('disorder/client/run_fixed_load.js');
	var load = this.coordinator.clientScript(disorderClientJs,'db' + node_id);
	load.run();
	
	
	slonik.run();
	this.coordinator.log("ExecuteScript.prototype.performInsert on node " + node_id + " - performing drop table");
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik executed drop table okay', slonik
			.getReturnCode(), 0);

	load.stop();
	this.coordinator.join(load);
	this.coordinator.log("ExecuteScript.prototype.performInsert on node " + node_id + " - SYNC, compare DBs");
	this.slonikSync(1,1);
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');
	this.compareDb('db1','db4');
	this.compareDb('db1','db5');
		
	this.coordinator.log("ExecuteScript.prototype.performInsert on node " + node_id + " - complete");
}
