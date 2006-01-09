create sequence ncolseq;
alter table table4 add column newcol timestamptz;
alter table table4 alter column newcol set default now();
alter table table4 rename column id1 to col1;
alter table table4 rename column id2 to col2;

ALTER TABLE billing_discount ADD column "use_term" character(1);
ALTER TABLE billing_discount ALTER column use_term set default 'n';
ALTER TABLE billing_discount add constraint use_term_cons check (use_term in ('y','n'));
