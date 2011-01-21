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


create table sales_txns (
   id serial primary key not null,
   trans_on timestamptz not null default 'now()',
   region_code integer not null references regions(region_code),
   product_id integer not null references products(product_id),
   quantity integer not null,
   amount numeric(12,2) not null
);

create or replace function sales_trig_ins () returns trigger as '
begin
   raise exception ''Missing sales_txns partition for date %'', new.trans_on;
end;
' language plpgsql;

create trigger sales_txn_ins before insert on sales_txns
    for each row execute procedure  sales_trig_ins();

create rule sales_txn_update as on update to sales_txns where new.trans_on <> old.trans_on
do instead (
   insert into sales_txns (id,trans_on,region_code,product_id,quantity,amount) values (new.id, new.trans_on, new.region_code, new.product_id, new.quantity, new.amount);
   delete from sales_txns where id = old.id;
);

-- We will be doing inserts into sales_txns inside the following stored proc
create or replace function purchase_product (integer, integer, integer, timestamptz) returns numeric(12,2) as '
declare
   i_region alias for $1;
   i_product alias for $2;
   i_quantity alias for $3;
   i_txndate alias for $4;
   c_price numeric(10,2);
   c_amount numeric(12,2);
begin
   select price into c_price from products where product_id = i_product;
   c_amount := c_price * i_quantity;
   insert into sales_txns (region_code, product_id, quantity, amount, trans_on) values (i_region, i_product, i_quantity, c_amount, i_txndate);
   return c_amount;
end' language plpgsql;

