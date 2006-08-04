set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');
set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');

table add key (node id = 1, fully qualified name = 'public.table3');
set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3', key = SERIAL);

try {
   set add table (id=4, set id=1, origin=1, fully qualified name = 'public.table4', key = 'no_good_candidate_pk', comment='bad table - table 4');
} on error {
   echo 'Tried to replicate table4 with no good candidate PK - rejected';
} on success {
   echo 'Tried to replicate table4 with no good candidate PK - accepted';
   exit 1;
}

set add table (id=4, set id=1, origin=1, fully qualified name = 'public.table5', comment='a table of many types');