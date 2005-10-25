CLUSTER NAME = slony_regress1;
NODE 1 ADMIN CONNINFO = 'dbname=slonyregress1 host=localhost user=pgsql';
NODE 3 ADMIN CONNINFO = 'dbname=slonyregress3 host=localhost user=pgsql';
drop node (id = 2);

