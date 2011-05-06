/**
 *
 */
coordinator.includeFile("disorder/tests/BasicTest.js");


function LongTransaction(coordinator,results) {
	BasicTest.call(this,coordinator,results);	
	this.testDescription = 'Tests the effect a long running transaction has on replication';
}

LongTransaction.prototype = new BasicTest();
LongTransaction.prototype.constructor=LongTransaction;
LongTransaction.prototype.getNodeCount = function() {
	return 5;
}

LongTransaction.prototype.runTest = function() {
        this.coordinator.log("LongTransaction.prototype.runTest - begin");

	this.testResults.newGroup("Long Transaction");
	//this.prepareDb(['db1','db2']);

	
	//First setup slony

	this.setupReplication();
	this.addCompletePaths();
	
	
        this.coordinator.log("LongTransaction.prototype.runTest - start long transaction");
	/**
	 * Start a transaction.
	 * Start the add table process.
	 * The add tables should not proceed while the older transaction
	 * is in progress.
	 */
	var txnConnection = this.startTransaction();
	
        var slonikScript = 'echo \'LongTransaction.prototype.runTest\';\n';
	slonikScript += this.getAddTableSlonikScript();
	var slonikPreamble = this.getSlonikPreamble();
	var slonik  = this.coordinator.createSlonik('add tables',slonikPreamble,slonikScript);
	slonik.run();
	java.lang.Thread.sleep(10*1000);
	this.testResults.assertCheck('transaction is blocking add table command',slonik.isFinished(),false);




	txnConnection.rollback();
	txnConnection.close();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('add tables completed after transaction finished',slonik.getReturnCode(),0);
	
	
        this.coordinator.log("LongTransaction.prototype.runTest - start slons");
	//Start the slons.
	//These must be started before slonik runs or the subscribe won't happen
	//thus slonik won't finish.
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		slonArray[idx-1].run();
	}

	
        this.coordinator.log("LongTransaction.prototype.runTest - subscribe sets in background");
	var subs=this.subscribeSetBackground(1,1,1,[2,3]);
	for(var idx=0; idx < subs.length; idx++) {
		subs[idx].run();
	}
	//A transaction should not block the subscription.
	//make sure this is the case.
	 txnConnection = this.startTransaction();
        this.coordinator.log("LongTransaction.prototype.runTest - sleep 3x60x1000");
	java.lang.Thread.sleep(3*60*1000);
	for(var idx=0; idx < subs.length; idx++) {
		this.testResults.assertCheck('subscription blocking on the transaction', subs[idx].isFinished(),true);
	}
	//Okay now release the transaction.
	txnConnection.rollback();
	txnConnection.close();
	
	
        this.coordinator.log("LongTransaction.prototype.runTest - subscribe 4,5");
	this.subscribeSet(1,1,3,[4,5]);
	
        this.coordinator.log("LongTransaction.prototype.runTest - subscriptions complete");
	this.coordinator.log('subscriptions complete');

	
	txnConnection = this.startTransaction();
	var count1 = this.countOrders('db2');
	var load = this.generateLoad();
	java.lang.Thread.sleep(20*1000);
	var count2 = this.countOrders('db2');
	this.testResults.assertCheck('data is replicating',count1 < count2,true);
	load.stop();
	this.slonikSync(1,1);
	this.coordinator.join(load);
        this.coordinator.log("LongTransaction.prototype.runTest - compare db1,2,4");
	this.compareDb('db1','db2');
	this.compareDb('db1','db4');
	
	txnConnection.rollback();
	txnConnection.close();
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1].stop();
		this.coordinator.join(slonArray[idx-1]);	
	}
}

LongTransaction.prototype.startTransaction=function() {
	var dbCon = this.coordinator.createJdbcConnection('db1');
	var stat = dbCon.createStatement();
	dbCon.setAutoCommit(false);
	var  rs= stat.executeQuery('SELECT COUNT(*) FROM disorder.do_customer;');
	rs.close();
	stat.close();
	return dbCon;
        this.coordinator.log("LongTransaction.prototype.runTest - begin");
}

LongTransaction.prototype.countOrders=function(db) {
	var dbCon = this.coordinator.createJdbcConnection(db);
	var stat = dbCon.createStatement();
	var  rs= stat.executeQuery('SELECT COUNT(*) FROM disorder.do_order;');
	rs.next();
	var count = rs.getInt(1);
	rs.close();
	stat.close();
	dbCon.close();
	return count;
}