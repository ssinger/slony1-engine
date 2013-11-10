
create type @NAMESPACE@.vactables as (nspname name, relname name);

comment on type @NAMESPACE@.vactables is 'used as return type for SRF function TablesToVacuum';


ALTER TABLE @NAMESPACE@.sl_table SET (user_catalog_table=true);
ALTER TABLE @NAMESPACE@.sl_sequence SET(user_catalog_table=true);
ALTER TABLE @NAMESPACE@.sl_set SET (user_catalog_table=true);