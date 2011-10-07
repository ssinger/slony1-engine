var NUM_NODES=2;
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
    var sqlScript = coordinator.readFile('regression/testtruncate/init_schema.sql');
    return sqlScript;
}

function load_data(coordinator) {
    var sqlScript = coordinator.readFile('regression/testtruncate/init_data.sql');
    psql = coordinator.createPsqlCommand('db1',sqlScript);
    psql.run();
    coordinator.join(psql);
}

function init_cluster() {
    return 'init cluster(id=1, comment=\'Regress test node\');\n';
}

function init_tables() {
    var tables = ["products", "customers", "orders", "line_items"];
    var script = "";
    var tnum = 0;
    for (var i=0; i < tables.length; i++) {
	var table = tables[i];
	tnum = tnum + 1;
	script = script + "set add table (id = " + tnum + ", set id=1, origin=1, fully qualified name='public." + table + "', comment = 'test table');\n";
    }
    script = script + "set add table (id = 237, set id=1, origin=1, fully qualified name='public.test_237', comment = 'table to exercise bug 237 fix');\n";
    return script;
}
	

function create_set() {
    return 'create set (id=1, origin=1,comment=\'All testtruncate tables\'); \n';
}

function subscribe_set() {
    var script=	"subscribe set (id = 1, provider = 1, receiver = 2, forward = no);\n";
    return script;
}

var gdf_it = 0;  // Iteration counter
function generate_data_file(coordinator) {
    gdf_it = gdf_it + 1;
    var file = java.io.File.createTempFile("testdata",".sql");
    file.deleteOnExit();
    var fileWriter = new java.io.FileWriter(file);

    // Set up a bunch of products and customers
    for (var i = 1; i <= 20; i++) {
	var pn = random_number(1,10000);
	var cn = random_number(1,10000);
	var price = random_number(5,100);
	fileWriter.write ("select mkproduct('p" + pn + i + "', " + price + ");\n");
	fileWriter.write ("select mkcustomer('c" + cn + i + "');\n");
    }

    // Set up a temp table consisting of a random set of customers
    // and generate orders for random products for those customers
    for (var i=0; i< 20; i++) {
	fileWriter.write ("drop table if exists t_order;\n");
	fileWriter.write ("select mkorder (cname) onum into temp table t_order \n  from (select cname from customers order by random() limit 1) as foo;\n");
	var numprods = random_number(3,10);
	fileWriter.write ("select order_line (onum, pname, (random()*15+3)::integer) from t_order, (select pname from products order by random() limit " + numprods + ") as foo;\n");
    }

    // Populate some data into test_237
    var trvalues = ["this", "that", "other"];
    for (var i = 0; i < trvalues.length; i++) {
	var s = trvalues[i];
	fileWriter.write ("insert into public.test_237(value) values ('" + s + gdf_it + "');\n");
    }
    
    fileWriter.close();
    return file;
}

function run_sample_load (coordinator) {
    var file = generate_data_file(coordinator);
    psql = coordinator.createPsqlCommand('db1',file);
    psql.run();
    coordinator.join(psql);
    wait_for_sync(coordinator);
}


function do_test(coordinator) {
    var sqlScript='';
    do_compare(coordinator);

    run_sample_load(coordinator);

    var TruncateList = ["truncate-basic", "truncate-cascade", "truncate-multiple"];

    for (var i=0 ; i < TruncateList.length; i++) {
	var sqlScript = coordinator.readFile('regression/testtruncate/' + TruncateList[i] + '.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);

	run_sample_load(coordinator);
    }

    // drop test_237 out of replication
    var slonikScript = "set drop table (origin=1, id=237);";
    var preamble = get_slonik_preamble();
    run_slonik('Drop test_237 table', coordinator, preamble, slonikScript);

    wait_for_sync(coordinator);

    // truncate test_237 on both db1 and db2
    var dblist = ["db1", "db2"];
    for (var i=0 ; i < dblist.length; i++) {
	var sqlScript = coordinator.readFile('regression/testtruncate/truncate-237.sql');
	psql = coordinator.createPsqlCommand(dblist[i],sqlScript);
	psql.run();
	coordinator.join(psql);
    }    

    wait_for_sync(coordinator);
    coordinator.log("done");
}

function get_compare_queries() {
    var queries = 
	["select id,pname, price from products order by id",
	 "SELECT id,cname from customers order by id",
	 "SELECT id,customer_id, order_date, order_value from orders order by id",
	 "SELECT order_id,product_id,quantity from line_items order by order_id,product_id",
	 "select id,value from test_237 order by id"];
    return queries;
}

run_test(coordinator,'testtruncate');
