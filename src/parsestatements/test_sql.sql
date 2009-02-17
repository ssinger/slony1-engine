select * from foo;  select * from bar;  select * from frobozz;
alter table foo add column c integer;
alter table foo alter column c set not null; 

-- Comment line that should hide a whole bunch of quoting... ;; $$
-- '"''"; "\"\"\"$$ ;"\"""

-- Here is an old-style pl/pgsql function using heavy quoting
create function foo (text) returns integer as '
  declare
     rc record;
  begin
    select * into rc from foo where name = ''Some Favored name'';
    return NULL;
  end;' language plpgsql;

select * from foo;  select * from bar;  select * from frobozz;

create or replace function foo (text) returns integer as $$
  begin
    select * into rc from foo where name = 'Some Favored name';
    return NULL;
  end;
$$ language plpgsql;

select * from foo;  select * from bar;  select * from frobozz;

-- This isn't actually a particularly well-framed stored function
-- but it abuses $$dollar quoting$$ quite nicely...
create or replace function foo (text) returns integer as $$
  begin
    select * into rc from foo where name = $23$Some Favored name$23$;
    -- Use a secondary bit of quoting to make sure that nesting works...
    select $24$ -- another " " thing ' ' \\\'\$ $24$;
    return NULL;
  end;
$$ language plpgsql;


-- Here is a rule creation with an embedded semicolon
-- "Dmitry Koterov" <dmitry@koterov.ru>

create table "public"."position";

CREATE RULE "position_get_last_id_on_insert2"
AS ON INSERT TO "public"."position" DO (SELECT
currval('position_position_id_seq'::regclass) AS id;);

-- Added to verify handling of queries tried by
-- "Dmitry Koterov" <dmitry@koterov.ru>

CREATE INDEX aaa ON public.bbb USING btree ((-ccc), ddd);

--  Apparently a pair of backslashes fold down into one?
-- "Dmitry Koterov" <dmitry@koterov.ru>

CREATE UNIQUE INDEX "i_dictionary_uni_abbr" ON "static"."dictionary"
USING btree ((substring(dic_russian, E'^([^(]*[^( ]) *\\('::text)))
WHERE (dic_category_id = 26);

-- Some more torturing per Weslee Bilodeau

-- I figure the $_$, $$, etc edge-cases would be another fun one to roll
-- into a custom parser.

CREATE FUNCTION test( ) RETURNS text AS $_$ SELECT ';', E'\';\'',
'"";""', E'"\';' ; SELECT 'OK'::text ; $_$ LANGUAGE SQL ;

SELECT $_$ hello; this ; - is '\" a '''' test $_$ ;

SELECT $$ $ test ; $ ;  $$ ;

-- All really funky, but perfectly valid.

-- Force a query to be at the end...

create table foo;
