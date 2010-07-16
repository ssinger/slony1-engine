-------------------------------------------------------------------------------------
-- 
-------------------------------------------------------------------------------------

--- Here is a schema suitable for loading in the SQL scripts generated
--- by the various tests.

create table slony_tests (
  id serial,

  -- Preface with a whole bunch of versioning information
  smoduleversion text NOT NULL,    -- populate via @NAMESPACE@.getModuleversion();
  sv_major integer NOT NULL,       -- populate via @NAMESPACE@.slonyVersionMajor();
  sv_minor integer NOT NULL,       -- populate via @NAMESPACE@.slonyVersionMinor();
  sv_patch integer NOT NULL,       -- populate via @NAMESPACE@.slonyVersionPatchlevel();

  pg_version text NOT NULL,        -- populate via version();

  uname_m text NOT NULL,             -- output of `uname -m` - uname -m = ia64
  uname_r text NOT NULL,             -- output of `uname -r` - uname -r = 2.6.18.6
  uname_s text NOT NULL,             -- output of `uname -s` - uname -s = Linux
  uname_v text NOT NULL,             -- output of `uname -v` - uname -v = #1 SMP Fri Feb 9 20:10:44 MSK 2007

  hostname text NOT NULL,          -- output of `hostname -f`

  tester_identity text NOT NULL,   -- email address of the tester, from getenv("SLONYTESTER")

  -- Now, test-specific data
  testname text NOT NULL,          -- which test was run?
  start_time timestamptz NOT NULL,
  end_time timestamptz NOT NULL,

  successful boolean NOT NULL,     -- was the test totally successful?
  failure_desc text,               -- A description of the failure noticed
  
  primary key(id)
);

create index st_slversion on slony_tests (sv_major, sv_minor, sv_patch);
create index st_host on slony_tests(tester_identity);
create index st_start on slony_tests(start_time);
