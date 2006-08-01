create table master (
   id serial primary key,
   trans_on timestamptz default 'now()',
   data text
);

create table sub1 (
) inherits (master);
alter table sub1 add primary key(id);

create table sub2 (
) inherits (master);
alter table sub2 add primary key(id);

create table sub3 (
) inherits (master);
alter table sub3 add primary key(id);


