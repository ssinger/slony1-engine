coordinator.includeFile('disorder/tests/BasicTest.js');

CloneNode=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription = 'This test exercieses the CLONE PREPARE and CLONE FINISH\n'
		+ 'commands.  It creates a new cloned slave node based on another existing\n'
		+ 'node.';
}
CloneNode.prototype = new BasicTest();
CloneNode.prototype.constructor = EmptySet;




CloneNode.prototype.runTest = function() {
        this.coordinator.log("CloneNode.prototype.runTest - begin");

	this.testResults.newGroup("Clone Node");
	this.setupReplication();
	
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}
	this.addTables();
        this.coordinator.log("CloneNode.prototype.runTest - subscribe sets");
	this.subscribeSet(1, 1,1,[2]);
	
	
        this.coordinator.log("CloneNode.prototype.runTest - CLONE PREPARE");
	var preamble = this.getSlonikPreamble();
	var script = 'CLONE PREPARE(id=6, provider=2);';
	var slonik = this.coordinator.createSlonik('clone',preamble,script);
	slonik.run();
	this.coordinator.join(slonik);
        this.coordinator.log("CloneNode.prototype.runTest - pgdump_db2 to generate new node");
	var dumpFile = java.io.File.createTempFile('pgdump_db2','.sql');
	//Now pg_dump the database.
	var pg_dump = this.coordinator.createPgDumpCommand("db2",dumpFile.getAbsolutePath(),null,false)
	pg_dump.run();
	this.coordinator.join(pg_dump);
        this.coordinator.log("CloneNode.prototype.runTest - generate node db6");
	var createDb = this.coordinator.createCreateDb('db6');
	createDb.run();
	this.coordinator.join(createDb);
	this.testResults.assertCheck('db6 created okay',createDb.getReturnCode(),0);
        this.coordinator.log("CloneNode.prototype.runTest - load dump to node db6");
	var restorePsql = this.coordinator.createPsqlCommand("db6",dumpFile);
	restorePsql.run();
	this.coordinator.join(restorePsql);
	this.testResults.assertCheck('database restored okay',restorePsql.getReturnCode(),0);
        this.coordinator.log("CloneNode.prototype.runTest - add db6 to cluster");
	//Now run slonik to finish things off.
	
	
	//
	//
	// We want to generate a preamble with 6 nodes, we temporarily replace
	// the getNodeCount() function with one that does what we want
	// We then set it back.
	oldFunc = this.getNodeCount;
	this.getNodeCount = function() {
		return 6;
	}	
	preamble = this.getSlonikPreamble();
	
	
	this.getNodeCount = oldFunc;
	script = 'CLONE FINISH(id=6,provider=2);\n'			
			+  'store path(server=1,client=6,conninfo=@CONNINFO1 );\n'
			+'store path(server=6,client=1,conninfo=@CONNINFO6 );\n';
	slonik = this.coordinator.createSlonik('clone finish',preamble,script);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('clone finish succeeded',slonik.getReturnCode(),0);
	var slon6 = this.coordinator.createSlonLauncher('db6');
	slon6.run();
	
	//Restart all the slons 
	//for(var idx=1; idx <= this.getNodeCount(); idx++) {
	//	slonArray[idx-1].stop();
	//	this.coordinator.join(slonArray[idx-1]);
	//	slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
	//	slonArray[idx-1].run();
	//}
	
	this.slonikSync(1, 1);
	slon6.stop();
	this.coordinator.join(slon6);
	
        this.coordinator.log("CloneNode.prototype.runTest - db6 now in cluster");
	
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
	var dropDb = this.coordinator.createDropDbCommand('db6');
	dropDb.run();
	this.coordinator.join(dropDb);
        this.coordinator.log("CloneNode.prototype.runTest - complete");
	
}


