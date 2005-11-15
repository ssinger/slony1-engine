echo "
CLUSTER NAME = $CLUSTER1;
NODE 2 ADMIN CONNINFO = 'dbname=$DB2 host=$HOST2 user=$USER2 port=$PORT2 ';
NODE 3 ADMIN CONNINFO = 'dbname=$DB3 host=$HOST3 user=$USER3 port=$PORT3 ';
drop node (id = 2, event node = 1);
"