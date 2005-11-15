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

-- Table to perform UTF8 tests (checks multibyte; should be helpful in
-- preventing problems with Asian character sets too)
create table utf8table (
    id integer not null unique default nextval('utf8_id'),
    string text,
    primary key(id)
);

create sequence utf8_id;
INSERT INTO utf8table (string) VALUES ('1b\303\241r') ;
create sequence foo_id;

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
create sequence public."Evil Spacey Sequence Name";
create sequence "Studly Spacey Schema"."user";
create sequence "Schema.name"."a.periodic.sequence";
