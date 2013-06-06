set add table (id=1, set id=1, origin=1, fully qualified name = 'public.table1', comment='accounts table');
set add table (id=2, set id=1, origin=1, fully qualified name = 'public.table2', key='table2_id_key');
set add table (id=3, set id=1, origin=1, fully qualified name = 'public.table3');
   
set add table (set id = 1, origin = 1, id = 5, fully qualified name =
'public.evil_index_table', key = 'user', comment = 'Table with evil index name');

set add table (set id = 1, origin = 1, id = 6, fully qualified name ='"Schema.name".user', comment = 'Table with evil name - user, and a field called user');

set add table (set id = 1, origin = 1, id = 7, fully qualified name ='"Schema.name"."Capital Idea"', comment = 'Table with spaces in its name, caps, and a user field as PK');

set add table (set id = 1, origin = 1, id = 8, fully qualified name ='public."Jan''s bad ideas"', comment = 'Table single quote in name');
