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
