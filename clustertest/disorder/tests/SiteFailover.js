
coordinator.includeFile('disorder/tests/FailNodeTest.js');

SiteFailover = function(coordinator, testResults) {
	Failover.call(this, coordinator, testResults);
	this.testDescription='Test the FAILOVER command.  This test will try FAILOVER'
		+' with an entire site failing';
	this.compareQueryList.push(['select i_id,comments from disorder.do_item_review order by i_id','i_id']);

}
SiteFailover.prototype = new Failover();
SiteFailover.prototype.constructor = SiteFailover;

SiteFailover.prototype.getNodeCount = function() {
	return 6;
}

SiteFailover.prototype.runTest = function() {
    this.coordinator.log("SiteFailover.prototype.runTest - begin");
    this.testResults.newGroup("Site Fail Over Test");
    basicTest.prepareDb(['db6']);
    this.setupReplication();
    
    /**
     * 
     * set 1
     *
     *    1 --------> 2
     *   /\          /\ 
     *  3  4        5  6
     *
     * set 2 
     *   3-----------> 5
     *  /\            /\
     * 1  4          2  6
     *
     * 
     *  3/2 has a path because 3 is the local failover target for node 1
     *  
     *  There is a path between 1/5 because 1 is the failover target for set 2 if node 3 fails.
     *
     *  There is no path between 4/6 , or 1/6 or even 3/6.
     *  
     */
    
    /**
     * Start the slons.
     */
    this.slonArray = [];
    for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
	this.slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
	this.slonArray[idx - 1].run();
    }
    var slonikScript='';
    /**
     * add in paths between nodes  1-->4 and 2-->5
     * also add in node 6
     */
    slonikScript += 'store path(server=1,client=4,conninfo=@CONNINFO1 );\n';
    slonikScript += 'store path(server=4,client=1,conninfo=@CONNINFO4 );\n';
    slonikScript += 'store path(server=2,client=5,conninfo=@CONNINFO2 );\n';
    slonikScript += 'store path(server=5,client=2,conninfo=@CONNINFO5 );\n';

   
    slonikScript += 'store path(server=2,client=6,conninfo=@CONNINFO2 );\n';
    slonikScript += 'store path(server=6,client=2,conninfo=@CONNINFO6 );\n';
    slonikScript += 'store path(server=5,client=6,conninfo=@CONNINFO5 );\n';
    slonikScript += 'store path(server=6,client=5,conninfo=@CONNINFO6 );\n';
    var slonikPreamble = this.getSlonikPreamble();
    var slonik=this.coordinator.createSlonik('failover',slonikPreamble,slonikScript);
    slonik.run();
    this.coordinator.join(slonik);
    this.testResults.assertCheck('add extra paths passes',slonik.getReturnCode(),0);
    
    /**
     * Add some tables to replication.
     * 
     */
    this.addTables();


    /**
     * create the second set.
     */
    this.createSecondSet(3);

    /**
     * Subscribe the first set.
     */
    this.subscribeSet(1,1, 1, [ 2, 3 ,4]);
    this.subscribeSet(1,1, 2, [ 5,6 ]);
    /**
     * subscribe the second set.     
     */
    this.subscribeSet(2,3,3,[1,4,5]);
    this.subscribeSet(2,3,5,[2,6]);
    this.slonikSync(1,1);
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
    this.failover(1,2,3,5,4);		
    load.stop();
    this.coordinator.join(load);

	/**
	 * Now drop the nodes
	 */
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'SiteFailover.drop nodes\';\n';
	slonikScript+= 'drop node(id=\'1,3,4\',event node = 2);\n'
		+ 'try { \n uninstall node(id=1);}\n on error { echo \'slony not installed\';}\n'
		+ 'try{uninstall node(id=3);}on error{echo \'slony not installed\';}\ntry{ uninstall node(id=3);} on error {echo \'slony not installed\';}\n';

	var slonik=this.coordinator.createSlonik('drop node',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop 3 nodes passes',slonik.getReturnCode(),0);	
    this.currentOrigin='db2';
	load = this.generateLoad();
    java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);
    this.slonikSync(1,2);
    this.slonikSync(2,5);
    
	
    for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
	this.slonArray[idx - 1].stop();
	this.coordinator.join(this.slonArray[idx - 1]);
    }		
    this.compareDb('db2','db5');
    this.compareDb('db2','db6');
    this.dropDb(['db6']);
    
}

SiteFailover.prototype.failover=function(originA,backupA,originB,backupB,receiverC) 
{
    var slonikPreamble = 'cluster name=disorder_replica;\n';
    var idx=2
    slonikPreamble += 'node ' + idx + ' admin conninfo=\'dbname=$database.db'
	+ idx + '.dbname host=$database.db' + idx
	+ '.host  '
	+ ' port=$database.db' + idx + '.port'
	+' user=$database.db' + idx				
	+ '.user.slony password=$database.db' + idx + '.password.slony\';\n';
    
    var idx=5
    slonikPreamble += 'node ' + idx + ' admin conninfo=\'dbname=$database.db'
	+ idx + '.dbname host=$database.db' + idx
	+ '.host  '
	+ ' port=$database.db' + idx + '.port'
	+' user=$database.db' + idx				
	+ '.user.slony password=$database.db' + idx + '.password.slony\';\n';
    
    var idx=6
    slonikPreamble += 'node ' + idx + ' admin conninfo=\'dbname=$database.db'
	+ idx + '.dbname host=$database.db' + idx
	+ '.host  '
	+ ' port=$database.db' + idx + '.port'
	+' user=$database.db' + idx				
	+ '.user.slony password=$database.db' + idx + '.password.slony\';\n';
    

	/**
	 * first try failing over when we don't mention node 4.
	 * This should fail.
	 */
    var slonikScript = 'echo \'SiteFailover.prototype.failover\';\n';
	slonikScript += 'FAILOVER( node=(id='  + originA  + ',backup node=' + backupA +')'
	+ ', node=(id=' + originB + ',backup node=' + backupB + '));\n';
	var slonik=this.coordinator.createSlonik('failover',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('failover (should not work)',slonik.getReturnCode()==0,false);	
	

	/**
	 * try again, everything should work
	 */ 

	slonikScript = 'echo \'SiteFailover.prototype.failover\';\n';
	slonikScript += 'FAILOVER( node=(id='  + originA  + ',backup node=' + backupA +')'
	+ ', node=(id=' + originB + ',backup node=' + backupB 
		+ '), node=(id='+receiverC + ', backup node=' + backupA + ') );\n';
	var slonik=this.coordinator.createSlonik('failover',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('failover should work',slonik.getReturnCode(),0);

	
}


