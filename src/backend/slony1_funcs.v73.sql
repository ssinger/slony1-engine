-- ----------------------------------------------------------------------
-- slony1_funcs.v73.sql
--
--    Version 7.3 specific part of the replication support functions.
--
--	Copyright (c) 2003-2004, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_funcs.v73.sql,v 1.7.2.1 2005-09-28 01:18:55 cbbrowne Exp $
-- ----------------------------------------------------------------------

-- ----------------------------------------------------------------------
-- FUNCTION truncateTable(tab_fqname)
--
--	Remove all content from a table before the subscription
--	content is loaded via COPY.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.truncateTable(text)
returns int4
as '
declare
	p_tab_fqname		alias for $1;
begin
	execute ''delete from only '' || @NAMESPACE@.slon_quote_input(p_tab_fqname);
	return 1;
end;
' language plpgsql;

comment comment on function @NAMESPACE@.truncateTable(text) is
'Delete all data from specified table';

create or replace function @NAMESPACE@.pre74()
returns integer
as 'select 1' language sql;

comment on function @NAMESPACE@.pre74() is 
'Returns 1/0 based on whether or not the DB is running a
version earlier than 7.4';
