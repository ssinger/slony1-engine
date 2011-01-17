/**
 * The disorder.js file should be loaded/included before this script. 
 */

var g = new ClientGroup("disorder");

//g.setNumWorkers(1);
g.setClassName("disorder");
//g.setTransactionMix("trans_restock,trans_orderCreaten=10,trans_orderShip");
g.setSleep(50,1000,500)
g.setTransactionMix(  "trans_orderCreate=50,"+
       "trans_orderStatus=50,"+
        "trans_orderShip=5,"+
        "trans_restock=1,"+
        "trans_orderUpdate=0,"+
        "trans_itemCreate=3,"+
        "trans_itemDelete=1,"+
        "trans_recurringItem=0,"+
        "trans_checkCustomer=5,"+
        "trans_adjustCounts=1");

///g.setTransactionMix("trans_orderCreate,trans_restock" );
// ----
g.setDb(JDBC_URL, JDBC_USER, JDBC_PASSWORD);

//g.launchNumTransactions(10);
g.launch();