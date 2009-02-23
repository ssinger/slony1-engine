SET SEARCH_PATH TO @CLUSTERNAME@, pg_catalog;

BEGIN;
SET TRANSACTION ISOLATION LEVEL SERIALIZABLE ;

lock table sl_config_lock;

-- Redo stored triggers
create or replace function redo_table_triggers (i_tab_id integer) as $$
declare
	prec record;
	c_localnode integer;
	c_n integer;
begin
	c_localnode := getLocalNodeId('_@CLUSTERNAME@');
	select t.tab_reloid, t.tab_altered, t.tab_idxname, s.set_origin, pgx.indexrelid, 
	   slon_quote_brute(pgn.nspname) || '.' || slon_quote_brute(pgc.relname) as tab_fqname 
	   into prec
	from
		sl_table t, sl_set s, pg_class pgc, pg_namespace pgn,
		pg_index pgx, pg_class pgxc
	where
		t.tab_id = i_tab_id and
		t.tab_reloid = pgc.oid and
		pgc.relname = pgn.oid and
		pgx.indrelid = t.tab_reloid and
		pgx.indexrelid = pgxc.oid and
		pgxc.relname = t.tab_idxname
	for update;
	if not found then
		raise exception 'Table with id % not found!', i_tab_id;
	end if;
	if not prec.tab_altered then
		raise exception 'Table % - % - not in altered state', i_tab_id, prec.tab_fqname;
	end if;
	execute 'lock table ' || prec.tab_fqname || ' in access exclusive mode';

	execute 'create trigger "_@CLUSTERNAME@_logtrigger"' || 'after insert or update or delete on ' ||
		prec.tab_fqname || ' for each row execute procedure logTrigger(' ||
		quote_literal('_@CLUSTERNAME@') || ',' ||
		quote_literal(i_tab_id) || ',' ||
		quote_literal(determineAttKindUnique(prec.tab_fqname, prec.tab_idxname)) || ');';

	execute 'create trigger "_@CLUSTERNAME@_denyaccess"' ||
		' before insert or update or delete on ' || prec.tab_fqname ||
		' for each row execute procedure denyAccess(' || quote_literal('_@CLUSTERNAME@') || ');';

	if c_localnode := prec.set_origin then
		execute 'drop trigger "_@CLUSTERNAME@_logtrigger_' || i_tab_id || '" on ' || prec.tab_fqname;

		execute 'alter table ' || prec.tab_fqname || ' enable trigger "_@CLUSTERNAME@_logtrigger"';
		execute 'alter table ' || prec.tab_fqname || ' disable trigger "_@CLUSTERNAME@_denyaccess"';
	else
		execute 'drop trigger "_@CLUSTERNAME@_denyaccess_' || i_tab_id || '" on ' || prec.tab_fqname;
		update pg_trigger set tgrelid = prec.tab_reloid where tgrelid = prec.indexrelid;
		get diagnostics c_n = row_count;
		if c_n > 0 then
			update pg_class set reltriggers = reltriggers + c_n where oid = prec.tab_reloid;
		end if;
		update pg_rewrite set ev_class = prec.tab_reloid where ev_class = prec.indexrelid;
		get diagnostics c_n = row_count;
		if c_n > 0 then
			update pg_class set relhasrules = true where oid = prec.tab_reloid;
		end if;
		execute 'alter table ' || prec.tab_fqname || ' disable trigger "_@CLUSTERNAME@_logtrigger"';
		execute 'alter table ' || prec.tab_fqname || ' enable trigger "_@CLUSTERNAME@_denyaccess"';
	end if;
end
$$ language plpgsql;

select redo_table_triggers(tab_id) from sl_table;
drop function redo_table_triggers(tab_id integer);

alter table sl_node drop column no_spool;
drop table sl_trigger;

truncate sl_log_1, sl_log_2, sl_event, sl_setsync;

alter table sl_set drop column set_locked;
alter table sl_set add column set_locked bigint;
comment on column sl_set.set_locked is 'Transaction ID where the set was locked';

alter table sl_log_1 drop column log_xid;
alter table sl_log_2 drop column log_xid
alter table sl_log_1 add column log_txid bigint;
alter table sl_log_2 add column log_txid bigint;
comment on column sl_log_1.log_txid is 'Transaction ID on the origin node';
comment on column sl_log_2.log_txid is 'Transaction ID on the origin node';

alter table sl_event drop column ev_minxid;
alter table sl_event drop column ev_maxxid;
alter table sl_event add column ev_snapshot "pg_catalog".txid_snapshot;
comment on column sl_event.ev_snapshot is 'TXID in provider node for this event';

alter table sl_setsync add column ev_snapshot "pg_catalog".txid_snapshot;
comment on column sl_setsync.ev_snapshot is 'TXID in provider node seen by the event';

drop function alterTableForReplication(int4);
drop function pre74(int4);

select setval('sl_event_seq', 0);
select setval('sl_action', 0);

select createevent('_@CLUSTERNAME@', 'SYNC', NULL);

-- insert into sl_setsync(ssy_setid, ssy_origin, ssy_seqno, ssy_snapshot, ssy_action_list) values 
-- MUMBLE...  Need to grab a transaction ID from the source node...

prepare transaction '@CLUSTERNAME@ upgrade from Slony-I 1.2 to 2.0';