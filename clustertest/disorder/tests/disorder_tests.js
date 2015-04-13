coordinator.includeFile('disorder/tests/UnsubscribeBeforeEnable.js');
coordinator.includeFile('disorder/tests/EmptySet.js');
coordinator.includeFile('disorder/tests//OmitCopy.js');
coordinator.includeFile('disorder/tests/SubscribeUnderLoad.js');
coordinator.includeFile('disorder/tests/MoveSet.js');
coordinator.includeFile('disorder/tests/CloneNode.js');
coordinator.includeFile('disorder/tests/AddPathsAfterSubscribe.js');
coordinator.includeFile('disorder/tests/SlonKilling.js');
coordinator.includeFile('disorder/tests/InitialCopyFail.js');
coordinator.includeFile('disorder/tests/FailNodeTest.js');
coordinator.includeFile('disorder/tests/ExecuteScript.js');
coordinator.includeFile('disorder/tests/Failover.js');
coordinator.includeFile('disorder/tests/DropPath.js');
coordinator.includeFile('disorder/tests/DropSet.js');
coordinator.includeFile('disorder/tests/LogShipping.js');
coordinator.includeFile('disorder/tests/HeavyLoadTest.js');
coordinator.includeFile('disorder/tests/Unsubscribe.js');
coordinator.includeFile('disorder/tests/RestartTest.js');
coordinator.includeFile('disorder/tests/MultipleOrigins.js');
coordinator.includeFile('disorder/tests/BigBacklog.js');
coordinator.includeFile('disorder/tests/LongTransaction.js');
coordinator.includeFile('disorder/tests/RenameTests.js');
coordinator.includeFile('disorder/tests/CleanupTest.js');
coordinator.includeFile('disorder/tests/RecreateSet.js');
coordinator.includeFile('disorder/tests/MergeSet.js');
coordinator.includeFile('disorder/tests/BulkAddingTest.js');
coordinator.includeFile('disorder/tests/WaitForTest.js');
coordinator.includeFile('disorder/tests/MultinodeFailover.js');
coordinator.includeFile('disorder/tests/Resubscribe.js');
coordinator.includeFile('disorder/tests/SiteFailover.js');
coordinator.includeFile('disorder/tests/DropNode.js');
coordinator.includeFile('disorder/tests/CleanupInterval.js');

var tests = 
    [new EmptySet(coordinator,results)
     ,new OmitCopy(coordinator,results)
     ,new SubscribeUnderLoad(coordinator,results)
     ,new MoveSet(coordinator,results)
     ,new CloneNode(coordinator,results)
     ,new AddPathsAfterSubscribe(coordinator,results)
     ,new SlonKilling(coordinator,results)
     ,new InitialCopyFail(coordinator,results)
     ,new FailNodeTest(coordinator,results) //fail, bug133
     ,new DropPath(coordinator,results)

     ,new ExecuteScript(coordinator,results) //compare failures
     ,new Failover(coordinator,results) //bug136 related
     ,new LogShipping(coordinator,results)
     ,new HeavyLoadTest(coordinator,results)
     ,new Unsubscribe(coordinator,results)
     ,new RestartTest(coordinator,results)
     ,new MultipleOrigins(coordinator,results)
     ,new BigBacklogTest(coordinator,results)
     ,new LongTransaction(coordinator,results)
     ,new RenameTests(coordinator,results)
     ,new MergeSet(coordinator,results)
     ,new BulkAddingTest(coordinator,results)
	 ,new WaitForTest(coordinator,results)
	 ,new MultinodeFailover(coordinator,results)
	 ,new Resubscribe(coordinator,results)
	 ,new SiteFailover(coordinator,results)
	 ,new DropNode(coordinator,results)
	 ,new CleanupInterval(coordinator,results)
	 //Below tests are known to fail.
	 //,new UnsubscribeBeforeEnable(coordinator,results)
     //,new DropSet(coordinator,results) //fails bug 133
     //,new CleanupTest(coordinator,results) //cleanup_interval does not (yet) do what the test wants
    ];

//tests=[new Failover(coordinator,results),
//	   new MultinodeFailover(coordinator,results)
//	   ,new SiteFailover(coordinator,results)];

var basicTest = new BasicTest(coordinator,results);

//Setup the schema.
basicTest.prepareDb(['db1','db2','db3','db4','db5']);
var seed=basicTest.seedData(1);
coordinator.join(seed);
basicTest.postSeedSetup(['db1','db2','db3','db4','db5'])
for(var idx=0; idx < tests.length; idx++) {
	coordinator.log("DESCRIPTION:" + tests[idx].testDescription);
	tests[idx].runTest();
	tests[idx].teardownSlony();
}

basicTest.dropDb(['db1','db2','db3','db4','db5']);
