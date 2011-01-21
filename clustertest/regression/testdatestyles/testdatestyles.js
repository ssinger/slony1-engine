var NUM_NODES=2;
 
coordinator.includeFile('regression/common_tests.js');

function get_schema() {
	var sqlScript = coordinator.readFile('regression/testdatestyles/init_schema.sql');
	return sqlScript;
	
}
function load_data(coordinator) {
	var sqlScript = coordinator.readFile('regression/testdatestyles/init_data.sql');
	psql = coordinator.createPsqlCommand('db1',sqlScript);
	psql.run();
	coordinator.join(psql);
	
}

function init_cluster() {
	return 'init cluster(id=1, comment=\'Regress test node\');\n';


}

function init_tables() {
	var script="set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='big date table');\n";
	return script;
}

function create_set() {
	return 'create set (id=1, origin=1,comment=\'All test1 tables\'); \n';

}

function subscribe_set() {
	var script=	"subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);\n";
		return script;
}


function generate_data() {
	var sqlScript='';
	var dateStyles=['DMY', 
	                'European',
	                'German',
	                'ISO',
	                'MDY',
	                'NonEuropean',
	                'Postgres',
	                'SQL',
	                 'US',	                
	                'YMD'];
	
	
	  for(var idx = 0; idx < dateStyles.length; idx++) {
		  var datestyle = dateStyles[idx];
		  	
		    // status "generating transactions of date data for datestyle ${datestyle}"
		   sqlScript+="SET DATESTYLE TO "+datestyle+";\n";
		   sqlScript+= "insert into table1(ts, tsz, ds) values ('infinity', 'infinity', now());\n";
		   sqlScript+="insert into table1(ts, tsz, ds) values ('-infinity', '-infinity', now());\n";
		     // Final second of last year
		   TS="(date_trunc('years', now())-'1 second'::interval)";
		   sqlScript+="insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Final second of last month
		   TS="(date_trunc('months', now())-'1 second'::interval)";
		   sqlScript+="insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Sorta February 29th...
		   TS="(date_trunc('years', now())+'60 days'::interval)";
		   sqlScript+= "insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Sorta February 29th...
		   TS="(date_trunc('years', now())+'60 days'::interval+'1 year'::interval)";
		   sqlScript+= "insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Sorta February 29th...
		   TS="(date_trunc('years', now())+'60 days'::interval+'2 years'::interval)";
		   sqlScript+="insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Sorta February 29th...
		   TS="(date_trunc('years', now())+'60 days'::interval+'3 years'::interval)";
		   sqlScript+= "insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Sorta February 29th...
		   TS="(date_trunc('years', now())+'60 days'::interval+'4 years'::interval)";
		   sqlScript+= "insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		   // Now
		   TS="now()";
		   sqlScript+="insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";

		   // Now, go for a period of 1000 days, and generate an entry for each
		   // day.
		   for(var d1=0;d1 < 10;d1++) {
		       for(var d2=0; d2<10;d2++) {
		          for(var d3=0; d3 < 10; d3++) {
		             var NUM=d1+d2+d3;
		             TS="(date_trunc('years',now())+'"+NUM+" days'::interval)";
		             sqlScript+= "insert into table1(ts, tsz, ds) values ("+TS+", "+TS+", "+TS+");\n";
		          }
		       }
		   }     
		    for(var u1=0; u1 < 10; u1++) {
		        for(var inc=0; inc <=10; inc++) {
		           sqlScript+= "update table1 set ts = ts - '5 days'::interval + '"+inc+" days'::interval,"
		                                   +"tsz = tsz - '5 days'::interval + '"+inc+" days'::interval,"
		                                   +"ds = ds - '5 days'::interval + '"+inc+" days'::interval\n"
		                 +"where id in (select id from table1 order by random() limit 5);\n";
		        }
		    }
	  }//dateStyle	
	return sqlScript;
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
	wait_for_sync(coordinator);
	
	
}

function get_compare_queries() {
	var queries=['SELECT id,ts,tsz,ds,nowts,nowtsz,nowds FROM table1 ORDER BY id'];

	return queries;
}

run_test(coordinator,'testdatestyles');
