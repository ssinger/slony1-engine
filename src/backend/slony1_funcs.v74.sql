-- ----------------------------------------------------------------------
-- slony1_funcs.v73.sql
--
--    Version 7.3 specific part of the replication support functions.
--
--	Copyright (c) 2003-2004, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_funcs.v74.sql,v 1.1 2004-03-12 23:17:32 wieck Exp $
-- ----------------------------------------------------------------------


-- ----------------------------------------------------------------------
-- FUNCTION determineAttKindSerial (tab_fqname, indexname)
--
--	Given a tablename, return the Slony-I specific attkind (used for
--	the log trigger) of the table. Use the specified unique index or
--	the primary key (if indexname is NULL).
-- ----------------------------------------------------------------------
create function @NAMESPACE@.determineAttkindUnique(text, name) returns text
as '
declare
	p_tab_fqname	alias for $1;
	p_idx_name		alias for $2;
	v_idxrow		record;
	v_attrow		record;
	v_i				integer;
	v_attno			int2;
	v_attkind		text default '''';
	v_attfound		bool;
begin
	--
	-- Lookup the tables primary key or the specified unique index
	--
	if p_idx_name isnull then
		select PGXC.relname, PGX.indexrelid, PGX.indnatts, PGX.indkey
				into v_idxrow
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN,
					"pg_catalog".pg_index PGX,
					"pg_catalog".pg_class PGXC
				where "pg_catalog".quote_ident(PGN.nspname) || ''.'' ||
					"pg_catalog".quote_ident(PGC.relname) = p_tab_fqname
					and PGN.oid = PGC.relnamespace
					and PGX.indrelid = PGC.oid
					and PGX.indexrelid = PGXC.oid
					and PGX.indisprimary;
		if not found then
			raise exception ''Slony-I: table % has no primary key'',
					p_tab_fqname;
		end if;
	else
		select PGXC.relname, PGX.indexrelid, PGX.indnatts, PGX.indkey
				into v_idxrow
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN,
					"pg_catalog".pg_index PGX,
					"pg_catalog".pg_class PGXC
				where "pg_catalog".quote_ident(PGN.nspname) || ''.'' ||
					"pg_catalog".quote_ident(PGC.relname) = p_tab_fqname
					and PGN.oid = PGC.relnamespace
					and PGX.indrelid = PGC.oid
					and PGX.indexrelid = PGXC.oid
					and PGX.indisunique
					and PGXC.relname = p_idx_name;
		if not found then
			raise exception ''Slony-I: table % has no unique index %'',
					p_tab_fqname, p_idx_name;
		end if;
	end if;

	--
	-- Loop over the tables attributes and check if they are
	-- index attributes. If so, add a "k" to the return value,
	-- otherwise add a "v".
	--
	for v_attrow in select PGA.attnum, PGA.attname
			from "pg_catalog".pg_class PGC,
			    "pg_catalog".pg_namespace PGN,
				"pg_catalog".pg_attribute PGA
			where "pg_catalog".quote_ident(PGN.nspname) || ''.'' ||
			    "pg_catalog".quote_ident(PGC.relname) = p_tab_fqname
				and PGN.oid = PGC.relnamespace
				and PGA.attrelid = PGC.oid
				and not PGA.attisdropped
				and PGA.attnum > 0
			order by attnum
	loop
		v_attfound = ''f'';

		for v_i in 0 .. v_idxrow.indnatts - 1 loop
			select indkey[v_i] into v_attno from "pg_catalog".pg_index
					where indexrelid = v_idxrow.indexrelid;
			if v_attrow.attnum = v_attno then
				v_attfound = ''t'';
				exit;
			end if;
		end loop;

		if v_attfound then
			v_attkind := v_attkind || ''k'';
		else
			v_attkind := v_attkind || ''v'';
		end if;
	end loop;

	--
	-- Return the resulting attkind
	--
	return v_attkind;
end;
' language plpgsql called on null input;


