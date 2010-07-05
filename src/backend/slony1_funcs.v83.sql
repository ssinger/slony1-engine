-- ----------------------------------------------------------------------
-- slony1_funcs.v83.sql
--
--    Version 8.3 specific part of the replication support functions.
--
--	Copyright (c) 2007-2009, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_funcs.v83.sql,v 1.5 2010-07-05 15:06:04 ssinger Exp $
-- ----------------------------------------------------------------------

-- ----------------------------------------------------------------------
-- FUNCTION ShouldSlonyVacuumTable (nspname, tabname)
--
--	Returns 't' if the table needs to be vacuumed by Slony-I
--      Returns 'f' if autovac handles the table, so Slony-I should not
--                  or if the table is not needful altogether
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.ShouldSlonyVacuumTable (name, name) returns boolean as
$$
declare
	i_nspname alias for $1;
	i_tblname alias for $2;
	c_table oid;
	c_namespace oid;
	c_enabled boolean;
	v_dummy int4;
begin
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

comment on function @NAMESPACE@.ShouldSlonyVacuumTable (name, name) is 
'returns false if autovacuum handles vacuuming of the table, or if the table does not exist; returns true if Slony-I should manage it';




create or replace function @NAMESPACE@.TruncateOnlyTable ( name) returns void as
$$
begin
	execute 'truncate '|| @NAMESPACE@.slon_quote_input($1);
end;
$$
LANGUAGE plpgsql;


comment on function @NAMESPACE@.TruncateOnlyTable(name) is
'Calls TRUNCATE with out specifying ONLY, syntax supported in version 8.3';
