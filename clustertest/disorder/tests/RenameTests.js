/**
 * Tests Renaming tables.
 * 
 */

coordinator.includeFile('disorder/tests/BasicTest.js');

RenameTests = function(coordinator, testResults) {
	ExecuteScript.call(this, coordinator, testResults);
	this.testDescription='This test will rename tables in various ways and ensure that'
		+' replication is not broken by these changes';
}
RenameTests.prototype = new ExecuteScript();
RenameTests.prototype.constructor = RenameTests;

RenameTests.prototype.runTest = function() {
        this.coordinator.log("RenameTests.prototype.runTest - begin");

	this.testResults.newGroup("Rename tables");
	this.setupReplication();

	/**
	 * Start the slons.
	 */
	var slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx - 1].run();
	}

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

	
	/**
	  * Create the test_transient table.
	  */
	
	this.createAndReplicateTestTable();
	
	
	/**
	 * Add a row to the table.
	 */
	var jdbcCon = this.coordinator.createJdbcConnection('db1');
	var stat = jdbcCon.createStatement();
	stat.execute("INSERT INTO disorder.test_transient(data) VALUES ('foo')");
	/**
	 * Rename a table via execute script.
	 * Ensure that changes to the table still get
	 * replicated.
	 */
	this.slonikSync(1,1);		
	this.executeScript("ALTER TABLE disorder.test_transient RENAME TO test_transient2;\n");
	
	 
	 /**
	  * Add an additional row, using the new name. 
	  */
	 stat.execute("INSERT INTO disorder.test_transient2(data) VALUES ('foo')");
	 this.slonikSync(1,1);
	 
	var jdbcCon2 = this.coordinator.createJdbcConnection('db4');
	var stat2 = jdbcCon2.createStatement();
	var rs = stat2.executeQuery("SELECT COUNT(*) FROM disorder.test_transient2;");
	rs.next();	
	this.testResults.assertCheck('change replicated to transient2',rs.getInt(1),2);
	rs.close();
	
	/**
	 * Now move the table to its own schema.	 
	 */
	this.executeScript("ALTER TABLE disorder.test_transient2 SET SCHEMA public");
	 
	 
	stat.execute("INSERT INTO public.test_transient2(data) VALUES ('foo')");
	this.slonikSync(1,1);
	rs = stat2.executeQuery("SELECT COUNT(*) FROM public.test_transient2;");	 
	rs.next();	
	this.testResults.assertCheck('change replicated to transient2',rs.getInt(1),3);			
	rs.close();
	
	/**
	 * Validate that sl_table has the updated data.
	 */
	rs = stat2.executeQuery("SELECT COUNT(*) FROM \"_" + this.getClusterName() 
		+"\".sl_table where tab_relname='test_transient2' AND tab_nspname='public';");		
	rs.next();
	this.testResults.assertCheck('proper row in sl_table exists',rs.getInt(1),1);	
	stat.close();
	stat2.close();
	jdbcCon.close();
	jdbcCon2.close();
	 
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx - 1].stop();
		this.coordinator.join(slonArray[idx - 1]);		
	}
	
        this.coordinator.log("RenameTests.prototype.runTest - complete");

}

RenameTests.prototype.executeScript=function(sql) {
	var scriptFile = java.io.File.createTempFile('executeScript', '.sql');
	scriptFile.deleteOnExit();
	var slonikPreamble = this.getSlonikPreamble();
	var fileWriter = new java.io.FileWriter(scriptFile);
	fileWriter.write(sql);
	fileWriter.close();
        var slonikScript = 'echo \'RenameTests.prototype.executeScript\';\n';
	slonikScript += "EXECUTE SCRIPT(FILENAME='" + scriptFile.getAbsolutePath()
		+ "',EVENT NODE=1);\n";
	var slonik = this.coordinator.createSlonik('rename table',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('rename table 1 worked okay',slonik.getReturnCode(),0);
}
