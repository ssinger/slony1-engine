set add table(id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');
set add table(id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');
table add key (node id = 1, fully qualified name = 'public.table3');
set add table(id=3, set id=1, origin=1, fully qualified name = 'public.table3', key = SERIAL);

