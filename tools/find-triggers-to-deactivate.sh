#!/bin/sh
# 

# This script may be used to ask which triggers exist on a cluster
# that likely need to get deactivated on a log shipping node.  Run
# this against a node from which you intend to acquire the DBMS schema
# for the log shipping node.

# ----
# Check for correct usage
# ----
if test $# -ne 2 ; then
	echo "usage: $0 dbname clustername" >&2
	exit 1
fi

# What Do I Do???
echo "## This script lists triggers and rules on tables that may need to be removed."
echo ""

# ----
# Remember call arguments and get the nodeId of the DB specified
# ----
dbname=$1
cluster=$2

FINDTRIGGERS="select st.tab_set, n.nspname, c.relname, t.tgname from _${cluster}.sl_table st, pg_catalog.pg_namespace n, pg_catalog.pg_class c, pg_catalog.pg_trigger t where st.tab_reloid = c.oid and t.tgrelid = c.oid and n.oid = c.relnamespace order by tab_set,  nspname, relname, tgname;"

printf "## %32s %32s %32s %32s\n" "Set" "Schema" "Table" "Trigger"
TRIGGERS=`psql -Atq -d ${dbname} -c "${FINDTRIGGERS}"`
for line in `echo ${TRIGGERS}`; do
  IFS="|"
  set x $line; shift
  set=$1
  nsp=$2
  tbl=$3
  trg=$4
  printf "## %32s %32s %32s %32s\n" $set $nsp $tbl $trg
done

echo ""
FINDRULES="select st.tab_set, n.nspname, c.relname, r.rulename from _${cluster}.sl_table st, pg_catalog.pg_namespace n, pg_catalog.pg_class c, pg_catalog.pg_rules r where st.tab_reloid = c.oid and r.schemaname = c.relname and r.tablename = c.relname and n.oid = c.relnamespace order by tab_set,  nspname, relname, rulename;"

printf "## %32s %32s %32s %32s\n" "Set" "Schema" "Table" "Rule"
RULES=`psql -Atq -d ${dbname} -c "${FINDRULES}"`
for line in `echo ${RULES}`; do
  IFS="|"
  set x $line; shift
  set=$1
  nsp=$2
  tbl=$3
  trg=$4
  printf "## %32s %32s %32s %32s\n" $set $nsp $tbl $trg
done
