/** Tests the failover() command.
 * 
 *  
 * 
 * 
 */

coordinator.includeFile('disorder/tests/FailNodeTest.js');

Failover = function(coordinator, testResults) {
	FailNodeTest.call(this, coordinator, testResults);
	this.testDescription='Test the FAILOVER command.  This test will try FAILOVER'
		+' from different cluster configurations';

}
Failover.prototype = new FailNodeTest();
Failover.prototype.constructor = Failover;

Failover.prototype.runTest = function() {
        this.coordinator.log("Failover.prototype.runTest - begin");
	this.testResults.newGroup("Fail Over Test");
	this.setupReplication();
	this.addCompletePaths();
	/**
	 * Start the slons.
	 */
	this.slonArray = [];
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1] = this.coordinator.createSlonLauncher('db' + idx);
		this.slonArray[idx - 1].run();
	}
	this.addCompletePaths();
	/**
	 * Add some tables to replication.
	 * 
	 */
	this.addTables();

	/**
	 * Subscribe the first node.
	 */
	this.subscribeSet(1,1, 1, [ 2, 3 ]);


	/**
	 * try failing over node 1 to node 4 first.
	 * node 4 is NOT subscribed to the sets of node
	 * 1 so this had better fail.
	 */
	this.slonikSync(1,1);
	this.failNode(1,4,false);
	this.slonArray[1 - 1] = this.coordinator.createSlonLauncher('db' + 1);
	this.slonArray[1 - 1].run();
	
	
	this.subscribeSet(1,1, 3, [ 4, 5 ]);
	this.slonikSync(1,1);


	

	var load = this.generateLoad();
	
	
	/**
	 * FAIL node 5.
	 * Node 5 is not a provider.
	 * This should go off smoothly.
	 */
	this.failNode(5,1,true);
  
	var lag1 = this.measureLag(1,5);
	java.lang.Thread.sleep(10*1000);
	var lag2 = this.measureLag(1,5);
	this.testResults.assertCheck('lag on node 5 is increasing',lag2 > lag1 ,true);
		
	
	/**
	 * DROP node 5.
	 */
	this.coordinator.log('PROGRESS:Testing drop of node 5');
	this.dropNode(5,1);
	//Make sure all the events from node 1 (ie the DROP NODE above)
	//make it elsewhere.
	this.slonikSync(1,1);
	this.reAddNode(5,1,3);
   	this.subscribeSet(1,1,3,[5]);

	

	
	
	
	
	/**
	 * FAILOVER node 1 to 3
	 * At this stage 4,5 are direct subscribers from 3 due to
	 * the above activities.
	 * 
	 * Re resubscribe node 2 to receive from node 3 before the
	 * FAILOVER this should test the more simple case.
	 */	this.coordinator.log('PROGRESS:failing 1=>3 where 2 is first moved to get from 3');
	this.addCompletePaths();		
	this.subscribeSet(1,1,3,[2]);
	this.slonikSync(1,1);
	this.failNode(1,3,true);	
	java.lang.Thread.sleep(10*1000);
	load.stop();
	this.coordinator.join(load);
	this.dropNode(1,3);
		
	this.reAddNode(1,3,3);
	this.addCompletePaths();
	
	this.moveSet(1,3,1);
	/**
	 * make sure we perform a SYNC after the move set
	 * but before we start failing nodes.  The listen network
	 * is different after the move set and we want to make sure
	 * all nodes have the new listen network.
	 * Node 5 might have a 1,SYNC event more recent
	 * than node 3 because prior to the MOVE SET
	 * the listen network allowed for this.
	 * The FAILOVER logic can't deal with 5 being
	 * the most-ahead node since it isn't a direct subscriber.
	 */
	this.slonikSync(1,1);
	load = this.generateLoad();
	
	
	
	//
	// At this stage we SHOULD be configured back to the 
	// default configuration.
	//
	// FAILOVER node 1=>3
	// node 1 is a provider to both 3 and 2
	// This is the more complicated case.
	this.coordinator.log('PROGRESS:failing 1=>3 where 2 also gets from 1');
	
	this.failNode(1,3,true);
	
	/**
	 * Replicate for a while.
	 */
	this.coordinator.log('replicating some data');
	java.lang.Thread.sleep(10*1000);
	
	//
	// Perform the subscribe in the background
	// We can't DROP node 1 until node 2 is reconfigured to 
	// receive from node 3.
	// Hoewver the normal 'subscribe' script won't work
	// in current versions of slony because confirmed=all will
	// wait for the node 1 confirmation, something that won't
	// happen.
	//
	// To get around this we don't use the normal wait for function.
	//var oldWait = this.generateSlonikWait;
	//this.generateSlonikWait = function(event_node) {
	//	var script = '';
	//	for(var node=1; node < this.getNodeCount(); node++) {
	//		if(node == event_node || node==1) {
	//			continue;
	//		}
	//		script+='wait for event(origin='+event_node+', wait on=' +
	//			event_node + ',confirmed=' + node+');\n';
	//	}
	//	return script;
	//}
	this.subscribeSet(1,3,3, [2]);
	//this.generateSlonikWait=oldWait;
	load.stop();
	this.coordinator.join(load);
	this.dropNode(1,3);
	//??this.coordinator.join(subscribeSlonik[0]);
	
	
	this.reAddNode(1,3,3);
	
	
	this.slonikSync(1,3);
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	this.addCompletePaths();
	this.moveSet(1,3,1)
	this.slonikSync(1,1);
	/**
	 * Now shutdown the slon for node 3, see how a failover to node 3 behaves.
	 */
	this.coordinator.log('PROGRESS:shutting down node 3 for a failover test');
	this.slonArray[2].stop();
	this.coordinator.join(this.slonArray[2]);
	/**
	 * create a timer event.
	 * in 60 seconds we will start up the slon again.
	 * the failover should not complete with the slon shutdown
	 * (at least not the 2.1 version of failover).
	 */
		

	this.coordinator.log('PROGRESS:load has stopped');
	var thisRef=this;	
	/**
	 * The failover needs to propogate before the DROP NODE or things can fail.
	 */
	 var onTimeout = {
		 onEvent : function(object, event) {
			 	thisRef.coordinator.log('PROGRESS:starting slon 3 back up');
				thisRef.slonArray[2] = thisRef.coordinator.createSlonLauncher('db3');
				thisRef.slonArray[2].run();


		}
	};
	var timeoutObserver = new Packages.info.slony.clustertest.testcoordinator.script.ExecutionObserver(onTimeout);
	var timer = this.coordinator.addTimerTask('restart slon', 120,
										 timeoutObserver);
	this.failNode(1,3,true);
	this.coordinator.removeObserver(timer,
										Packages.info.slony.clustertest.testcoordinator.Coordinator.EVENT_TIMER,
										timeoutObserver);
	if(this.slonArray[2].isFinished()) {
			thisRef.slonArray[2] = thisRef.coordinator.createSlonLauncher('db3');
			thisRef.slonArray[2].run();
	}

	this.dropNode(1,3);
	this.reAddNode(1,3,3);	
	this.slonikSync(1,3);
	this.compareDb('db1', 'db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	
	this.addCompletePaths();	
	this.moveSet(1,3,1);
	this.slonikSync(1,1);
	load = this.generateLoad();
	this.coordinator.log('stopping load');
	java.lang.Thread.sleep(30*1000);
	load.stop();
	this.coordinator.join(load);
	this.coordinator.log('load has been stopped');
	//this.slonikSync(1,1);
	
	/**
	 * Now try failing over from node 1=>node4.
	 * remove any paths connecting node 1 and 4
	 * but node 4 should have paths everywhere else. 
	 */	
	this.coordinator.log("PROGRESS:Failing from node 1 to 4");
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'Failover.prototype.runTest\';\n';
	slonikScript += 'drop path(server=1,client=4);\n'
		+'drop path(server=4,client=1);';
	var slonik = this.coordinator.createSlonik('drop paths to node 4',slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('drop path from 1 to 4',slonik.getReturnCode(),0);
	   
	this.slonikSync(1,1);
	 /**
	  * fail from 1--->4.  
	  * 4 is not a direct subscriber
	  * but the failover still works because 3
	  * can be used as a intermediate node.
	  */
	this.failNode(1,4,true);
		//this.dropNode(1,4);
	this.slonikSync(1,4);
	this.compareDb('db2','db4');
	this.compareDb('db3','db4');		
	java.lang.Thread.sleep(30*1000);

	this.coordinator.log('PROGRESS: About to re-add node 1');
	this.dropNode(1,4);
	this.reAddNode(1,4,4);
	
	
	this.slonikSync(1,4);	
	this.compareDb('db1','db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	this.compareDb('db4','db3');
	this.compareDb('db3','db2');
	this.compareDb('db4','db2');
	this.moveSet(1,4,1);
	//make nodes 2,3 receive from 1 directly
	this.addCompletePaths();
	this.subscribeSet(1,1,1,[2,3]);
		//
		// create a SECOND replication set
		// on the same origin as the first set.
		// Fail this over and make sure we can
		// failover both sets.
	this.createSecondSet(1);
	this.subscribeSet(2,1, 1, [ 2, 3 ]);
	this.slonikSync(1,1);
	this.failNode(1,2,true);

	
    this.slonikSync(1,2);
	

	this.compareDb('db1','db2');
	this.compareDb('db1', 'db3');
	this.compareDb('db1', 'db4');
	this.compareDb('db4','db3');
	this.compareDb('db3','db2');
	this.compareDb('db4','db2');
    
    	this.dropNode(1,2);
	this.reAddNode(1,2,2);	


	this.addCompletePaths();
	// NODE 4  MIGHT have been unsubscribed
	// from set 1.  This is because 
	// node 4 isn't subscribe to set 2.
	// If node 4 was more ahead
	// they are now unsubscribed.
	this.slonikSync(1,1);
	this.slonikSync(1,4);
	this.subscribeSet(1,1,1,[4]);

    this.slonikSync(1,2);
    this.moveSet(1,2,1);
    //stop slon 4
    this.coordinator.log('starting bug 318 test');
    this.slonArray[3].stop();
    this.coordinator.join(this.slonArray[3]);
    load2 = this.generateLoad();
    this.coordinator.log('stopping load');
    java.lang.Thread.sleep(3*30*1000);
    load2.stop();
	this.coordinator.join(load2);
    this.slonArray[3] = this.coordinator.createSlonLauncher('db4');
				this.slonArray[3].run();    
    this.failNode(1,3,true);
    
    this.compareDb('db2', 'db3');
    this.compareDb('db2', 'db4');
    this.compareDb('db3','db4');
    this.compareDb('db3','db5');
    
	for ( var idx = 1; idx <= this.getNodeCount(); idx++) {
		this.slonArray[idx - 1].stop();
		this.coordinator.join(this.slonArray[idx - 1]);
	}

}

Failover.prototype.failNode=function(node_id,backup_id, expect_success) {

	this.slonArray[node_id-1].stop();
	if(!this.slonArray[node_id-1].isFinished()) {
		this.coordinator.join(this.slonArray[node_id-1]);
	}
	
	
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'Failover.prototype.failNode\';\n';
	slonikScript += 'FAILOVER(id='  + node_id  + ',backup node=' + backup_id +');\n';
	for(var idx=1; idx <= this.getNodeCount();idx++) {
		if(idx == node_id ) {
			continue;
		}
		slonikScript += 'sync(id=' + idx + ");\n";
		
		for(var waitIdx=1; waitIdx < this.getNodeCount(); waitIdx++) {
			if(waitIdx==idx || waitIdx==node_id) {
				continue;
			}
			//autowaitfor			slonikScript += "echo 'waiting on " + idx + " " + waitIdx +"';\n";
			//autowaitfor			slonikScript += 'wait for event(origin=' + idx + ',wait on=' + idx + ',timeout=60,confirmed=' + waitIdx + ");\n";
		}
	}
		//+ 'sync(id=' + backup_id + ');\n'
		//+ 'wait for event(origin=' + backup_id+', wait on=' + backup_id + ',confirmed=all);\n'
		slonikScript+= 'echo \'the failover command has completed\';\n';

	var con = this.coordinator.createJdbcConnection('db' + backup_id);
	var stat = con.createStatement();
	var rs = stat.executeQuery("SELECT * FROM _disorder_replica.sl_path");
	while(rs.next()) {
		var line = rs.getString(1) + "," + rs.getString(2) + " ," + rs.getString(3);
		this.coordinator.log(line);
	}
	rs.close();
	var slonik = this.coordinator.createSlonik('fail node' , slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);	
	this.testResults.assertCheck('slonik failover status okay',slonik.getReturnCode()==0,expect_success);
	
	this.coordinator.log('subscribe list is now');
	rs = stat.executeQuery("SELECT * FROM _disorder_replica.sl_set");
	while(rs.next()) {
		var line = rs.getString(1) + "," + rs.getString(2) + " ," + rs.getString(3);
		this.coordinator.log(line);
	}
	rs.close();
	stat.close();
	con.close();
	
	
}


Failover.prototype.addCompletePaths = function() {
	var slonikPre = this.getSlonikPreamble();
	var slonikScript = 'echo \'Failover.prototype.addCompletePaths\';\n';
        for(var client=1; client <= this.getNodeCount(); client++) {
		for(var server=1; server <= this.getNodeCount(); server++) {
			if(server==client) {
				continue;
			}
			slonikScript+= 'store path(server=' + server + ',client=' + client +',conninfo=@CONNINFO' + server + ');\n';
		}
		//slonikScript+='sync(id=' + client+');\n';
		//slonikScript+='wait for event(origin=' + client + ',confirmed=all,wait on=' + client + ');\n';
	}
	
	var slonik = this.coordinator.createSlonik('add paths', slonikPre, slonikScript);
	
	
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('paths added okay', slonik.getReturnCode(), 0);
        this.coordinator.log("Failover.prototype.runTest - begin");
}

Failover.prototype.dropNode=function(node_id,event_node) {
	this.coordinator.log("Failover.prototype.dropNode - begin");
	var slonikPreamble = this.getSlonikPreamble();
	var slonikScript = 'echo \'Failover.prototype.dropNode\';\n';
	slonikScript += 'DROP NODE(id='  + node_id  + ',event node=' + event_node +');\n';
	for(var idx=1; idx <= this.getNodeCount(); idx++) {
		if(idx == node_id) {
			continue;
		}
		slonikScript +=  'wait for event(origin=' + event_node + ',confirmed=' + idx + ', wait on=' + event_node + ');\n';
	}
	var slonik = this.coordinator.createSlonik('drop node' , slonikPreamble,slonikScript);
	slonik.run();
	this.coordinator.join(slonik);
	this.testResults.assertCheck('slonik drop node status okay',slonik.getReturnCode(),0);
    this.coordinator.log("Failover.prototype.dropNode - complete");
}
