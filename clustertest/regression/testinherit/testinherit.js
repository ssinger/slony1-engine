var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testinherit/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testinherit/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';


}

function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.sub1', comment='sub table 1');\n"
		+"set add table (id=2, set id=1, origin=1, fully qualified name = 'public.sub2', comment='sub table 2');\n"
		+"set add table (id=3, set id=1, origin=1, fully qualified name = 'public.sub3', comment='sub table 3');\n"
		+"set add table (id=4, set id=1, origin=1, fully qualified name = 'public.master', comment='master table');\n"
		+"\n"
		+"set add table (id=5, set id=1, origin=1, fully qualified name = 'public.regions', comment='lookup table');\n"
		+"set add table (id=6, set id=1, origin=1, fully qualified name = 'public.sales_data', comment='master sales table');\n"
		+"set add table (id=7, set id=1, origin=1, fully qualified name = 'public.us_east', comment='Eastern US');\n"
		+"set add table (id=8, set id=1, origin=1, fully qualified name = 'public.us_west', comment='Western US');\n"
		+"set add table (id=9, set id=1, origin=1, fully qualified name = 'public.canada', comment='All of Canada');\n";
                                                                                                          

	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);\n"
		+"sync(id=1);\n"
		+"wait for event (origin=1, confirmed=2, wait on=1);\n";
	return script;
}


var subTxnPsqls=[];
function generate_data() {
	var sqlScript='';
	
	var numrows=random_number(50,90);
	for(var idx = 0; idx < numrows; idx++) {
		var textlen = random_number(1,100);
		var txta=random_string(textlen);
		txta = new java.lang.String(txta).replace("\\","\\\\");
		textlen = random_number(1,100);
		var txtblen = random_number(1,100);
		var txtb = random_string(txtblen);		
		txtb = new java.lang.String(txtb).replace("\\","\\\\");
		var txtclen = random_number(1,100);
		var txtc = random_string(txtclen);
		txtc = new java.lang.String(txtc).replace("\\","\\\\");
		var txtdlen = random_number(1,100);
		var txtd = random_string(txtdlen);
		txtc = new java.lang.String(txtc).replace("\\","\\\\");
		sqlScript += "INSERT INTO sub1(data) VALUES (E'sub1 "+txta+"');\n";
		sqlScript+="INSERT INTO sub2(data) VALUES (E'sub2 "+txtb+"}');\n";
		sqlScript+= "INSERT INTO sub3(data) VALUES (E'sub3 "+txtc+"');\n";
		sqlScript+= "INSERT INTO master(data) VALUES (E'master "+txtd+"');\n";
		sqlScript+="select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;\n";
	}

	var subTransaction=
		  SUBTRANSACTIONQUERY="begin;" 			  
			  +"select product_id into temp table products_foo from products order by random() limit 10;\n"
			  +"select region_code into temp table regions_foo from regions order by random() limit 10;\n"
			  +"savepoint a;\n"
			  +"select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from products_foo, regions_foo order by random() limit 10;\n"
			  +"select pg_sleep(2);\n"
			  +"savepoint b;\n"
			  +"select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from products_foo, regions_foo order by random() limit 5;\n"
			  +"rollback to savepoint b;\n"
			  +"savepoint c;\n"
			  "select product_id into temp table products_bar from products order by random() limit 5;\n"
			  +"select region_code into temp table regions_bar from regions order by random() limit 5;\n"
			  +"savepoint d;\n"
			  +"select pg_sleep(2);\n"
			  +"select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from products_bar, regions_bar order by random() limit 5;\n"
			  +"rollback to savepoint d;\n"
			  +"savepoint e;\n"			  		
			  +"select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from products_bar, regions_bar order by random() limit 10;\n"
			  +"select pg_sleep(2);\n"
			  +"commit;"
			  
	for(var idx=0; idx < 16; idx ++) {
		subTxnPsqls[idx] = coordinator.createPsqlCommand('db1',subTransaction);
		subTxnPsqls[idx].run();
	}
	return sqlScript;
}


function exec_ddl(coordinator) {
	var slonikScript = "EXECUTE SCRIPT(SET ID=1, FILENAME='regression/testdeadlockddl/ddl_updates',EVENT NODE=1);\n";
	var preamble = get_slonik_preamble();
	
	run_slonik('exec ddl',coordinator,preamble,slonikScript);
	
}


function do_test(coordinator) {

	var numrows = random_number(150,350);
	var trippoint = numrows / 20;
	var percent=0;
	
	var sqllScript='';
	sql = generate_data();
	psql = coordinator.createPsqlCommand('db1',sql);
	psql.run();
	
	coordinator.join(psql);
	for(var idx=0; idx < subTxnPsqls.length; idx++) {
		coordinator.join(subTxnPsqls[idx]);
	}
	
	wait_for_sync(coordinator);
	
	
	
}

function get_compare_queries() {
	

	var queries=["select id, trans_on, data from master order by id"
	             ,"select 'main', * from only sales_data order by id"
	             ,"select 'us west', * from us_west order by id"
	             ,"select 'us east', * from us_east order by id"
	             ,"select 'canada', * from canada order by id"
 
	            ];

	return queries;
}

run_test(coordinator,'testinherit');
