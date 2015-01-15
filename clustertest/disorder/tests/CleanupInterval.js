coordinator.includeFile('disorder/tests/BasicTest.js');

CleanupInterval=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
    this.testDescription = 'This test exercieses the cleanup interval in the config file';
}
CleanupInterval.prototype = new BasicTest();
CleanupInterval.prototype.constructor = EmptySet;




CleanupInterval.prototype.runTest = function() {
        this.coordinator.log("CleanupInterval.prototype.runTest - begin");

	this.testResults.newGroup("Cleanup Interval");
	this.setupReplication();
	
	var slonArray=[];        
        
    for(var idx=1; idx <= this.getNodeCount(); idx++) {
	var confMap = this.getSlonConfFileMap(idx);
	confMap.put('cleanup_interval','10');
	slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx,confMap);
	slonArray[idx-1].run();
	}
	this.addTables();
        this.coordinator.log("CleanupInterval.prototype.runTest - subscribe sets");
	this.subscribeSet(1, 1,1,[2]);
        this.slonikSync(1,1);
	
	var populate=this.generateLoad(1);
	//sleep 1 minute, check that nothing sl_event from the origin is older than 50 seconds
	var dbCon = this.coordinator.createJdbcConnection('db1');
	var stat = dbCon.createStatement();

	for(var cnt=0; cnt < 5;cnt++) {
	    java.lang.Thread.sleep(60*1000);
	    var  rs= stat.executeQuery("SELECT count(*) from _" + this.getClusterName() 
				       + ".sl_event where ev_origin=1 and ev_type='SYNC' and ev_timestamp<now()-'40 seconds'::interval");			    
		rs.next();
		this.testResults.assertCheck('no old events in sl_event',rs.getInt(1),0);
		rs.close();     
	}
	stat.close();
	dbCon.close();
	populate.stop();
	this.coordinator.join(populate);

	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
	
        this.coordinator.log("CleanupInterval.prototype.runTest - complete");
	
}



