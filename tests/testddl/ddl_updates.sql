create sequence ncolseq;
alter table table4 add column newcol timestamptz;
alter table table4 alter column newcol set default now();
alter table table4 rename column id1 to col1;
alter table table4 rename column id2 to col2;
