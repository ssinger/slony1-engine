-- ----------
-- disorder-1.sql
--
--	Table and function definitions for the DIStributed ORDER system.
--	To make the initial data population easier, all foreign key
--	constraints are place in disorder-2.sql and created later.
-- ----------

drop schema disorder cascade;
create schema disorder;

CREATE LANGUAGE plpgsql;

GRANT ALL on SCHEMA disorder to PUBLIC;

set search_path to disorder, public, pg_catalog;

create table do_config (
	cfg_opt			text primary key,
	cfg_val			text not null
);

GRANT ALL ON TABLE do_config TO PUBLIC;

create table do_customer (
	c_id			bigserial primary key,
	c_name			varchar not null,
	c_total_orders	bigint not null default 0,
	c_total_value	numeric(17,2) not null default 0.0
);

GRANT ALL ON TABLE do_customer TO PUBLIC;
GRANT ALL ON SEQUENCE do_customer_c_id_seq TO PUBLIC;
create table do_item (
	i_id			bigserial primary key,
	i_name			varchar not null,
	i_price			numeric(17,2),
	i_in_production	boolean,
	i_critical		boolean
);

GRANT ALL ON TABLE do_item TO PUBLIC;
GRANT ALL ON SEQUENCE do_item_i_id_seq TO PUBLIC;

create table do_inventory (
	ii_id			bigint primary key,
	ii_in_stock		bigint,
	ii_reserved		bigint,
	ii_total_sold	bigint
);

GRANT ALL ON TABLE do_inventory TO PUBLIC;


create table do_restock (
	r_id			bigserial primary key,
	r_i_id			bigint,
	r_quantity		bigint
);
create index do_restock_item_idx on do_restock (r_i_id);
GRANT ALL ON TABLE do_restock TO PUBLIC;
GRANT ALL ON SEQUENCE do_restock_r_id_seq TO PUBLIC;

create table do_order (
	o_id			bigserial primary key,
	o_c_id			bigint not null,
	o_order_lines	bigint not null,
	o_total_items	bigint not null,
	o_total_value	numeric(17,2) not null,
	o_shipped		boolean
);
create index do_order_customer_idx on do_order (o_c_id, o_id);
create index do_order_shipped_idx on do_order (o_shipped, o_id);
GRANT ALL ON TABLE do_order TO PUBLIC;
GRANT ALL ON SEQUENCE do_order_o_id_seq TO PUBLIC;


create table do_order_line (
	ol_o_id			bigint not null,
	ol_line_no		bigint not null,
	ol_i_id			bigint,
	ol_i_name		varchar not null,
	ol_quantity		bigint not null,
	ol_value		numeric(17,2) not null,

	--unique (ol_o_id, ol_line_no)
	PRIMARY KEY (ol_o_id,ol_line_no)
);
create index do_order_line_item_idx on do_order_line (ol_i_id);
create index do_order_line_o_id_item_idx on do_order_line (ol_o_id, ol_i_id);
GRANT ALL ON TABLE do_order_line TO PUBLIC;


create table do_item_review (
	ir_id bigserial not null,
	i_id bigint not null,
	comments text,
	PRIMARY KEY (ir_id)
);
GRANT ALL ON TABLE do_item_review TO PUBLIC;
GRANT ALL ON SEQUENCE do_item_review_ir_id_seq TO PUBLIC;


create view do_order_view as select
	   * FROM do_order_line, do_order,
	   	 do_item
	   where do_item.i_id=do_order_line.ol_i_id
	   and do_order.o_id=do_order_line.ol_o_id;
	   

-- ----
-- random(x,y)
--
--	Returns a uniform random bigint in the range x..y
-- ----
create function random(x bigint, y bigint)
returns bigint as $$
begin
	return floor(pg_catalog.random() * (y - x + 1)::real)::bigint + x;
end;
$$ language plpgsql;


-- ----
-- nurand(A,x,y)
--
--	An implementation of the TPC NURand() function.
-- ----
create function nurand(a bigint, x bigint, y bigint)
returns bigint as $$
begin
	return ((random(0, a) | random(x, y)) % (y - x  + 1)) + x;
end;
$$ language plpgsql;


-- ----
-- digsyl(n, l)
--
--	An implementation of the TPC digsyl() function.
-- ----
create function digsyl(n bigint, l integer)
returns varchar as $$
declare
	result		varchar := '';
	i			integer;
	digits		text;
	dig			integer;
	fmt			text := '';
	syl			text = 'BA OG AL RI RE SE AT UL IN NG';
begin
	while length(fmt) < l loop
		fmt = fmt || '0';
	end loop;

	digits = to_char(n, fmt);
	-- raise notice 'digsyl: fmt=% digits=%', fmt, digits;

	for i in 2 .. l + 1 loop
		dig = to_number(substring(digits, i, 1), '9');
		result = result || substring(syl, dig * 3 + 1, 2);
	end loop;

	return result;
end;
$$ language plpgsql;


-- ----
-- select_nurand_a(n)
--
--	Determine the correct value for A in NURand according to the TPC-W
--	specs. Except for very small ranges, where we use 15.
-- ----
create function select_nurand_a(n bigint)
returns bigint as $$
begin
	if n >= 10000000 then
		return 524287;
	end if;
	if n >= 1000000 then
		return 32767;
	end if;
	if n >= 100000 then
		return 4095;
	end if;
	if n >= 10000 then
		return 511;
	end if;
	if n >= 1000 then
		return 63;
	end if;
	return 15;
end;
$$ language plpgsql;


create function populate(scaling bigint)
returns integer as $$
declare
	n_customer		bigint;
	a_customer		bigint;
	n_item			bigint;
	a_item			bigint;
	n_special		bigint;
	n_order			bigint;

	line_no			bigint;
	num_lines		bigint;
	order_id		bigint;
	item_id			bigint;
	quantity		bigint;
	n				integer;
	row				record;
begin
	-- ----
	-- Select ranges and nurand-A numbers according to scaling
	-- ----
	n_customer = 100 * scaling;
	a_customer = select_nurand_a(n_customer);
	n_item = 400 + 100 * scaling;
	a_item = select_nurand_a(n_item);
	n_special = 200;
	n_order = n_customer * 100;

	-- ----
	-- Store the configuration values in the config table
	-- ----
	insert into disorder.do_config (cfg_opt, cfg_val) values
		('scaling', scaling::text),
		('n_customer', n_customer::text),
		('a_customer', a_customer::text),
		('n_item', n_item::text),
		('a_item', a_item::text),
		('n_special', n_special::text);

	-- ----
	-- Populate the ITEM table first, so that we have the prices for
	-- orders and order lines.
	-- ----
	insert into disorder.do_item (i_name, i_price, i_in_production, i_critical)
		select 'Item ' || digsyl(random(0, 99999999), 8), 
				random(99, 99999)::numeric / 100.0,
				true, true
			from (select generate_series(1, n_item)) as S;
	analyze disorder.do_item;

	-- ----
	-- With the items in place we create all the order lines
	-- ----
	for order_id in 1 .. n_order loop
		num_lines = random(5, 10);

		for line_no in 1 .. num_lines loop
			item_id = nurand(a_item, 1, n_item);
			quantity = random(3, 20);
			insert into disorder.do_order_line
				(ol_o_id, ol_line_no, ol_i_id, ol_i_name, ol_quantity, ol_value)
				select order_id, line_no, i_id, i_name, quantity, i_price * quantity
					from (select i_id, i_name, i_price
						from disorder.do_item 
						where i_id = item_id
					) as item;
		end loop;
	end loop;
	analyze disorder.do_order_line;

	-- ----
	-- Build the order heads from that
	-- ----
	insert into disorder.do_order
			(o_id, o_c_id, o_order_lines, o_total_items, o_total_value,
			 o_shipped)
		select ol_o_id, nurand(a_customer, 1, n_customer),
				ol_num_lines, ol_total_items, ol_total_value,
				case when ol_o_id <= n_special then false
					 when random(0, 9) % 10 = 0 then false
					 else true
				end
			from (select ol_o_id, count(*) as ol_num_lines,
					sum(ol_quantity) as ol_total_items,
					sum(ol_value) as ol_total_value
				from disorder.do_order_line
				group by ol_o_id
			) as OL;
	perform setval('do_order_o_id_seq', n_order);
	analyze disorder.do_order;

	-- ----
	-- Get all the customers built
	-- ----
	insert into disorder.do_customer 
			(c_id, c_name, c_total_orders, c_total_value)
		select id, 
				digsyl(random(0, 999), 3) || ' ' || digsyl(random(0,9999), 4),
				coalesce((select count(*) from do_order where o_c_id = id), 0),
				coalesce((select sum(o_total_value) from do_order
						where o_c_id = id), 0.0)
			from (select generate_series(1, n_customer) as id) as C;
	perform setval('do_customer_c_id_seq', n_customer);
	analyze disorder.do_customer;

create temp table inv_order_quantity (
	t_id			bigint primary key,
	t_not_shipped	bigint,
	t_quantity		bigint
);
insert into inv_order_quantity (t_id, t_not_shipped, t_quantity)
	select ol_i_id, 
		sum(case when o_shipped then 0 else ol_quantity end),
		sum(ol_quantity)
		from disorder.do_order_line
		join disorder.do_order on o_id = ol_o_id
		group by 1;


	-- ----
	-- Create the inventory
	-- ----
	insert into disorder.do_inventory
			(ii_id, ii_in_stock, ii_reserved, ii_total_sold)
		select i_id, 
			coalesce(t_not_shipped, 0)
				+ case when i_in_production then
						random(100,200)
					else
						random(1, 50) * random(0, 2)
				end,
			coalesce(t_not_shipped, 0),
			coalesce(t_quantity, 0)
			from disorder.do_item
			left join inv_order_quantity on t_id = i_id;
	analyze disorder.do_inventory;
	
	-- ----
	-- Finally create all the restock rows
	-- ----
	insert into disorder.do_restock
			(r_i_id, r_quantity)
		select ii_id, ii_in_stock + coalesce(sum(ol_quantity), 0.0)
			from disorder.do_inventory 
			left join disorder.do_order_line on ol_i_id = ii_id
			left join do_order on o_id = ol_o_id
			where o_shipped
			group by ii_id, ii_in_stock;
	analyze disorder.do_restock;

	return 1;
end;
$$ language plpgsql;

