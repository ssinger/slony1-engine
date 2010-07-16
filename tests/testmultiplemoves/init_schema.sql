-- 

create table table1 (id serial primary key, data text);
create table table2 (id serial primary key, data text);

create table table1a (id serial primary key, table1_id int4 references table1a(id) on update cascade on delete cascade, data text);
create table table2a (id serial primary key, table2_id int4 references table2a(id) on update cascade on delete cascade, data text);

