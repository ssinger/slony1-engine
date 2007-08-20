INSERT INTO table1(data) VALUES ('placeholder 1');
INSERT INTO table1(data) VALUES ('placeholder 2');
INSERT INTO table2(table1_id,data) VALUES (1,'placeholder 1');
INSERT INTO table2(table1_id,data) VALUES (2,'placeholder 2');

INSERT INTO table4(numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values ('74.0','7.40','(7,4)','((7,7),(4,4),(0,0),(7,0))','((7,4),(0,7),(4,0),(0,4))','<(7,4),0>','192.168.7.40','08:00:2d:07:04:00',X'740');

INSERT INTO table4(numcol,realcol,ptcol,pathcol,polycol,circcol,ipcol,maccol,bitcol) values ('93.1','9.31','(9,3)','((9,9),(3,3),(1,1),(9,1))','((9,3),(1,9),(3,1),(1,3))','<(9,3),1>','192.168.9.31','08:00:2d:09:03:01',X'931');

insert into table1 (data) values ('Evil C Format String: %d%05d%s%f%G%p%n%ld');