-- ----------------------------------------------------------------------
-- slony1_base.v83.sql
--
--    Version 8.3 specific parts of the basic replication schema.
--
--	Copyright (c) 2007, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_base.v84.sql,v 1.1.2.1 2009-07-21 21:15:51 cbbrowne Exp $
-- ----------------------------------------------------------------------

create type @NAMESPACE@.vactables as (nspname name, relname name);

comment on type @NAMESPACE@.vactables is 'used as return type for SRF function TablesToVacuum';


