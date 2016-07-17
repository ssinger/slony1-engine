# ----
# config.sh
# ----

if [ -z $PGPORT ] ; then
	PGPORT=5432
	export PGPORT
fi

DBNAME_BASE="pgbench_tutorial"

# ----
# SLONIK_PREAMBLE_1234
#	Slonik admin conninfo to use when all 4 nodes of the
#	cluster are available.
# ----
SLONIK_PREAMBLE_1234="cluster name = pgbench;

node 1 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_1';
node 2 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_2';
node 3 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_3';
node 4 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_4';
"

# ----
# SLONIK_PREAMBLE_123
#	Slonik admin conninfo to use when node 4
#	is not available.
# ----
SLONIK_PREAMBLE_123="cluster name = pgbench;

node 1 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_1';
node 2 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_2';
node 3 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_3';
"

# ----
# SLONIK_PREAMBLE_234
#	Slonik admin conninfo to use when node 1
#	is not available.
# ----
SLONIK_PREAMBLE_234="cluster name = pgbench;

node 2 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_2';
node 3 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_3';
node 4 admin conninfo = 'port=$PGPORT dbname=${DBNAME_BASE}_4';
"

