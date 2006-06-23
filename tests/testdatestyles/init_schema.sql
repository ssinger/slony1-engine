CREATE TABLE table1(
  id            SERIAL PRIMARY KEY, 
  ts            TIMESTAMP,
  tsz           TIMESTAMPTZ,
  ds            DATE,
  nowts         TIMESTAMP DEFAULT NOW(),
  nowtsz        TIMESTAMPTZ DEFAULT NOW(),
  nowds         DATE DEFAULT NOW()
);
