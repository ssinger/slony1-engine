create domain alt_timestamptz timestamptz;

CREATE TABLE table1(
  id            SERIAL PRIMARY KEY, 
  ts            TIMESTAMP,
  tsz           alt_timestamptz, 
  ds            DATE,
  nowts         TIMESTAMP DEFAULT NOW(),
  nowtsz        TIMESTAMPTZ DEFAULT NOW(),
  nowds         DATE DEFAULT NOW(),
  tsz_array     TIMESTAMPTZ[] DEFAULT '{now(),now()}'
);               
