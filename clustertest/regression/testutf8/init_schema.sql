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
  data		FLOAT NOT NULL DEFAULT round(random()::numeric,8)
  CONSTRAINT table3_date_check	CHECK (mod_date <= now()),
  primary key (id)
); 

-- Table to perform UTF8 tests (checks multibyte; should be helpful in
-- preventing problems with Asian character sets too)
CREATE TABLE utf8table (
    id serial,
    string text,
    primary key(id)
);
INSERT INTO utf8table (string) VALUES (E'1b\303\241r') ;

