-- ----------------------------------------------------------------------
-- slony1_funcs.v83.sql
--
--    Version 8.3 specific part of the replication support functions.
--
--	Copyright (c) 2007-2009, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- 
-- ----------------------------------------------------------------------

-- ----------------------------------------------------------------------
-- FUNCTION ShouldSlonyVacuumTable (nspname, tabname)
--
--	Returns 't' if the table needs to be vacuumed by Slony-I
--      Returns 'f' if autovac handles the table, so Slony-I should not
--                  or if the table is not needful altogether
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.ShouldSlonyVacuumTable (i_nspname name, i_tblname name) returns boolean as
$$
declare
	c_table oid;
	c_namespace oid;
	c_enabled boolean;
	v_dummy int4;
begin
	if not exists (select 1 from pg_catalog.pg_class c, pg_catalog.pg_namespace n 
                   where c.relname = i_tblname and n.nspname = i_nspname and c.relnamespace = n.oid) then
        return 'f'::boolean;   -- if table does not exist, then don't vacuum
	end if;				
	select 1 into v_dummy from "pg_catalog".pg_settings where name = 'autovacuum' and setting = 'on';
	if not found then
		return 't'::boolean;       -- If autovac is turned off, then we gotta vacuum
	end if;
	
	select into c_namespace oid from "pg_catalog".pg_namespace where nspname = i_nspname;
	if not found then
		raise exception 'Slony-I: namespace % does not exist', i_nspname;
	end if;
	select into c_table oid from "pg_catalog".pg_class where relname = i_tblname and relnamespace = c_namespace;
	if not found then
		raise warning 'Slony-I: table % does not exist in namespace %/%', i_tblname, c_namespace, i_nspname;
		return 'f'::boolean;
	end if;
	
	-- So, the table is legit; try to look it up for autovacuum policy
	if exists (select 1 from pg_class where 'autovacuum_enabled=off' = any (reloptions) and oid = c_table) then
		return 't'::boolean;   -- Autovac is turned on, but this table is disabled
	end if;

	return 'f'::boolean;

end;$$ language plpgsql;

comment on function @NAMESPACE@.ShouldSlonyVacuumTable (i_nspname name, i_tblname name) is 
'returns false if autovacuum handles vacuuming of the table, or if the table does not exist; returns true if Slony-I should manage it';

create or replace function @NAMESPACE@.TruncateOnlyTable (name) returns void as
$$
begin
	execute 'truncate only '|| @NAMESPACE@.slon_quote_input($1);
end;
$$
LANGUAGE plpgsql;

comment on function @NAMESPACE@.TruncateOnlyTable(name) is
'Calls TRUNCATE ONLY, syntax supported in version >= 8.4';


create or replace function @NAMESPACE@.addTruncateTrigger (i_fqtable text, i_tabid integer) returns integer as $$
begin
		execute 'create trigger "_@CLUSTERNAME@_truncatetrigger" ' ||
				' before truncate on ' || i_fqtable || ' for each statement execute procedure ' ||
				'@NAMESPACE@.log_truncate(' || i_tabid || ');';
		execute 'create trigger "_@CLUSTERNAME@_truncatedeny" ' ||
				' before truncate on ' || i_fqtable || ' for each statement execute procedure ' ||
				'@NAMESPACE@.truncate_deny();';
		return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.addtruncatetrigger (i_fqtable text, i_tabid integer) is 
'function to add TRUNCATE TRIGGER';

create or replace function @NAMESPACE@.replica_truncate_trigger(i_fqname text) returns integer as $$
begin
		execute 'alter table ' || i_fqname || 
				' disable trigger "_@CLUSTERNAME@_truncatetrigger";';
		execute 'alter table ' || i_fqname || 
				' enable trigger "_@CLUSTERNAME@_truncatedeny";';
		return 1;
end $$ language plpgsql;

comment on function @NAMESPACE@.replica_truncate_trigger(i_fqname text) is
'enable deny access, disable log trigger on origin.';

create or replace function @NAMESPACE@.origin_truncate_trigger(i_fqname text) returns integer as $$
begin
		execute 'alter table ' || i_fqname || 
				' enable trigger "_@CLUSTERNAME@_truncatetrigger";';
		execute 'alter table ' || i_fqname || 
				' disable trigger "_@CLUSTERNAME@_truncatedeny";';
		return 1;
end $$ language plpgsql;

comment on function @NAMESPACE@.origin_truncate_trigger(i_fqname text) is
'disable deny access, enable log trigger on origin.';

create or replace function @NAMESPACE@.add_truncate_triggers () returns integer as $$
begin

		--- Add truncate triggers
		begin		
		perform @NAMESPACE@.addtruncatetrigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname), tab_id)
				from @NAMESPACE@.sl_table
                where 2 <> (select count(*) from information_schema.triggers where 
					  event_object_schema = tab_nspname and trigger_name in ('_@CLUSTERNAME@_truncatedeny', '_@CLUSTERNAME@_truncatetrigger') and
                      event_object_table = tab_relname);

		exception when unique_violation then
				  raise warning 'add_truncate_triggers() - uniqueness violation';
				  raise warning 'likely due to truncate triggers existing partially';
				  raise exception 'add_truncate_triggers() - failure - [%][%]', SQLSTATE, SQLERRM;
		end;

		-- Activate truncate triggers for replica
		perform @NAMESPACE@.replica_truncate_trigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname))
		        from @NAMESPACE@.sl_table
                where tab_set not in (select set_id from @NAMESPACE@.sl_set where set_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'));

		-- Activate truncate triggers for origin
		perform @NAMESPACE@.origin_truncate_trigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname))
		        from @NAMESPACE@.sl_table
                where tab_set in (select set_id from @NAMESPACE@.sl_set where set_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'));

		return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.add_truncate_triggers () is 
'Add ON TRUNCATE triggers to replicated tables.';

