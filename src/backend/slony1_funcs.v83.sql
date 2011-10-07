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
	if exists (select 1 from pg_catalog.pg_autovacuum where vacrelid = c_table and enabled = 'f') then
		return 't'::boolean;   -- Autovac is turned on, but this table is disabled
	end if;

	return 'f'::boolean;

end;$$ language plpgsql;

comment on function @NAMESPACE@.ShouldSlonyVacuumTable (i_nspname name, i_tblname name) is 
'returns false if autovacuum handles vacuuming of the table, or if the table does not exist; returns true if Slony-I should manage it';

create or replace function @NAMESPACE@.TruncateOnlyTable (name) returns void as
$$
begin
	execute 'truncate '|| @NAMESPACE@.slon_quote_input($1);
end;
$$
LANGUAGE plpgsql;

comment on function @NAMESPACE@.TruncateOnlyTable(name) is
'Calls TRUNCATE with out specifying ONLY, syntax supported in version 8.3';

create or replace function @NAMESPACE@.alterTableAddTruncateTrigger (i_fqtable text, i_tabid integer) returns integer as $$
begin
		return 0;
end
$$ language plpgsql;

comment on function @NAMESPACE@.alterTableAddTruncateTrigger (i_fqtable text, i_tabid integer) is 
'function to add TRUNCATE TRIGGER';

create or replace function @NAMESPACE@.alterTableDropTruncateTrigger (i_fqtable text, i_tabid integer) returns integer as $$
begin
		return 0;
end
$$ language plpgsql;

comment on function @NAMESPACE@.alterTableDropTruncateTrigger (i_fqtable text, i_tabid integer) is 
'function to drop TRUNCATE TRIGGER';

create or replace function @NAMESPACE@.alterTableConfigureTruncateTrigger(i_fqname text, i_log_stat text, i_deny_stat text) returns integer as $$
begin
		return 0;
end $$ language plpgsql;

comment on function @NAMESPACE@.alterTableConfigureTruncateTrigger(i_fqname text, i_log_stat text, i_deny_stat text) is
'disable deny access, enable log trigger on origin.  

NOOP on PostgreSQL 8.3 because it does not support triggers ON TRUNCATE';

create or replace function @NAMESPACE@.upgradeSchemaAddTruncateTriggers () returns integer as $$
begin
		raise warning 'This node is running PostgreSQL 8.3 - cannot apply TRUNCATE triggers';
		return 0;
end
$$ language plpgsql;

comment on function @NAMESPACE@.upgradeSchemaAddTruncateTriggers () is 
'Add ON TRUNCATE triggers to replicated tables.  Not supported on PG 8.3, so a NOOP in this case.';

