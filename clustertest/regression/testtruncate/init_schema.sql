create table products (
   id serial primary key,
   pname text unique not null,
   price numeric (12,2) not null
);

create table customers (
   id serial primary key,
   cname text unique not null
);

create table orders (
   id serial primary key,
   customer_id integer references customers(id) on delete restrict,
   order_date timestamptz not null default now(),
   order_value numeric (12,2)
);

create table line_items (
   order_id integer references orders(id) on delete cascade,
   product_id integer references products(id) on delete restrict,
   primary key (order_id, product_id),
   quantity integer
);

create table test_237 (
   id serial primary key,
   value text unique not null
);

insert into test_237 (value) 
values ('initial-1'), ('initial-2'), ('initial-3'), ('initial-4');

create or replace function mkproduct (i_pname text, i_price numeric(12,2)) returns integer as $$
begin
		insert into products(pname, price) values (i_pname, i_price);
        return 1;
end
$$ language plpgsql;

create or replace function mkcustomer (i_cname text) returns integer as $$
begin
		insert into customers(cname) values (i_cname);
        return 1;
end
$$ language plpgsql;

create or replace function mkorder (i_cname text) returns integer as $$
declare
   c_oid integer;
begin
		insert into orders (customer_id, order_value)
			   select id, 0 from customers where cname = i_cname
        returning id into c_oid;
		return c_oid;
end
$$ language plpgsql;

create or replace function order_line (i_order integer, i_product text, i_quantity integer) returns numeric (12,2) as $$
declare
		c_product integer;
		c_price numeric(12,2);
begin
		select id, price into c_product, c_price from products where pname = i_product;
		insert into line_items (order_id, product_id, quantity) values (i_order, c_product, i_quantity);
		update orders set order_value = order_value + (i_quantity * c_price) where id = i_order;
        return (select order_value from orders where id = i_order);
end
$$ language plpgsql;
