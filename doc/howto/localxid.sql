-- $Id: localxid.sql,v 1.1 2004-07-30 20:44:01 cbbrowne Exp $

-- Creates a "placeholder" XXID domain, used just to generate
-- documentation.

-- In reality, XXID is a type defined in C, but that isn't important
-- for the documentation.

create domain @NAMESPACE@.xxid integer not null;