-- ----------------------------------------------------------------------
-- slony1_funcs.v73.sql
--
--    Version 7.3 specific part of the replication support functions.
--
--	Copyright (c) 2003-2009, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_funcs.v73.sql,v 1.9.2.2 2009-08-17 17:39:57 devrim Exp $
-- ----------------------------------------------------------------------

-- ----------------------------------------------------------------------
-- FUNCTION prepareTableForCopy(tab_id)
--
--	Remove all content from a table before the subscription
--	content is loaded via COPY and disable index maintenance.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.prepareTableForCopy(int4)
returns int4
as '
declare
	p_tab_id		alias for $1;
	v_tab_oid		oid;
	v_tab_fqname	text;
begin
	-- ----
	-- Get the tables OID and fully qualified name
	-- ---
	select	PGC.oid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || ''.'' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
		into v_tab_oid, v_tab_fqname
			from @NAMESPACE@.sl_table T,   
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
				where T.tab_id = p_tab_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid;
	if not found then
		raise exception ''Table with ID % not found in sl_table'', p_tab_id;
	end if;

	-- ----
	-- Setting pg_class.relhasindex to false will cause copy not to
	-- maintain any indexes. At the end of the copy we will reenable
	-- them and reindex the table. This bulk creating of indexes is
	-- faster.
	-- ----
	update pg_class set relhasindex = ''f'' where oid = v_tab_oid;
	execute ''delete from only '' || @NAMESPACE@.slon_quote_input(v_tab_fqname);
	return 1;
end;
' language plpgsql;

comment on function @NAMESPACE@.prepareTableForCopy(int4) is
'Delete all data and suppress index maintenance';

-- ----------------------------------------------------------------------
-- FUNCTION finishTableAfterCopy(tab_id)
--
--	Reenable index maintenance and reindex the table after COPY.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.finishTableAfterCopy(int4)
returns int4
as '
declare
	p_tab_id		alias for $1;
	v_tab_oid		oid;
	v_tab_fqname	text;
begin
	-- ----
	-- Get the tables OID and fully qualified name
	-- ---
	select	PGC.oid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || ''.'' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
		into v_tab_oid, v_tab_fqname
			from @NAMESPACE@.sl_table T,   
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
				where T.tab_id = p_tab_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid;
	if not found then
		raise exception ''Table with ID % not found in sl_table'', p_tab_id;
	end if;

	-- ----
	-- Reenable indexes and reindex the table.
	-- ----
	update pg_class set relhasindex = ''t'' where oid = v_tab_oid;
	execute ''reindex table '' || @NAMESPACE@.slon_quote_input(v_tab_fqname);
	return 1;
end;
' language plpgsql;

comment on function @NAMESPACE@.finishTableAfterCopy(int4) is
'Reenable index maintenance and reindex the table';

create or replace function @NAMESPACE@.pre74()
returns integer
as 'select 1;' language sql;

comment on function @NAMESPACE@.pre74() is 
'Returns 1 or 0 based on whether or not the DB is running a
version earlier than 7.4';

create or replace function @NAMESPACE@.make_function_strict (text, text) returns void as
'
   update "pg_catalog"."pg_proc" set proisstrict = ''t'' where 
           proname = $1 and pronamespace = (select oid from "pg_catalog"."pg_namespace" where nspname = '_@CLUSTERNAME@') and prolang = (select oid from "pg_catalog"."pg_language" where lanname = ''c'');
' language sql;

comment on function @NAMESPACE@.make_function_strict (text, text) is
'Equivalent to 8.1+ ALTER FUNCTION ... STRICT';
