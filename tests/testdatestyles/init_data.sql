insert into table1(ts, tsz, ds) values ('2006-01-01', '2006-06-01', '2007-01-01');
insert into table1(ts, tsz, ds) values ('infinity', 'infinity', now());
insert into table1(ts, tsz, ds) values ('-infinity', '-infinity', now());

-- Some nice dates that take place inside the time window that was
-- a tad ambiguous in 2007 in that the definition of daylight savings time
-- was changed - per Bill Moran

insert into table1 (ts, tsz, ds) values (now(), '2007-03-12 11:05:32.913154 edt', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-12 10:05:32.913154 est', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-14 17:39:28.595669 edt', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-14 16:39:28.595669 est', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-28 14:45:55.75936 edt', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-28 13:45:55.75936 est', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-29 13:35:19.960505 edt', now());
insert into table1 (ts, tsz, ds) values (now(), '2007-03-29 12:35:19.960505 est', now());

