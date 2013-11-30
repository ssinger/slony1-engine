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
  primary key(id)
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

CREATE TABLE table5 (
  id	serial,
  data	int4,
  primary key (id)
);

insert into table5 (data) values (1);
insert into table5 (data) values (2);
insert into table5 (data) values (3);

create sequence billing_discount_seq;

CREATE TABLE billing_discount (
discount_code character(2) NOT NULL,
billing_object_type character varying(10) NOT NULL,
billing_action_type character varying(10) NOT NULL,
discount_amount numeric(7,2) NOT NULL,
start_date timestamp with time zone NOT NULL,
end_date timestamp with time zone NOT NULL,
billing_discount_id integer DEFAULT nextval('billing_discount_seq'::text) NOT NULL,
registrar_id integer,
tld_id integer,
zone_id integer
);

ALTER TABLE ONLY billing_discount
    ADD CONSTRAINT billing_discount_pkey PRIMARY KEY (billing_discount_id);

CREATE OR REPLACE FUNCTION insert_table1() returns trigger
as $$
declare

begin
 insert into table1(data) values (NEW.data);
  return NEW;
end;
$$
language plpgsql;