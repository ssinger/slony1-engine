subscribe set (id = 1, provider = 1, receiver = 3, forward = no);
create set (id=2, origin=2, comment='set subscribed by nodes 2,4,5');


set add table (id=1, set id=2, origin=2, fully qualified name = 'public.table1', comment='accounts table');
set add table (id=2, set id=2, origin=2, fully qualified name = 'public.table2', key='table2_id_key');
set add table (id=4, set id=2, origin=2, fully qualified name = 'public.table4', comment='a table of many types');
subscribe set (id = 2, provider = 2, receiver = 4, forward = no);
subscribe set (id = 2, provider = 2, receiver = 5, forward = no);
