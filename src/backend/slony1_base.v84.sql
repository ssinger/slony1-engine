-- ----------------------------------------------------------------------
-- slony1_base.v84.sql
--
--    Version 8.3 specific parts of the basic replication schema.
--
--	Copyright (c) 2007-2009, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- 
-- ----------------------------------------------------------------------

create type @NAMESPACE@.vactables as (nspname name, relname name);

comment on type @NAMESPACE@.vactables is 'used as return type for SRF function TablesToVacuum';


