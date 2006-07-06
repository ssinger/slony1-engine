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

create table table4 (
  id serial NOT NULL,
  id2 integer
);

create unique index no_good_candidate_pk on table4 (id, id2);