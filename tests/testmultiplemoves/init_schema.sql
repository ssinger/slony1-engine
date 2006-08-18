-- $Id: init_schema.sql,v 1.1 2006-08-18 18:46:25 cbbrowne Exp $

create table table1 (id serial primary key, data text);
create table table2 (id serial primary key, data text);
create table table3 (id serial primary key, data text);
create table table4 (id serial primary key, data text);
create table table5 (id serial primary key, data text);
create table table6 (id serial primary key, data text);
create table table7 (id serial primary key, data text);
create table table8 (id serial primary key, data text);
create table table9 (id serial primary key, data text);
create table tableA (id serial primary key, data text);

create table table1a (id serial primary key, table1_id int4 references table1a(id) on update cascade on delete cascade, data text);
create table table2a (id serial primary key, table2_id int4 references table2a(id) on update cascade on delete cascade, data text);
create table table3a (id serial primary key, table3_id int4 references table3a(id) on update cascade on delete cascade, data text);
create table table4a (id serial primary key, table4_id int4 references table4a(id) on update cascade on delete cascade, data text);
create table table5a (id serial primary key, table5_id int4 references table5a(id) on update cascade on delete cascade, data text);
create table table6a (id serial primary key, table6_id int4 references table6a(id) on update cascade on delete cascade, data text);
create table table7a (id serial primary key, table7_id int4 references table7a(id) on update cascade on delete cascade, data text);
create table table8a (id serial primary key, table8_id int4 references table8a(id) on update cascade on delete cascade, data text);
create table table9a (id serial primary key, table9_id int4 references table9a(id) on update cascade on delete cascade, data text);
create table tableAa (id serial primary key, tableA_id int4 references tableAa(id) on update cascade on delete cascade, data text);
