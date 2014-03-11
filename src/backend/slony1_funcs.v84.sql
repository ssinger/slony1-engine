-- ----------------------------------------------------------------------
-- slony1_funcs.v84.sql
--
--    Version 8.4 specific part of the replication support functions.
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

create or replace function @NAMESPACE@.alterTableAddTruncateTrigger (i_fqtable text, i_tabid integer) returns integer as $$
begin
		execute 'create trigger "_@CLUSTERNAME@_truncatetrigger" ' ||
				' before truncate on ' || i_fqtable || ' for each statement execute procedure ' ||
				'@NAMESPACE@.log_truncate(' || i_tabid || ');';
		execute 'create trigger "_@CLUSTERNAME@_truncatedeny" ' ||
				' before truncate on ' || i_fqtable || ' for each statement execute procedure ' ||
				'@NAMESPACE@.deny_truncate();';
		return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.alterTableAddTruncateTrigger (i_fqtable text, i_tabid integer) is 
'function to add TRUNCATE TRIGGER';

create or replace function @NAMESPACE@.alterTableDropTruncateTrigger (i_fqtable text, i_tabid integer) returns integer as $$
begin
		execute 'drop trigger "_@CLUSTERNAME@_truncatetrigger" ' ||
				' on ' || i_fqtable || ';';
		execute 'drop trigger "_@CLUSTERNAME@_truncatedeny" ' ||
				' on ' || i_fqtable || ';';
		return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.alterTableDropTruncateTrigger (i_fqtable text, i_tabid integer) is 
'function to drop TRUNCATE TRIGGER';

create or replace function @NAMESPACE@.alterTableConfigureTruncateTrigger(i_fqname text, i_log_stat text, i_deny_stat text) returns integer as $$
begin
		execute 'alter table ' || i_fqname || ' ' || i_log_stat ||
				' trigger "_@CLUSTERNAME@_truncatetrigger";';
		execute 'alter table ' || i_fqname || ' ' || i_deny_stat ||
				' trigger "_@CLUSTERNAME@_truncatedeny";';
		return 1;
end $$ language plpgsql;

comment on function @NAMESPACE@.alterTableConfigureTruncateTrigger(i_fqname text, i_log_stat text, i_deny_stat text) is
'Configure the truncate triggers according to origin status.';

create or replace function @NAMESPACE@.upgradeSchemaAddTruncateTriggers () returns integer as $$
begin

		--- Add truncate triggers
		begin		
		perform @NAMESPACE@.alterTableAddTruncateTrigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname), tab_id)
				from @NAMESPACE@.sl_table
                where 2 <> (select count(*) from pg_catalog.pg_trigger,
					  pg_catalog.pg_class, pg_catalog.pg_namespace where 
					  pg_trigger.tgrelid=pg_class.oid
					  AND pg_class.relnamespace=pg_namespace.oid
					  AND
					  pg_namespace.nspname = tab_nspname and tgname in ('_@CLUSTERNAME@_truncatedeny', '_@CLUSTERNAME@_truncatetrigger') and
                      pg_class.relname = tab_relname
					  );

		exception when unique_violation then
				  raise warning 'upgradeSchemaAddTruncateTriggers() - uniqueness violation';
				  raise warning 'likely due to truncate triggers existing partially';
				  raise exception 'upgradeSchemaAddTruncateTriggers() - failure - [%][%]', SQLSTATE, SQLERRM;
		end;

		-- Activate truncate triggers for replica
		perform @NAMESPACE@.alterTableConfigureTruncateTrigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname)
		,'disable','enable') 
		        from @NAMESPACE@.sl_table
                where tab_set not in (select set_id from @NAMESPACE@.sl_set where set_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'))
                      and exists (select 1 from  pg_catalog.pg_trigger
                                           where pg_trigger.tgname like '_@CLUSTERNAME@_truncatetrigger' and pg_trigger.tgenabled = 'O'
                                                 and pg_trigger.tgrelid=tab_reloid ) 
                      and  exists (select 1 from  pg_catalog.pg_trigger
                                            where pg_trigger.tgname like '_@CLUSTERNAME@_truncatedeny' and pg_trigger.tgenabled = 'D' 
                                                  and pg_trigger.tgrelid=tab_reloid);

		-- Activate truncate triggers for origin
		perform @NAMESPACE@.alterTableConfigureTruncateTrigger(@NAMESPACE@.slon_quote_brute(tab_nspname) || '.' || @NAMESPACE@.slon_quote_brute(tab_relname)
		,'enable','disable') 
		        from @NAMESPACE@.sl_table
                where tab_set in (select set_id from @NAMESPACE@.sl_set where set_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'))
                      and exists (select 1 from  pg_catalog.pg_trigger
                                           where pg_trigger.tgname like '_@CLUSTERNAME@_truncatetrigger' and pg_trigger.tgenabled = 'D'
                                                 and pg_trigger.tgrelid=tab_reloid )                                                    
                      and  exists (select 1 from  pg_catalog.pg_trigger
                                            where pg_trigger.tgname like '_@CLUSTERNAME@_truncatedeny' and pg_trigger.tgenabled = 'O'
                                                  and pg_trigger.tgrelid=tab_reloid);

		return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.upgradeSchemaAddTruncateTriggers () is 
'Add ON TRUNCATE triggers to replicated tables.';

