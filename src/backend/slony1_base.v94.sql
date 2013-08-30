
create type @NAMESPACE@.vactables as (nspname name, relname name);

comment on type @NAMESPACE@.vactables is 'used as return type for SRF function TablesToVacuum';


ALTER TABLE @NAMESPACE@.sl_table SET (treat_as_catalog_table=true);
ALTER TABLE @NAMESPACE@.sl_sequence SET(treat_as_catalog_table=true);
ALTER TABLE @NAMESPACE@.sl_set SET (treat_as_catalog_table=true);