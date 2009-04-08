set search_path to foo;
INSERT INTO foo.table1(data) VALUES ('placeholder 1');
INSERT INTO foo.table2(table1_id,data) VALUES (1,'placeholder 1');
INSERT INTO foo.table3(table2_id) VALUES (1);
