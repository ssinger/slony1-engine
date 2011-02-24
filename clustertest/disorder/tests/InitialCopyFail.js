/**
 * This test checks out the behaviour when an initial copy fails
 * and ensures that the copy will succeed later (once the problem
 * causing the copy failure to go away)
 *
 */
coordinator.includeFile('disorder/tests/BasicTest.js');

InitialCopyFail=function(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription="Tests behaviour when an "
		+ "initial copy/subscribe fails. The reason "
		+ "causing the failure is then removed and "
		+ "the test verifies that the subscription then " 
		+ "proceeds.";
}
InitialCopyFail.prototype = new BasicTest();
InitialCopyFail.prototype.constructor = InitialCopyFail;

InitialCopyFail.prototype.getNodeCount=function() {
	return 3;
}

InitialCopyFail.prototype.runTest = function() {
	
        this.coordinator.log("InitialCopyFail.prototype.runTest - begin");
	this.testResults.newGroup("Initial Copy Fails");
	this.setupReplication();
	this.addTables();
	
	/**
	 * Go to node 1 and ADD a column.
	 */
	var connection = this.coordinator.createJdbcConnection('db1');
	var statement = connection.createStatement();
	var result = statement.execute('ALTER TABLE disorder.do_customer ADD COLUMN testme int4;');
	this.testResults.assertCheck('alter table okay',result,false);			
	statement.close();
	connection.close();
	
	
	var copyFail=0;
	var thisRef = this;
	var slonikArray=[];
	var onCopyFail = {
			onEvent : function(source,event) {
		
				//
				// By removing the observer we ensure that the same slon/node does not
				// get counted twice in copyFail - since the slon will keep retring.
				thisRef.coordinator.removeObserver(source,event,this);
				copyFail++;
				if(copyFail >= thisRef.getNodeCount()-1) {

					
					//
					// Now validate that the tables are empty on the subscribers
					// we delete everything from the subscribers first.
					for(var idx=2; idx <=thisRef.getNodeCount();idx++) {
						var connection = thisRef.coordinator.createJdbcConnection('db' + idx);
						var stat = connection.createStatement();
						var rs = stat.executeQuery("SELECT COUNT(*) FROM disorder.do_customer");
						rs.next();
						thisRef.testResults.assertCheck('customer table is empty',rs.getInt(1),0);
						rs.close();
						stat.close();
						connection.close();
					}
					
														
				}
			}
	};
	
	
	
        this.coordinator.log("InitialCopyFail.prototype.runTest - start slons");
	/**
	 * Start the slons.
	 */
	var slonArray=[];
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		slonArray[idx-1] = this.coordinator.createSlonLauncher('db' + idx);
		this.coordinator.registerObserver(slonArray[idx-1],
				Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_COPY_FAILED,
				new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(onCopyFail));
		slonArray[idx-1].run();
	}
	
	
	
	/**
	 * Issue the subscribe set command via slonik 
	 * We
	 *     1) Expect it to fail (slonik command timeout)
	 *     2) Expect the subscribers to be empty after
	 *    
	 * 
	 */
	var slonikPreamble = this.getSlonikPreamble();
	
        this.coordinator.log("InitialCopyFail.prototype.runTest - subscribe sets");
	
	for(var idx=2; idx <= this.getNodeCount(); idx++) {
		// truncate the subscribers
		// this way if the table is non-empty then the subscription must
		// have worked.
		var jdbcCon = this.coordinator.createJdbcConnection('db'+idx);
		var stat = jdbcCon.createStatement();
		stat.execute("TRUNCATE disorder.do_customer CASCADE");
		stat.close();
		jdbcCon.close();

	        var slonikScript = 'echo \'InitialCopyFail.prototype.runTest\';\n';
		slonikScript += 'subscribe set (id=1, provider=1, receiver=' + idx+ ');\n'
		+'wait for event(origin=1, confirmed=' + idx + ', wait on=1,timeout=30);\n'
		+'sync(id=1);\n'
		+'wait for event(origin=1, confirmed=' + idx + ', wait on=1,timeout=30);\n';
		slonikArray[idx] = this.coordinator.createSlonik('subscribe',slonikPreamble,slonikScript);
		slonikArray[idx].run();	
	}	
	for(var idx=2; idx <= this.getNodeCount(); idx++) {
		this.coordinator.join(slonikArray[idx]);
		var result = slonikArray[idx].getReturnCode();
		this.testResults.assertCheck('slonik returned failure on a blocked subscription',result,255);
	}
		

   for(var idx=2; idx <= thisRef.getNodeCount(); idx++) {
	   var connection = thisRef.coordinator.createJdbcConnection('db' + idx);
	   var statement = connection.createStatement();
	   var result = statement.execute('ALTER TABLE disorder.do_customer ADD COLUMN testme int4;');
	   thisRef.testResults.assertCheck('alter table okay',result,false);
	   statement.close();
	   connection.close();
	   
   }
	
	
        this.coordinator.log("InitialCopyFail.prototype.runTest - sync");
	//This sync should work. 
	this.slonikSync(1,1);
        this.coordinator.log("InitialCopyFail.prototype.runTest - compare db1,2,3");
	this.compareDb('db1','db2');
	this.compareDb('db1','db3');

	for(var idx=0; idx < this.getNodeCount(); idx++) {
		slonArray[idx].stop();
		this.coordinator.join(slonArray[idx]);
	}
	
	this.coordinator.log('About to remove the column we added, testme');
	/**
	 * Now connect to each DB and drop the column we added.
	 */	
	 for(var idx=1; idx <= this.getNodeCount(); idx++) {
	 	try {
	 		var connection = this.coordinator.createJdbcConnection('db' + idx);
			var statement = connection.createStatement();
			this.coordinator.log('dropping column testme on ' + idx);
			var result = statement.execute('ALTER TABLE disorder.do_customer DROP COLUMN testme;');
		}
		catch(e) {
			this.testResults.assertCheck('failure dropping column:' + e, true,false);
		}
		finally {
			statement.close();
			connection.close();
		}
					
	 }
        this.coordinator.log("InitialCopyFail.prototype.runTest - complete");
}
