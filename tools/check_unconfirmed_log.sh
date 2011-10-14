#!/bin/sh

if [ $# -lt 2 ] ; then
	echo "usage: `basename $0` clustername [psql-options] dbname" >&2
	exit 1
fi

cluster=$1; shift
nsp="\"_${cluster}\""
psql="psql -qtA $*"
rc=0

#
# Get a list of all nodes that are origin to at least one set
#
origins=`${psql} <<_EOF_
    select distinct set_origin from ${nsp}.sl_set;
_EOF_
`

for origin in $origins ; do
	#
	# Per origin determine which is the highest event seqno
	# that is confirmed by all subscribers to any of the origins
	# sets.
	# 
	all_confirmed=`${psql} <<_EOF_
		select min(max_seqno) from (
			select con_received, max(con_seqno) as max_seqno
				from ${nsp}.sl_confirm
				where con_origin = '${origin}'
				and con_received in (
					select distinct sub_receiver from ${nsp}.sl_subscribe SUB,
							${nsp}.sl_set SET
						where SET.set_id = SUB.sub_set
						and SET.set_origin = '${origin}'
				)
				group by con_received
			) as maxconfirmed;
_EOF_
`

	#
	# Get the txid snapshot that corresponds with that event
	#
	all_snapshot=`${psql} <<_EOF_
		select ev_snapshot from ${nsp}.sl_event
			where ev_origin = '${origin}'
			and ev_seqno = '${all_confirmed}';
_EOF_
`

	#
	# Count the number of unconfirmed sl_log rows that appeared after
	# that event was created.
	#
	unconfirmed=`${psql} <<_EOF_
		select count(*) from (
		select log_origin from ${nsp}.sl_log_1
			where log_origin = '${origin}'
			and log_txid >= "pg_catalog".txid_snapshot_xmax('${all_snapshot}')
		union all
		select log_origin from ${nsp}.sl_log_1
			where log_origin = '${origin}'
			and log_txid in (
				select * from "pg_catalog".txid_snapshot_xip('${all_snapshot}')
			)
		union all
		select log_origin from ${nsp}.sl_log_2
			where log_origin = '${origin}'
			and log_txid >= "pg_catalog".txid_snapshot_xmax('${all_snapshot}')
		union all
		select log_origin from ${nsp}.sl_log_2
			where log_origin = '${origin}'
			and log_txid in (
				select * from "pg_catalog".txid_snapshot_xip('${all_snapshot}')
			)
		) AS cnt;
_EOF_
`
	#
	# Report the result
	#
	if [ $unconfirmed -gt 0 ] ; then
		echo "origin $origin: ERROR - node has $unconfirmed unconfirmed log rows"
		echo "origin $origin:         the last event confirmed by all subscribers"
		echo "origin $origin:         is $all_confirmed"
		rc=2
	else
		echo "origin $origin: OK"
	fi
done

exit ${rc}
