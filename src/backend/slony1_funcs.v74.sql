-- ----------------------------------------------------------------------
-- slony1_funcs.v74.sql
--
--    Version 7.4 specific part of the replication support functions.
--
--	Copyright (c) 2003-2004, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_funcs.v74.sql,v 1.5 2005-04-06 23:29:46 darcyb Exp $
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
	execute ''truncate '' || p_tab_fqname;
	return 1;
end;
' language plpgsql;

