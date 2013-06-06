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
  data		FLOAT		NOT NULL DEFAULT round(random()::numeric,8)
  CONSTRAINT table3_date_check	CHECK (mod_date <= now()),
  primary key (id)
); 

-- Create some Evil names...
create schema "Schema.name";
create schema "Studly Spacey Schema";
create table "Schema.name"."user" (
  id integer,
  "user" text not null unique,
  primary key (id)
);

create table "Schema.name"."Capital Idea" (
  "user" text,
description text,
  primary key("user")
);

create table public.evil_index_table (
  id integer not null,
  name text not null,
  "eViL StudlyCaps column" text
);
create unique index "user" on public.evil_index_table(id);

create table public."Jan's bad ideas" (
	id int4 primary key
);
