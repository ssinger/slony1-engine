echo <<EOF
CLUSTER NAME = $CLUSTER1
NODE 1 ADMIN CONNINFO = 'dbname=$DB1 host=$HOST1 user=$USER1 ';
NODE 3 ADMIN CONNINFO = 'dbname=$DB3 host=$HOST3 user=$USER3 ';
drop node (id = 2);
EOF


