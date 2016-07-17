#!/bin/sh

if [ $# -lt 1 ] ; then
	echo "usage: $0 NODE_ID [...]" >&2
	exit 2
fi

. ./config.sh

for noid in $* ; do
	echo "Node $noid:"
	psql ${DBNAME_BASE}_$noid <<_EOF_
		with A as (select sum(abalance) as sum_abalance from pgbench_accounts),
			 B as (select sum(bbalance) as sum_bbalance from pgbench_branches),
			 T as (select sum(tbalance) as sum_tbalance from pgbench_tellers),
			 H as (select sum(delta) as sum_delta from pgbench_history),
			 E as (select count(*) as not_epoch from pgbench_accounts
				where last_update <> 'epoch')
		select sum_abalance, sum_bbalance, sum_tbalance, sum_delta, not_epoch
			from A, B, T, H, E;
_EOF_
	echo ""
done
