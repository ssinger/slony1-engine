create sequence t1seq;
alter table table1 add column seqed integer;
alter table table1 alter column seqed set default nextval('t1seq');
update table1 set seqed = nextval('t1seq');
alter table table1 add constraint seqed_unique UNIQUE(seqed);
