try {
    set add table (set id = 1, tables='public.*');
} on error {
    echo 'Expected failure - some of the tables in public are not replicable';
} on success {
    echo 'Oops - seems we added all the tables in public, which should not have worked';
    exit 1;
}

try {
    set add table (set id = 1, tables='public.table2');
} on error {
    echo 'Expected failure - table2 does not have a primary key';
} on success {
    echo 'Oops - table2 is not supposed to be successfully added!';
    exit 1;
}

try {
   set add table (set id = 1, tables='public.table[145]');
} on error {
   echo 'table1, table4, table5 should be replicable';
   exit 1;
}

try {
   set add table (set id = 1, tables='public.table[145]');
} on error {
   echo 'these tables are already replicated so we expect this to fail';
} on success {
    echo 'Oops - these tables should not be successfully added!';
    exit 1;
}

set add table (id=22, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');
