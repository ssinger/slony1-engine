alter table table4 add column newcol timestamptz;
alter table table4 alter column newcol set default now();
alter table table4 add column newint integer;
update table4 set newint = 42;
-- The following tries to exercise any cases where DDL statements might be evaluated by printf()
update table1 set data = data || '%d%s%s%s%s%f%l%d%s%f%p%d%f' where id in (select id from table1 order by id desc limit 150);