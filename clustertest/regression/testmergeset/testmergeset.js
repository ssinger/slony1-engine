var NUM_NODES=4;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testmergeset/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testmergeset/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';

}




function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.regions', comment='regions table');"
		+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.products', comment='product table');"
		+"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.sales_txns', comment='Global sales transactions');"                                                                                        

	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = yes);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=2, wait on=1);\n"
		+"subscribe set ( id = 1, provider = 1, receiver = 3, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=3, wait on=1);\n"
		+"subscribe set ( id = 1, provider = 2, receiver = 4, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=4, wait on=1);\n";
	return script;
}



function create_partition(year,month,cluster,coordinator) {
	
	nextmonth = month+1;
	nextmonthsyear=year;
	if(nextmonth >= 13) {
		nextmonth=1;
		nextmonthyear = year+1;
	}
	var sqlScript = "create table sales_txns_" + year + "_" + month 
		+ " ( check (trans_on >= '" + year + "-" + month + "-01' AND trans_on < '"
		+ nextmonthsyear + "-" + nextmonth+ "-01' ), primary key (id) )"
		+" inherits(sales_txns);\n";
	sqlScript += "create rule sales_" + year + "_"+ month + " as on insert to sales_txns where"
	+ " trans_on >= '" + year + "-" + month + "-01' AND trans_on < '" 
	+ nextmonthsyear + "-" + nextmonth + "-01' "
	+ " DO INSTEAD ("
	+ "INSERT INTO sales_txns_" + year + "_" + month+ " select new.id,new.trans_on,new.region_code" 
	+ " , new.product_id, new.quantity, new.amount;);\n";
	
	//Write the SQL out to a file, we will need to pass it to EXECUTE script.
	var ddlFile = java.io.File.createTempFile("mergeset","sql");
	ddlFile.deleteOnExit();
	var ddlWriter = new java.io.FileWriter(ddlFile);
	ddlWriter.write(sqlScript);
	ddlWriter.close();
	
	var slonikScript = "EXECUTE SCRIPT( filename='" + ddlFile.getAbsolutePath()
		+ "', EVENT NODE =1);\n";
	slonikScript += "sync(id=1);\n";
	slonikScript += "wait for event(origin=1,confirmed=all,wait on =1);\n";
	slonikScript += "create set(id=999,origin=1, comment='temp set for new partition');\n";
	slonikScript += "set add table(id=" + year;
	slonikScript += month + ",origin=1,set id =999"
		+ ", fully qualified name='public.sales_txns_" + year + "_" + month + "');\n";
	
	for(var idx=2; idx <=3; idx++) {
		slonikScript += "subscribe set(id=999, provider=1, receiver=" + idx + ", forward=yes);\n";
		slonikScript+=" sync(id=1);\n";
		slonikScript+= "wait for event(origin=1, confirmed=all, wait on=1);\n";
	}
	slonikScript += "subscribe set(id=999, provider=2, receiver=4, forward=yes);\n";
	slonikScript+=" sync(id=1);\n";
	slonikScript+= "wait for event(origin=1, confirmed=all, wait on=1);\n";
	
	slonikScript+= 'sleep(seconds=4);\n';
	slonikScript+="merge set(id=1, ADD ID=999, origin=1);\n";
	var slonikPreamble=get_slonik_preamble();
	run_slonik('subscribe partition',coordinator,slonikPreamble,slonikScript);
}



function do_test(coordinator) {
	
	var sqlScript='';
	for(var year=2006; year <=2006; year++) {
		for(var month=1; month <= 3; month++) {
			var num_rows = random_number(5,7);
			for(var row=0; row < num_rows; row++) {
				var quantity = random_number(1,9);
				var day = random_number(1,25);
				var hour = random_number(8,19); //normal hours, in london
				var minute=random_number(1,59);
				sqlScript += " SELECT purchase_product(region_code,product_id,(" +
					quantity + "+random()*3)::integer,'" +year + "-" + month + "-" + day
					+" " + hour  + ":" + minute + " GMT'::timestamp) FROM regions,"
					+" products order by random() limit 3;";
				
			}
			coordinator.log("Creating partitions for " + year + "-" + month)
			create_partition(year,month,properties.getProperty('clustername'),coordinator);
			coordinator.log("executing SQL for " + year + " month");
			sql = coordinator.createPsqlCommand('db1',sqlScript);
			sql.run();
			coordinator.join(sql);							
		}
		
	}
	coordinator.log("done");
	
	wait_for_sync(coordinator);
	
	
	
}

function get_compare_queries() {
	

	var queries=["select id, trans_on, quantity, amount from sales_txns order by id"
	             ,"select 'main', * from only sales_txns order by id"
	             ,"select '2006_1', * from sales_txns_2006_1 order by id"
	             ,"select '2006_2', * from sales_txns_2006_2 order by id"
	             ,"select '2006_3', * from sales_txns_2006_3 order by id"
 
	            ];

	return queries;
}

run_test(coordinator,'testmergeset');
