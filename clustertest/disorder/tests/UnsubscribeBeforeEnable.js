/**
 * This test tests a scenario along the lines of the following:
 * 
 *  1)Set 1 is subscribed to with node 1 as the origin
 *  2)SUBSCRIBE SET is processed on node 2 
 *  3)A slonik process executes unsubscribe set on node 2
 *  4)The   ENABLE_SUBSCRIPTION is processed on node 2 after the unsubscribe
 *  
 *  @see Slony bug 111.
 * 
 *  Currently this bug is unresolved (ie the test fails).
 * 
 */
coordinator.includeFile("disorder/tests/BasicTest.js");


function UnsubscribeBeforeEnable(coordinator,testResults) {
	BasicTest.call(this,coordinator,testResults);
	this.testDescription='Tests an UNSUBSCRIBE event in the middle'
	+' of a subscription process. This currently reveals bug 111';
}

UnsubscribeBeforeEnable.prototype=new BasicTest();
UnsubscribeBeforeEnable.prototype.constructor=UnsubscribeBeforeEnable;
UnsubscribeBeforeEnable.prototype.getNodeCount = function() {
	return 2;
}


UnsubscribeBeforeEnable.prototype.runTest = function() {
        this.coordinator.log("UnsubscribeBeforeEnable.prototype.runTest - begin");	
	this.testResults.newGroup("unsubscribe before enable");

	//First setup slony 

	this.setupReplication();
	this.addTables();

	/**
	 * We replace subscribeset_int on the subscriber with a version that does a pg_sleep
	 * this gives us time to unsubscribe AFTER the subscribe set is processed on node 2. 
	 */


	var replaceSql = 'ALTER FUNCTION _foo.subscribeset_int(int4,int4,int4,bool,bool) RENAME TO subscribeset_int2;\n' 
	
		+'CREATE OR REPLACE FUNCTION _foo.subscribeset_int(int4,int4,int4,bool,bool) returns int4\n' 
		+ 'as $$ DECLARE r int4; \n'
		+ 'BEGIN \n'
		+ ' select into r _foo.subscribeset_int2($1,$2,$3,$4,$5);PERFORM  pg_sleep(10); return r\n;'
		+ ' END;\n'
		+' $$  language \'plpgsql\';\n';


	psql = this.coordinator.createPsqlCommand(['db2'],replaceSql);
	psql.run();
	this.coordinator.join(psql);
	
	var preamble = this.getSlonikPreamble();
	var slonikList=this.subscribeSetBackground(1,1,1,[2]);
	var slonik=slonikList[0];

	//Startup slon

	//Start the slons.

	slon1 = this.coordinator.createSlonLauncher("db1");
	slon2 = this.coordinator.createSlonLauncher("db2");

	//
	// This handler will unsubscribe the set. 
	// It should be invoked after the store subscription has been processed.
	var thisRef = this;
	onEnable = {
			onEvent : function(source,eventType) {
			//ENABLE SUBSCRIPTION has run
			var unsubscribe = 'unsubscribe set(id=1,receiver=2);';
			var slonikUnsubscribe=thisRef.coordinator.createSlonik('unsubscribe',preamble,unsubscribe);
		
			
			var unsubscribeFinished = {
					onEvent : function(source,eventType) {
					thisRef.coordinator.removeObserver(slonikUnsubscribe,Packages.info.slony.clustertest.testcoordinator.Coordinator.EVENT_FINISHED,this);
				
				}
			};
			thisRef.coordinator.registerObserver(slonikUnsubscribe,Packages.info.slony.clustertest.testcoordinator.Coordinator.EVENT_FINISHED,
					new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(unsubscribeFinished));
			slonikUnsubscribe.run();
		}
	};

	this.coordinator.registerObserver(slon2,Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_STORE_SUBSCRIBE
			,new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(onEnable));


	//
	// Check for slon failures. 
	// If slon exits abnormally then something went wrong.
	onSlonFails = {
			onEvent : function(source,eventType) {
			results.assertCheck("slon exited abnormally",true,false);
			slonik.stop();
		}
	};


	this.coordinator.registerObserver(slon1,Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_ABNORMAL_EXIT
			,new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(onSlonFails));
	this.coordinator.registerObserver(slon2,Packages.info.slony.clustertest.testcoordinator.slony.SlonLauncher.EVENT_ABNORMAL_EXIT
			,new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(onSlonFails));


	slon1.run();
	slon2.run();
	slonik.run();
	/**
	 * slonik should start the subscription process, which will trigger the
	 * slon processes (slon2 in particular) into issuing an notice that invokes
	 * the onEnable observer (above).
	 * 
	 * The onEnable observer will trigger an unsubscribe, while the slon2 sleeps
	 * due to the pgplsql function manipulation done above.
	 * 
	 * This should leave the cluster in an unsubscribed state, 
	 * however bug 111 meant that the cluster got left where the initial subscription
	 * never finished.  If our slonik times out then the test fails.
	 * 
	 */
	this.coordinator.join(slonik);


	if(slonik.getReturnCode()==0) {
		populate = this.generateLoad();
		java.lang.Thread.sleep(10*1000);
		populate.stop();
		this.coordinator.join(populate);
		var r = this.slonikSync(1,1);
		if(r == 0) {
			this.compareDb('db1','db2');
		}
	}
	slon1.stop();
	slon2.stop();
	this.coordinator.join(slon1);
	this.coordinator.join(slon2);
        this.coordinator.log("UnsubscribeBeforeEnable.prototype.runTest - complete");	
}
