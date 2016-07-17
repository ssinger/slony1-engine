#!/bin/sh
echo "######################################################################"
echo "# 11_fail_node_1.sh"
echo "#"
echo "#	This script performs what needs to happen when node 1 (master)"
echo "#	fails CATASTROPHICALLY!!!"
echo "######################################################################"

# ----
# Source in the configuration file.
# ----
. ./config.sh

# ----
# We simulate the crash of our current master database by
# killing all existing client connections and then dropping the database.
# ----
while true ; do
	psql -c "select pid, pg_terminate_backend(pid) from pg_stat_activity
		where datname = '${DBNAME_BASE}_1'" ${DBNAME_BASE}_2
	dropdb ${DBNAME_BASE}_1 && break
	echo "let's try that again ..."
done

echo "node 1 got destroyed"
sleep 10

echo "This is what our application thought about it:"
echo "----"
cat ./log/pgbench.log
echo "----"

# ----
# Perform the failover. We need to use the -w option here because
# we really do not have a properly working cluster. So understanding
# the exact event propagation mechanism is key here.
# ----
slonik -w <<_EOF_
# ----
# Use the slony preamble for all nodes.
# ----
$SLONIK_PREAMBLE_234

# ----
# Create the paths to save node 4 below.
# ----
echo 'SLONIK: creating paths between nodes 2 and 4';
store path (server = 2, client = 4,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_2');
store path (server = 4, client = 2,
	conninfo = 'port=${PGPORT} dbname=${DBNAME_BASE}_4');
sync (id = 2);
sync (id = 4);
wait for event (origin = 2, confirmed = 4, wait on = 4, timeout = 0);
wait for event (origin = 4, confirmed = 2, wait on = 2, timeout = 0);

# ----
# Node 4 can now communicate with node 2. Time to do the actual failover.
# ----
echo 'SLONIK: failing over node 1 to node 2';
failover (id = 1, backup node = 2);

_EOF_

# ----
# As soon as that is done, we can launch our application against the
# backup node 2.
# ----
echo "starting pgbench against backup node 2"
./start_pgbench.sh 2

# ----
# All that is left is to resubscribe node 4 to the backup master node 2.
# ----
slonik -w <<_EOF_
# ----
$SLONIK_PREAMBLE_234

echo 'SLONIK: resubscribing node 4 with provider 2';
subscribe set (id = 1000, receiver = 4, provider = 2, forward = yes);
sync (id = 2);
wait for event (origin = 2, confirmed = all, wait on = 2, timeout = 0);
echo 'SLONIK: drop node 1';
drop node (id = 1, event node = 2);
sync (id = 2);
wait for event (origin = 2, confirmed = all, wait on = 2, timeout = 0);

_EOF_

# ----
# There is a bug in DROP NODE that may leave behind old data belonging
# to a dropped node. This will be fixed in the next release.
# ----
for noid in 2 3 4 ; do
psql ${DBNAME_BASE}_$noid <<_EOF_

set search_path to _pgbench;

\! echo -n "sl_apply_stats: "
delete from sl_apply_stats where as_origin = 1;
\! echo -n "sl_components:  "
delete from sl_components where co_node = 1;
\! echo -n "sl_confirm:     "
delete from sl_confirm where con_origin = 1 or con_received = 1;
\! echo -n "sl_event:       "
delete from sl_event where ev_origin = 1;
\! echo -n "sl_listen:      "
delete from sl_listen where li_origin = 1 or li_provider = 1 or li_receiver = 1;
\! echo -n "sl_log_1:       "
delete from sl_log_1 where log_origin = 1;
\! echo -n "sl_log_2:       "
delete from sl_log_2 where log_origin = 1;
\! echo -n "sl_log_script:  "
delete from sl_log_script where log_origin = 1;
\! echo -n "sl_node:        "
delete from sl_node where no_id = 1;
\! echo -n "sl_path:        "
delete from sl_path where pa_server = 1 or pa_client = 1;
\! echo -n "sl_seqlog:      "
delete from sl_seqlog where seql_origin = 1;
\! echo -n "sl_set:         "
delete from sl_set where set_origin = 1;
\! echo -n "sl_setsync:     "
delete from sl_setsync where ssy_origin = 1;
\! echo -n "sl_subscribe:   "
delete from sl_subscribe where sub_provider = 1 or sub_receiver = 1;

_EOF_
done
