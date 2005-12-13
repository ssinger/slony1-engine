CREATE TABLE table1(
  id		SERIAL		PRIMARY KEY, 
  data		TEXT
);

CREATE TABLE table2(
  id		SERIAL		UNIQUE NOT NULL, 
  table1_id	INT4		REFERENCES table1(id) 
					ON UPDATE CASCADE ON DELETE CASCADE, 
  data		TEXT
);

CREATE TABLE table3(
  id		SERIAL,
  table2_id	INT4		REFERENCES table2(id)
					ON UPDATE SET NULL ON DELETE SET NULL,
  mod_date	TIMESTAMPTZ	NOT NULL DEFAULT now(),
  data		FLOAT		NOT NULL DEFAULT random()
  CONSTRAINT table3_date_check	CHECK (mod_date <= now())
); 

CREATE TABLE table4 (
  id1 serial,
  id2 serial,
  data text,
  primary key (id1, id2)
);

insert into table4 (data) values ('BA Baracus');
insert into table4 (data) values ('HM Murdoch');
insert into table4 (data) values ('Face');
insert into table4 (data) values ('Hannibal');