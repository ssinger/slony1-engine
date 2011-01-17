-- ----------
-- disorder-2.sql
--
--	Finish the schema by creating all foreign key constraints
--	and triggers.
-- ----------

alter table disorder.do_item
	add constraint do_item_inventory_ref
	foreign key (i_id)
	references disorder.do_inventory (ii_id)
	initially deferred;

alter table disorder.do_inventory
	add constraint do_inventory_item_ref
	foreign key (ii_id)
	references disorder.do_item (i_id)
	on delete cascade;

create function do_item_ins () returns trigger
as $$
begin
	insert into disorder.do_inventory
			(ii_id, ii_in_stock, ii_reserved, ii_total_sold)
		values
			(NEW.i_id, 0, 0, 0);
	return NEW;
end;
$$ language plpgsql;

create trigger do_item_ins after insert on disorder.do_item
	for each row execute procedure do_item_ins();

alter table disorder.do_restock
	add constraint do_restock_inventory_ref
	foreign key (r_i_id)
	references disorder.do_inventory (ii_id)
	on delete cascade;

alter table disorder.do_order
	add constraint do_order_customer_ref
	foreign key (o_c_id)
	references disorder.do_customer (c_id);

alter table disorder.do_order_line
	add constraint do_order_line_order_ref
	foreign key (ol_o_id)
	references disorder.do_order (o_id)
	on delete cascade;

alter table disorder.do_order_line
	add constraint do_order_line_item_ref
	foreign key (ol_i_id)
	references disorder.do_item (i_id)
	on delete set null;

