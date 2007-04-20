alter table table4 add column newcol timestamptz;
alter table table4 alter column newcol set default now();
alter table table4 add column newint integer;
update table4 set newint = 42;
