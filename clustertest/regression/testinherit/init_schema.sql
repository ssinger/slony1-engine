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

-- And a second, rather more complex instance, which does some automatic partitioning by region...
create table regions (
  region_code integer primary key,
  iso_country char(2),
  region text,
  city text
);
create unique index region_names on regions(iso_country, region, city);

create table products (
  product_id serial primary key,
  name text unique not null,
  price numeric(10,2) not null
);

create table sales_data (
  id serial primary key,
  trans_on timestamptz default 'now()',
  region_code integer not null references regions(region_code),
  product_id integer not null references products(product_id),
  quantity integer not null,
  amount numeric(12,2) not null
);

create table us_east (
) inherits (sales_data);
alter table us_east add primary key(id);
alter table us_east add constraint region_chk check (region_code between 1000 and 1999);
alter table us_east add constraint region_code foreign key(region_code) references regions(region_code);
alter table us_east add constraint product_code foreign key(product_id) references products(product_id);

create table us_west (
) inherits (sales_data);
alter table us_west add primary key(id);
alter table us_west add constraint region_chk check (region_code between 2000 and 2999);
alter table us_west add constraint region_code foreign key(region_code) references regions(region_code);
alter table us_west add constraint product_code foreign key(product_id) references products(product_id);

create table canada (
) inherits (sales_data);
alter table canada add primary key(id);
alter table canada add constraint region_chk check (region_code between 3000 and 3999);
alter table canada add constraint region_code foreign key(region_code) references regions(region_code);
alter table canada add constraint product_code foreign key(product_id) references products(product_id);

create rule sales_data_distribute_us_east as on insert to sales_data where new.region_code between 1000 and 1999
do instead insert into us_east (id, trans_on, region_code, product_id,quantity,amount) values (new.id, new.trans_on, new.region_code, new.product_id,new.quantity,new.amount);

create rule sales_data_distribute_us_west as on insert to sales_data where new.region_code between 2000 and 2999
do instead insert into us_west (id, trans_on, region_code, product_id,quantity,amount) values (new.id, new.trans_on, new.region_code, new.product_id,new.quantity,new.amount);

create rule sales_data_distribute_canada as on insert to sales_data where new.region_code between 3000 and 3999
do instead insert into canada (id, trans_on, region_code, product_id,quantity,amount) values (new.id, new.trans_on, new.region_code, new.product_id,new.quantity,new.amount);

-- We will be doing inserts into sales_data inside the following stored proc
create or replace function purchase_product (integer, integer, integer) returns numeric(12,2) as '
declare
   i_region alias for $1;
   i_product alias for $2;
   i_quantity alias for $3;
   c_price numeric(10,2);
   c_amount numeric(12,2);
begin
   select price into c_price from products where product_id = i_product;
   c_amount := c_price * i_quantity;
   insert into sales_data (region_code, product_id, quantity, amount) values (i_region, i_product, i_quantity, c_amount);
   return c_amount;
end' language plpgsql;


