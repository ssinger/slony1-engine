create table test1 (
       id1 int4 not null,
       id2 int4 not null,
       id3 int4,
       data text
);
create unique index test1_pkey on test1(id1);

insert into test1(id1,id2,id3,data) values (1,1,1,'test');
insert into test1(id1,id2,id3,data) values (2,2,1,'test2');


