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

create table table3 (
  id serial NOT NULL,
  id2 integer
);

create unique index no_good_candidate_pk on table3 (id, id2);

create table table4 (
  id serial primary key,
  numcol numeric(12,4), -- 1.23
  realcol real,     -- (1.23)
  ptcol point,      -- (1,2)
  pathcol path,     -- ((1,1),(2,2),(3,3),(4,4))
  polycol polygon,  -- ((1,1),(2,2),(3,3),(4,4))
  circcol circle,   -- <(1,2>,3>
  ipcol inet,       -- "192.168.1.1"
  maccol macaddr,   -- "04:05:06:07:08:09"
  bitcol bit varying(20)  -- X'123' 
);

create table table5 (
  id serial,
  d1 text,
  d2 text,
  id2 serial,
  d3 text,
  d4 text,
  d5 text,
  d6 text,
  id3 serial,
  d7 text,
  d8 text,
  d9 text,
  d10 text,
  d11 text,
  primary key(id, id2, id3)
);

create table x1 (
	   id serial primary key,
	   data text
);

create table x2 (
	   id serial primary key,
	   data text
);

create table x3 (
	   id serial primary key,
	   data text
);
