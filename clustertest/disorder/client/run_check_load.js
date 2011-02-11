/**
 * The disorder.js file should be loaded/included before this script. 
 */

var g = new ClientGroup("disorder");

g.setNumWorkers(1);
g.setClassName("disorder");
g.setTransactionMix("trans_orderStatus" );

// ----
g.setDb(JDBC_URL, JDBC_USER, JDBC_PASSWORD);

g.launchNumTransactions(1);
