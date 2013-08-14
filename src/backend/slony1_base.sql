-- ----------------------------------------------------------------------
-- slony1_base.sql
--
--    Declaration of the basic replication schema.
--
--	Copyright (c) 2003-2009, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- 
-- ----------------------------------------------------------------------


-- **********************************************************************
-- * Tables
-- **********************************************************************


-- ----------------------------------------------------------------------
-- TABLE sl_node
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_node (
	no_id				int4,
	no_active			bool,
	no_comment			text,
	no_failed			bool,
	CONSTRAINT "sl_node-pkey"
		PRIMARY KEY (no_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_node is 'Holds the list of nodes associated with this namespace.';
comment on column @NAMESPACE@.sl_node.no_id is 'The unique ID number for the node';  
comment on column @NAMESPACE@.sl_node.no_active is 'Is the node active in replication yet?';  
comment on column @NAMESPACE@.sl_node.no_comment is 'A human-oriented description of the node';


-- ----------------------------------------------------------------------
-- TABLE sl_nodelock
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_nodelock (
	nl_nodeid			int4,
	nl_conncnt			serial,
	nl_backendpid		int4,

	CONSTRAINT "sl_nodelock-pkey"
		PRIMARY KEY (nl_nodeid, nl_conncnt)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_nodelock is 'Used to prevent multiple slon instances and to identify the backends to kill in terminateNodeConnections().';
comment on column @NAMESPACE@.sl_nodelock.nl_nodeid is 'Clients node_id';
comment on column @NAMESPACE@.sl_nodelock.nl_conncnt is 'Clients connection number';
comment on column @NAMESPACE@.sl_nodelock.nl_backendpid is 'PID of database backend owning this lock';


-- ----------------------------------------------------------------------
-- TABLE sl_set
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_set (
	set_id				int4,
	set_origin			int4,
	set_locked			bigint,
	set_comment			text,

	CONSTRAINT "sl_set-pkey"
		PRIMARY KEY (set_id),
	CONSTRAINT "set_origin-no_id-ref"
		FOREIGN KEY (set_origin)
		REFERENCES @NAMESPACE@.sl_node (no_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_set is 'Holds definitions of replication sets.';
comment on column @NAMESPACE@.sl_set.set_id is 'A unique ID number for the set.';
comment on column @NAMESPACE@.sl_set.set_origin is 
	'The ID number of the source node for the replication set.';
comment on column @NAMESPACE@.sl_set.set_locked is 'Transaction ID where the set was locked.';
comment on column @NAMESPACE@.sl_set.set_comment is 'A human-oriented description of the set.';


-- ----------------------------------------------------------------------
-- TABLE sl_setsync
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_setsync (
	ssy_setid			int4,
	ssy_origin			int4,
	ssy_seqno			int8,
	ssy_snapshot		"pg_catalog".txid_snapshot,
	ssy_action_list		text,

	CONSTRAINT "sl_setsync-pkey"
		PRIMARY KEY (ssy_setid),
	CONSTRAINT "ssy_setid-set_id-ref"
		FOREIGN KEY (ssy_setid)
		REFERENCES @NAMESPACE@.sl_set (set_id),
	CONSTRAINT "ssy_origin-no_id-ref"
		FOREIGN KEY (ssy_origin)
		REFERENCES @NAMESPACE@.sl_node (no_id)
) WITHOUT OIDS;
comment on table  @NAMESPACE@.sl_setsync is 'SYNC information';
comment on column @NAMESPACE@.sl_setsync.ssy_setid is 'ID number of the replication set';
comment on column @NAMESPACE@.sl_setsync.ssy_origin is 'ID number of the node';
comment on column @NAMESPACE@.sl_setsync.ssy_seqno is 'Slony-I sequence number';
comment on column @NAMESPACE@.sl_setsync.ssy_snapshot is 'TXID in provider system seen by the event';
comment on column @NAMESPACE@.sl_setsync.ssy_action_list is 'action list used during the subscription process. At the time a subscriber copies over data from the origin, it sees all tables in a state somewhere between two SYNC events. Therefore this list must contains all log_actionseqs that are visible at that time, whose operations have therefore already been included in the data copied at the time the initial data copy is done.  Those actions may therefore be filtered out of the first SYNC done after subscribing.';


-- ----------------------------------------------------------------------
-- TABLE sl_table
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_table (
	tab_id				int4,
	tab_reloid			oid UNIQUE NOT NULL,
	tab_relname			name NOT NULL,
	tab_nspname			name NOT NULL,
	tab_set				int4,
	tab_idxname			name NOT NULL,
	tab_altered			boolean NOT NULL,
	tab_comment			text,

	CONSTRAINT "sl_table-pkey"
		PRIMARY KEY (tab_id),
	CONSTRAINT "tab_set-set_id-ref"
		FOREIGN KEY (tab_set)
		REFERENCES @NAMESPACE@.sl_set (set_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_table is 'Holds information about the tables being replicated.';
comment on column @NAMESPACE@.sl_table.tab_id is 'Unique key for Slony-I to use to identify the table';
comment on column @NAMESPACE@.sl_table.tab_reloid is 'The OID of the table in pg_catalog.pg_class.oid';
comment on column @NAMESPACE@.sl_table.tab_relname is 'The name of the table in pg_catalog.pg_class.relname used to recover from a dump/restore cycle'; 
comment on column @NAMESPACE@.sl_table.tab_nspname is 'The name of the schema in pg_catalog.pg_namespace.nspname used to recover from a dump/restore cycle'; 
comment on column @NAMESPACE@.sl_table.tab_set is 'ID of the replication set the table is in';
comment on column @NAMESPACE@.sl_table.tab_idxname is 'The name of the primary index of the table';
comment on column @NAMESPACE@.sl_table.tab_altered is 'Has the table been modified for replication?';
comment on column @NAMESPACE@.sl_table.tab_comment is 'Human-oriented description of the table';


-- ----------------------------------------------------------------------
-- TABLE sl_sequence
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_sequence (
	seq_id				int4,
	seq_reloid			oid UNIQUE NOT NULL,
	seq_relname			name NOT NULL,
	seq_nspname			name NOT NULL,
	seq_set				int4,
	seq_comment			text,

	CONSTRAINT "sl_sequence-pkey"
		PRIMARY KEY (seq_id),
	CONSTRAINT "seq_set-set_id-ref"
		FOREIGN KEY (seq_set)
		REFERENCES @NAMESPACE@.sl_set (set_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_sequence is 'Similar to sl_table, each entry identifies a sequence being replicated.';
comment on column @NAMESPACE@.sl_sequence.seq_id is 'An internally-used ID for Slony-I to use in its sequencing of updates';
comment on column @NAMESPACE@.sl_sequence.seq_reloid is 'The OID of the sequence object';
comment on column @NAMESPACE@.sl_sequence.seq_relname is 'The name of the sequence in pg_catalog.pg_class.relname used to recover from a dump/restore cycle'; 
comment on column @NAMESPACE@.sl_sequence.seq_nspname is 'The name of the schema in pg_catalog.pg_namespace.nspname used to recover from a dump/restore cycle'; 
comment on column @NAMESPACE@.sl_sequence.seq_set is 'Indicates which replication set the object is in';
comment on column @NAMESPACE@.sl_sequence.seq_comment is 'A human-oriented comment';


-- ----------------------------------------------------------------------
-- TABLE sl_path
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_path (
	pa_server			int4,
	pa_client			int4,
	pa_conninfo			text NOT NULL,
	pa_connretry		int4,

	CONSTRAINT "sl_path-pkey"
		PRIMARY KEY (pa_server, pa_client),
	CONSTRAINT "pa_server-no_id-ref"
		FOREIGN KEY (pa_server)
		REFERENCES @NAMESPACE@.sl_node (no_id),
	CONSTRAINT "pa_client-no_id-ref"
		FOREIGN KEY (pa_client)
		REFERENCES @NAMESPACE@.sl_node (no_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_path is 'Holds connection information for the paths between nodes, and the synchronisation delay';
comment on column @NAMESPACE@.sl_path.pa_server is 'The Node ID # (from sl_node.no_id) of the data source';
comment on column @NAMESPACE@.sl_path.pa_client is 'The Node ID # (from sl_node.no_id) of the data target';
comment on column @NAMESPACE@.sl_path.pa_conninfo is 'The PostgreSQL connection string used to connect to the source node.';
comment on column @NAMESPACE@.sl_path.pa_connretry is 'The synchronisation delay, in seconds';


-- ----------------------------------------------------------------------
-- TABLE sl_listen
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_listen (
	li_origin			int4,
	li_provider			int4,
	li_receiver			int4,

	CONSTRAINT "sl_listen-pkey"
		PRIMARY KEY (li_origin, li_provider, li_receiver),
	CONSTRAINT "li_origin-no_id-ref"
		FOREIGN KEY (li_origin)
		REFERENCES @NAMESPACE@.sl_node (no_id),
	CONSTRAINT "sl_listen-sl_path-ref"
		FOREIGN KEY (li_provider, li_receiver)
		REFERENCES @NAMESPACE@.sl_path (pa_server, pa_client)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_listen is 'Indicates how nodes listen to events from other nodes in the Slony-I network.';
comment on column @NAMESPACE@.sl_listen.li_origin is 'The ID # (from sl_node.no_id) of the node this listener is operating on';
comment on column @NAMESPACE@.sl_listen.li_provider is 'The ID # (from sl_node.no_id) of the source node for this listening event';
comment on column @NAMESPACE@.sl_listen.li_receiver is 'The ID # (from sl_node.no_id) of the target node for this listening event';


-- ----------------------------------------------------------------------
-- TABLE sl_subscribe
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_subscribe (
	sub_set				int4,
	sub_provider		int4,
	sub_receiver		int4,
	sub_forward			bool,
	sub_active			bool,

	CONSTRAINT "sl_subscribe-pkey"
		PRIMARY KEY (sub_receiver, sub_set),
	CONSTRAINT "sl_subscribe-sl_path-ref"
		FOREIGN KEY (sub_provider, sub_receiver)
		REFERENCES @NAMESPACE@.sl_path (pa_server, pa_client),
	CONSTRAINT "sub_set-set_id-ref"
		FOREIGN KEY (sub_set)
		REFERENCES @NAMESPACE@.sl_set (set_id)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_subscribe is 'Holds a list of subscriptions on sets';
comment on column @NAMESPACE@.sl_subscribe.sub_set is 'ID # (from sl_set) of the set being subscribed to';
comment on column @NAMESPACE@.sl_subscribe.sub_provider is 'ID# (from sl_node) of the node providing data';
comment on column @NAMESPACE@.sl_subscribe.sub_receiver is 'ID# (from sl_node) of the node receiving data from the provider';
comment on column @NAMESPACE@.sl_subscribe.sub_forward is 'Does this provider keep data in sl_log_1/sl_log_2 to allow it to be a provider for other nodes?';
comment on column @NAMESPACE@.sl_subscribe.sub_active is 'Has this subscription been activated?  This is not set on the subscriber until AFTER the subscriber has received COPY data from the provider';


-- ----------------------------------------------------------------------
-- TABLE sl_event
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_event (
	ev_origin			int4,
	ev_seqno			int8,
	ev_timestamp		timestamptz,
	ev_snapshot			"pg_catalog".txid_snapshot,
	ev_type				text,
	ev_data1			text,
	ev_data2			text,
	ev_data3			text,
	ev_data4			text,
	ev_data5			text,
	ev_data6			text,
	ev_data7			text,
	ev_data8			text,

	CONSTRAINT "sl_event-pkey"
		PRIMARY KEY (ev_origin, ev_seqno)
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_event is 'Holds information about replication events.  After a period of time, Slony removes old confirmed events from both this table and the sl_confirm table.';
comment on column @NAMESPACE@.sl_event.ev_origin is 'The ID # (from sl_node.no_id) of the source node for this event';
comment on column @NAMESPACE@.sl_event.ev_seqno is 'The ID # for the event';
comment on column @NAMESPACE@.sl_event.ev_timestamp is 'When this event record was created';
comment on column @NAMESPACE@.sl_event.ev_snapshot is 'TXID snapshot on provider node for this event';
comment on column @NAMESPACE@.sl_event.ev_seqno is 'The ID # for the event';
comment on column @NAMESPACE@.sl_event.ev_type is 'The type of event this record is for.  
				SYNC				= Synchronise
				STORE_NODE			=
				ENABLE_NODE			=
				DROP_NODE			=
				STORE_PATH			=
				DROP_PATH			=
				STORE_LISTEN		=
				DROP_LISTEN			=
				STORE_SET			=
				DROP_SET			=
				MERGE_SET			=
				SET_ADD_TABLE		=
				SET_ADD_SEQUENCE	=
				STORE_TRIGGER		=
				DROP_TRIGGER		=
				MOVE_SET			=
				ACCEPT_SET			=
				SET_DROP_TABLE			=
				SET_DROP_SEQUENCE		=
				SET_MOVE_TABLE			=
				SET_MOVE_SEQUENCE		=
				FAILOVER_SET		=
				SUBSCRIBE_SET		=
				ENABLE_SUBSCRIPTION	=
				UNSUBSCRIBE_SET		=
				DDL_SCRIPT			=
				ADJUST_SEQ			=
				RESET_CONFIG		=
';
comment on column @NAMESPACE@.sl_event.ev_data1 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data2 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data3 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data4 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data5 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data6 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data7 is 'Data field containing an argument needed to process the event';
comment on column @NAMESPACE@.sl_event.ev_data8 is 'Data field containing an argument needed to process the event';


-- ----------------------------------------------------------------------
-- TABLE sl_confirm
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_confirm (
	con_origin			int4,
	con_received		int4,
	con_seqno			int8,
	con_timestamp		timestamptz DEFAULT timeofday()::timestamptz
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_confirm is 'Holds confirmation of replication events.  After a period of time, Slony removes old confirmed events from both this table and the sl_event table.';

comment on column @NAMESPACE@.sl_confirm.con_origin is 'The ID # (from sl_node.no_id) of the source node for this event';
comment on column @NAMESPACE@.sl_confirm.con_seqno is 'The ID # for the event';
comment on column @NAMESPACE@.sl_confirm.con_timestamp is 'When this event was confirmed';

create index sl_confirm_idx1 on @NAMESPACE@.sl_confirm
	(con_origin, con_received, con_seqno);
create index sl_confirm_idx2 on @NAMESPACE@.sl_confirm
	(con_received, con_seqno);

-- ----------------------------------------------------------------------
-- TABLE sl_seqlog
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_seqlog (
	seql_seqid			int4,
	seql_origin			int4,
	seql_ev_seqno		int8,
	seql_last_value		int8
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_seqlog is 'Log of Sequence updates';

comment on column @NAMESPACE@.sl_seqlog.seql_seqid is 'Sequence ID';
comment on column @NAMESPACE@.sl_seqlog.seql_origin is 'Publisher node at which the sequence originates';
comment on column @NAMESPACE@.sl_seqlog.seql_ev_seqno is 'Slony-I Event with which this sequence update is associated';
comment on column @NAMESPACE@.sl_seqlog.seql_last_value is 'Last value published for this sequence';

create index sl_seqlog_idx on @NAMESPACE@.sl_seqlog
	(seql_origin, seql_ev_seqno, seql_seqid);


-- ----------------------------------------------------------------------
-- FUNCTION sequenceLastValue (seqname)
--
--	Support function used in sl_seqlastvalue view
-- ----------------------------------------------------------------------
create function @NAMESPACE@.sequenceLastValue(p_seqname text) returns int8
as $$
declare
	v_seq_row	record;
begin
	for v_seq_row in execute 'select last_value from ' || @NAMESPACE@.slon_quote_input(p_seqname)
	loop
		return v_seq_row.last_value;
	end loop;

	-- not reached
end;
$$ language plpgsql;

comment on function @NAMESPACE@.sequenceLastValue(p_seqname text) is
'sequenceLastValue(p_seqname)

Utility function used in sl_seqlastvalue view to compactly get the
last value from the requested sequence.';

-- ----------------------------------------------------------------------
-- TABLE sl_log_1
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_log_1 (
	log_origin			int4,
	log_txid			bigint,
	log_tableid			int4,
	log_actionseq		int8,
	log_tablenspname	text,
	log_tablerelname	text,
	log_cmdtype			"char",
	log_cmdupdncols		int4,
	log_cmdargs			text[]
) WITHOUT OIDS;
create index sl_log_1_idx1 on @NAMESPACE@.sl_log_1
	(log_origin, log_txid, log_actionseq);

-- Add in an additional index as sometimes log_origin isn't a useful discriminant
-- create index sl_log_1_idx2 on @NAMESPACE@.sl_log_1
--	(log_txid);

comment on table @NAMESPACE@.sl_log_1 is 'Stores each change to be propagated to subscriber nodes';
comment on column @NAMESPACE@.sl_log_1.log_origin is 'Origin node from which the change came';
comment on column @NAMESPACE@.sl_log_1.log_txid is 'Transaction ID on the origin node';
comment on column @NAMESPACE@.sl_log_1.log_tableid is 'The table ID (from sl_table.tab_id) that this log entry is to affect';
comment on column @NAMESPACE@.sl_log_1.log_actionseq is 'The sequence number in which actions will be applied on replicas';
comment on column @NAMESPACE@.sl_log_1.log_tablenspname is 'The schema name of the table affected';
comment on column @NAMESPACE@.sl_log_1.log_tablerelname is 'The table name of the table affected';
comment on column @NAMESPACE@.sl_log_1.log_cmdtype is 'Replication action to take. U = Update, I = Insert, D = DELETE, T = TRUNCATE';
comment on column @NAMESPACE@.sl_log_1.log_cmdupdncols is 'For cmdtype=U the number of updated columns in cmdargs';
comment on column @NAMESPACE@.sl_log_1.log_cmdargs is 'The data needed to perform the log action on the replica';

-- ----------------------------------------------------------------------
-- TABLE sl_log_2
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_log_2 (
	log_origin			int4,
	log_txid			bigint,
	log_tableid			int4,
	log_actionseq		int8,
	log_tablenspname	text,
	log_tablerelname	text,
	log_cmdtype			"char",
	log_cmdupdncols		int4,
	log_cmdargs			text[]
) WITHOUT OIDS;
create index sl_log_2_idx1 on @NAMESPACE@.sl_log_2
	(log_origin, log_txid, log_actionseq);

-- Add in an additional index as sometimes log_origin isn't a useful discriminant
-- create index sl_log_2_idx2 on @NAMESPACE@.sl_log_2
-- 	(log_txid);

comment on table @NAMESPACE@.sl_log_2 is 'Stores each change to be propagated to subscriber nodes';
comment on column @NAMESPACE@.sl_log_2.log_origin is 'Origin node from which the change came';
comment on column @NAMESPACE@.sl_log_2.log_txid is 'Transaction ID on the origin node';
comment on column @NAMESPACE@.sl_log_2.log_tableid is 'The table ID (from sl_table.tab_id) that this log entry is to affect';
comment on column @NAMESPACE@.sl_log_2.log_actionseq is 'The sequence number in which actions will be applied on replicas';
comment on column @NAMESPACE@.sl_log_2.log_tablenspname is 'The schema name of the table affected';
comment on column @NAMESPACE@.sl_log_2.log_tablerelname is 'The table name of the table affected';
comment on column @NAMESPACE@.sl_log_2.log_cmdtype is 'Replication action to take. U = Update, I = Insert, D = DELETE, T = TRUNCATE';
comment on column @NAMESPACE@.sl_log_2.log_cmdupdncols is 'For cmdtype=U the number of updated columns in cmdargs';
comment on column @NAMESPACE@.sl_log_2.log_cmdargs is 'The data needed to perform the log action on the replica';

-- ----------------------------------------------------------------------
-- TABLE sl_log_script
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_log_script (
	log_origin			int4,
	log_txid			bigint,
	log_actionseq		int8,
	log_cmdtype			"char",
	log_cmdargs			text[]
) WITHOUT OIDS;
create index sl_log_script_idx1 on @NAMESPACE@.sl_log_script
	(log_origin, log_txid, log_actionseq);

comment on table @NAMESPACE@.sl_log_script is 'Captures SQL script queries to be propagated to subscriber nodes';
comment on column @NAMESPACE@.sl_log_script.log_origin is 'Origin name from which the change came';
comment on column @NAMESPACE@.sl_log_script.log_txid is 'Transaction ID on the origin node';
comment on column @NAMESPACE@.sl_log_script.log_actionseq is 'The sequence number in which actions will be applied on replicas';
comment on column @NAMESPACE@.sl_log_2.log_cmdtype is 'Replication action to take. S = Script statement, s = Script complete';
comment on column @NAMESPACE@.sl_log_script.log_cmdargs is 'The DDL statement, optionally followed by selected nodes to execute it on.';

-- ----------------------------------------------------------------------
-- TABLE sl_registry
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_registry (
	reg_key				text primary key,
	reg_int4			int4,
	reg_text			text,
	reg_timestamp		timestamptz
) WITHOUT OIDS;
comment on table @NAMESPACE@.sl_registry is 'Stores miscellaneous runtime data';
comment on column @NAMESPACE@.sl_registry.reg_key is 'Unique key of the runtime option';
comment on column @NAMESPACE@.sl_registry.reg_int4 is 'Option value if type int4';
comment on column @NAMESPACE@.sl_registry.reg_text is 'Option value if type text';
comment on column @NAMESPACE@.sl_registry.reg_timestamp is 'Option value if type timestamp';


-- ----------------------------------------------------------------------
-- TABLE sl_apply_stats
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_apply_stats (
	as_origin			int4,
	as_num_insert		int8,
	as_num_update		int8,
	as_num_delete		int8,
	as_num_truncate		int8,
	as_num_script		int8,
	as_num_total		int8,
	as_duration			interval,
	as_apply_first		timestamptz,
	as_apply_last		timestamptz,
	as_cache_prepare	int8,
	as_cache_hit		int8,
	as_cache_evict		int8,
	as_cache_prepare_max int8
) WITHOUT OIDS;

create index sl_apply_stats_idx1 on @NAMESPACE@.sl_apply_stats
	(as_origin);

comment on table @NAMESPACE@.sl_apply_stats is 'Local SYNC apply statistics (running totals)';
comment on column @NAMESPACE@.sl_apply_stats.as_origin is 'Origin of the SYNCs';
comment on column @NAMESPACE@.sl_apply_stats.as_num_insert is 'Number of INSERT operations performed';
comment on column @NAMESPACE@.sl_apply_stats.as_num_update is 'Number of UPDATE operations performed';
comment on column @NAMESPACE@.sl_apply_stats.as_num_delete is 'Number of DELETE operations performed';
comment on column @NAMESPACE@.sl_apply_stats.as_num_truncate is 'Number of TRUNCATE operations performed';
comment on column @NAMESPACE@.sl_apply_stats.as_num_script is 'Number of DDL operations performed';
comment on column @NAMESPACE@.sl_apply_stats.as_num_total is 'Total number of operations';
comment on column @NAMESPACE@.sl_apply_stats.as_duration is 'Processing time';
comment on column @NAMESPACE@.sl_apply_stats.as_apply_first is 'Timestamp of first recorded SYNC';
comment on column @NAMESPACE@.sl_apply_stats.as_apply_last is 'Timestamp of most recent recorded SYNC';
comment on column @NAMESPACE@.sl_apply_stats.as_cache_evict is 'Number of apply query cache evict operations';
comment on column @NAMESPACE@.sl_apply_stats.as_cache_prepare_max is 'Maximum number of apply queries prepared in one SYNC group';


-- **********************************************************************
-- * Views
-- **********************************************************************
-- ----------------------------------------------------------------------
-- VIEW sl_seqlastvalue
-- ----------------------------------------------------------------------
create view @NAMESPACE@.sl_seqlastvalue as
	select SQ.seq_id, SQ.seq_set, SQ.seq_reloid,
			S.set_origin as seq_origin,
			@NAMESPACE@.sequenceLastValue(
					"pg_catalog".quote_ident(PGN.nspname) || '.' ||
					"pg_catalog".quote_ident(PGC.relname)) as seq_last_value
		from @NAMESPACE@.sl_sequence SQ, @NAMESPACE@.sl_set S,
			"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
		where S.set_id = SQ.seq_set
			and PGC.oid = SQ.seq_reloid and PGN.oid = PGC.relnamespace;
		

create view @NAMESPACE@.sl_failover_targets as
select  set_id,
	set_origin as set_origin,
	sub1.sub_receiver as backup_id

FROM
	@NAMESPACE@.sl_subscribe sub1
	,@NAMESPACE@.sl_set set1
	where
 	sub1.sub_set=set_id
	and sub1.sub_forward=true
	--exclude candidates where the set_origin
	--has a path a node but the failover
	--candidate has no path to that node
	and sub1.sub_receiver not in
	    (select p1.pa_client from
	    @NAMESPACE@.sl_path p1 
	    left outer join @NAMESPACE@.sl_path p2 on
	    (p2.pa_client=p1.pa_client 
	    and p2.pa_server=sub1.sub_receiver)
	    where p2.pa_client is null
	    and p1.pa_server=set_origin
	    and p1.pa_client<>sub1.sub_receiver
	    )
	and sub1.sub_provider=set_origin
	--exclude any subscribers that are not
	--direct subscribers of all sets on the
	--origin
	and sub1.sub_receiver not in
	(select direct_recv.sub_receiver
		from
			
		(--all direct receivers of the first set
		select subs2.sub_receiver
		from @NAMESPACE@.sl_subscribe subs2
		where subs2.sub_provider=set1.set_origin
		      and subs2.sub_set=set1.set_id) as
		      direct_recv
		inner join
		(--all other sets from the origin
		select set_id from @NAMESPACE@.sl_set set2
		where set2.set_origin=set1.set_origin
		and set2.set_id<>sub1.sub_set)
		as othersets on(true)
		left outer join @NAMESPACE@.sl_subscribe subs3
		on(subs3.sub_set=othersets.set_id
		   and subs3.sub_forward=true
		   and subs3.sub_provider=set1.set_origin
		   and direct_recv.sub_receiver=subs3.sub_receiver)
	    where subs3.sub_receiver is null
	    );

		      
	

	    

-- **********************************************************************
-- * Sequences
-- *********************************************************************


-- ----------------------------------------------------------------------
-- SEQUENCE sl_local_node_id
--
--	The local node ID is initialized to -1, meaning that this node
--	is not initialized yet.
-- ----------------------------------------------------------------------
create sequence @NAMESPACE@.sl_local_node_id
	MINVALUE -1;
SELECT setval('@NAMESPACE@.sl_local_node_id', -1);
comment on sequence @NAMESPACE@.sl_local_node_id is 'The local node ID is initialized to -1, meaning that this node is not initialized yet.';


-- ----------------------------------------------------------------------
-- SEQUENCE sl_event_seq
--
--	The sequence for numbering events originating from this node.
-- ----------------------------------------------------------------------
create sequence @NAMESPACE@.sl_event_seq;
comment on sequence @NAMESPACE@.sl_event_seq is 'The sequence for numbering events originating from this node.';
select setval('@NAMESPACE@.sl_event_seq', 5000000000);

-- ----------------------------------------------------------------------
-- SEQUENCE sl_action_seq
--
--	The sequence to number statements in the transaction logs, so that
--	the replication engines can figure out the "agreeable" order of
--	statements.
-- ----------------------------------------------------------------------
create sequence @NAMESPACE@.sl_action_seq;
comment on sequence @NAMESPACE@.sl_action_seq is 'The sequence to number statements in the transaction logs, so that the replication engines can figure out the "agreeable" order of statements.';




-- ----------------------------------------------------------------------
-- SEQUENCE sl_log_status
--
--	Bit 0x01 determines the currently active log table
--	Bit 0x02 tells if the engine needs to read both logs
--	after switching until the old log is clean and truncated.
--
--	Possible values:
--		0		sl_log_1 active, sl_log_2 clean
--		1		sl_log_2 active, sl_log_1 clean
--		2		sl_log_1 active, sl_log_2 unknown - cleanup
--		3		sl_log_2 active, sl_log_1 unknown - cleanup
-- ----------------------------------------------------------------------
create sequence @NAMESPACE@.sl_log_status
	MINVALUE 0 MAXVALUE 3;
SELECT setval('@NAMESPACE@.sl_log_status', 0);
comment on sequence @NAMESPACE@.sl_log_status is '
Bit 0x01 determines the currently active log table
Bit 0x02 tells if the engine needs to read both logs
after switching until the old log is clean and truncated.

Possible values:
	0		sl_log_1 active, sl_log_2 clean
	1		sl_log_2 active, sl_log_1 clean
	2		sl_log_1 active, sl_log_2 unknown - cleanup
	3		sl_log_2 active, sl_log_1 unknown - cleanup

This is not yet in use.
';


-- **********************************************************************
-- * Misc
-- **********************************************************************


-- ----------------------------------------------------------------------
-- TABLE sl_config_lock
--
--	This table exists solely to prevent overlapping execution of
--	configuration change procedures and the resulting possible
--	deadlocks.
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_config_lock (
	dummy				integer
);
comment on table @NAMESPACE@.sl_config_lock is 'This table exists solely to prevent overlapping execution of configuration change procedures and the resulting possible deadlocks.
';
comment on column @NAMESPACE@.sl_config_lock.dummy is 'No data ever goes in this table so the contents never matter.  Indeed, this column does not really need to exist.';

-- ----------------------------------------------------------------------
-- TABLE sl_event_lock
--
--	This table exists solely to prevent multiple connections from
--	concurrently creating new events.  We separate this from
--	sl_event because we really don't need a lock that prevents
--	*reads* on sl_event.
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_event_lock (
	dummy				integer
);
comment on table @NAMESPACE@.sl_event_lock is 'This table exists solely to prevent multiple connections from concurrently creating new events and perhaps getting them out of order.';
comment on column @NAMESPACE@.sl_event_lock.dummy is 'No data ever goes in this table so the contents never matter.  Indeed, this column does not really need to exist.';

-- ----------------------------------------------------------------------
-- TABLE sl_archive_counter
--
--	This table is used to generate the archive number for logshipping.
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_archive_counter (
	ac_num			bigint,
	ac_timestamp	timestamptz
) without oids;
comment on table @NAMESPACE@.sl_archive_counter is 'Table used to generate the log shipping archive number.
';
comment on column @NAMESPACE@.sl_archive_counter.ac_num is 'Counter of SYNC ID used in log shipping as the archive number';
comment on column @NAMESPACE@.sl_archive_counter.ac_timestamp is 'Time at which the archive log was generated on the subscriber';

insert into @NAMESPACE@.sl_archive_counter (ac_num, ac_timestamp)
	values (0, 'epoch'::timestamptz);

-- ----------------------------------------------------------------------
-- TABLE sl_components
--
--  This table captures the state of each Slony component to help
--  with monitoring
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_components (
	co_actor	 text not null primary key,
	co_pid		 integer not null,
	co_node		 integer not null,
	co_connection_pid integer not null,
	co_activity	  text,
	co_starttime	  timestamptz not null,
	co_event	  bigint,
	co_eventtype 	  text
) without oids;

comment on table @NAMESPACE@.sl_components is 'Table used to monitor what various slon/slonik components are doing';
comment on column @NAMESPACE@.sl_components.co_actor is 'which component am I?';
comment on column @NAMESPACE@.sl_components.co_pid is 'my process/thread PID on node where slon runs';
comment on column @NAMESPACE@.sl_components.co_node is 'which node am I servicing?';
comment on column @NAMESPACE@.sl_components.co_connection_pid is 'PID of database connection being used on database server';
comment on column @NAMESPACE@.sl_components.co_activity is 'activity that I am up to';
comment on column @NAMESPACE@.sl_components.co_starttime is 'when did my activity begin?  (timestamp reported as per slon process on server running slon)';
comment on column @NAMESPACE@.sl_components.co_eventtype is 'what kind of event am I processing?  (commonly n/a for event loop main threads)';
comment on column @NAMESPACE@.sl_components.co_event is 'which event have I started processing?';



--
-- we create a function + aggregate for string_agg to aggregate strings
-- some versions of PG (ie prior to 9.0) don't support this
CREATE OR replace function @NAMESPACE@.agg_text_sum(txt_before TEXT, txt_new TEXT) RETURNS TEXT AS
$BODY$
DECLARE
  c_delim text;
BEGIN
    c_delim = ',';
	IF (txt_before IS NULL or txt_before='') THEN
	   RETURN txt_new;
	END IF;
	RETURN txt_before || c_delim || txt_new;
END;
$BODY$
LANGUAGE plpgsql;
comment on function @NAMESPACE@.agg_text_sum(text,text) is 
'An accumulator function used by the slony string_agg function to
aggregate rows into a string';
--
-- create a string_agg function in the slony schema.
-- PG 8.3 does not have this function so we make our own
-- when slony stops supporting PG 8.3 we can switch to
-- the PG 9.0+ provided version of string_agg
--
CREATE AGGREGATE @NAMESPACE@.string_agg(text) (
SFUNC=@NAMESPACE@.agg_text_sum,
STYPE=text,
INITCOND=''
);


-- ----------------------------------------------------------------------
-- Last but not least grant USAGE to the replication schema objects.
-- ----------------------------------------------------------------------
grant usage on schema @NAMESPACE@ to public;

