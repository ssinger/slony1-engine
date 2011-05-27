drop index test1_pkey;
create unique index test1_pkey on test1(id1,id2);
update test1 set id1=1;

