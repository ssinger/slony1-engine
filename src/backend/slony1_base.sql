-- ----------------------------------------------------------------------
-- slony1_base.sql
--
--    Declaration of the basic replication schema.
--
--	Copyright (c) 2003-2004, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- $Id: slony1_base.sql,v 1.16 2004-09-06 03:42:21 wieck Exp $
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

	CONSTRAINT "sl_node-pkey"
		PRIMARY KEY (no_id)
);
comment on table @NAMESPACE@.sl_node is 'Holds the list of nodes associated with this namespace.  no_id is the unique ID number for the node; no_comment is a human-oriented description of the node';

-- ----------------------------------------------------------------------
-- TABLE sl_set
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_set (
	set_id				int4,
	set_origin			int4,
	set_locked			@NAMESPACE@.xxid,
	set_comment			text,

	CONSTRAINT "sl_set-pkey"
		PRIMARY KEY (set_id),
	CONSTRAINT "set_origin-no_id-ref"
		FOREIGN KEY (set_origin)
		REFERENCES @NAMESPACE@.sl_node (no_id)
);
comment on table @NAMESPACE@.sl_set is 'Holds definitions of replication sets.  

set_id is a unique ID number for the set.
set_origin is the ID number of the source node for the replication set.
set_locked indicates whether or not the set is locked.
set_comment is a human-oriented description of the set.
';


-- ----------------------------------------------------------------------
-- TABLE sl_setsync
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_setsync (
	ssy_setid			int4,
	ssy_origin			int4,
	ssy_seqno			int8,
	ssy_minxid			@NAMESPACE@.xxid,
	ssy_maxxid			@NAMESPACE@.xxid,
	ssy_xip				text,
	ssy_action_list		text,

	CONSTRAINT "sl_setsync-pkey"
		PRIMARY KEY (ssy_setid),
	CONSTRAINT "ssy_setid-set_id-ref"
		FOREIGN KEY (ssy_setid)
		REFERENCES @NAMESPACE@.sl_set (set_id),
	CONSTRAINT "ssy_origin-no_id-ref"
		FOREIGN KEY (ssy_origin)
		REFERENCES @NAMESPACE@.sl_node (no_id)
);
comment on table @NAMESPACE@.sl_setsync is 'Not documented yet';


-- ----------------------------------------------------------------------
-- TABLE sl_table
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_table (
	tab_id				int4,
	tab_reloid			oid UNIQUE NOT NULL,
	tab_set				int4,
	tab_idxname			name NOT NULL,
	tab_altered			boolean NOT NULL,
	tab_comment			text,

	CONSTRAINT "sl_table-pkey"
		PRIMARY KEY (tab_id),
	CONSTRAINT "tab_set-set_id-ref"
		FOREIGN KEY (tab_set)
		REFERENCES @NAMESPACE@.sl_set (set_id)
);
comment on table @NAMESPACE@.sl_table is 'Holds information about the tables being replicated.

tab_id - unique key for Slony-I to use to identify the table
tab_reloid - the OID of the table in pg_catalog.pg_class.oid
tab_idxname - the name of the primary index of the table
tab_comment - Human-oriented description of the table';


-- ----------------------------------------------------------------------
-- TABLE sl_trigger
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_trigger (
	trig_tabid			int4,
	trig_tgname			name,

	CONSTRAINT "sl_trigger-pkey"
		PRIMARY KEY (trig_tabid, trig_tgname),
	CONSTRAINT "trig_tabid-tab_id-ref"
		FOREIGN KEY (trig_tabid)
		REFERENCES @NAMESPACE@.sl_table (tab_id)
		ON DELETE CASCADE
);
comment on table @NAMESPACE@.sl_trigger is 'Indicates the name of a trigger [for what purpose?]';


-- ----------------------------------------------------------------------
-- TABLE sl_sequence
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_sequence (
	seq_id				int4,
	seq_reloid			oid UNIQUE NOT NULL,
	seq_set				int4,
	seq_comment			text,

	CONSTRAINT "sl_sequence-pkey"
		PRIMARY KEY (seq_id),
	CONSTRAINT "seq_set-set_id-ref"
		FOREIGN KEY (seq_set)
		REFERENCES @NAMESPACE@.sl_set (set_id)
);
comment on table @NAMESPACE@.sl_sequence is 'Similar to sl_table, each entry identifies a sequence being replicated.
seq_id is an internally-used ID for Slony-I to use in its sequencing of updates
seq_reloid is the OID of the sequence object
seq_set indicates which replication set the object is in
seq_comment is a human-oriented comment';


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
);
comment on table @NAMESPACE@.sl_path is 'Holds connection information for the paths between nodes, and the synchronisation delay

pa_server - The Node ID # (from sl_node.no_id) of the data source
pa_client - The Node ID # (from sl_node.no_id) of the data target
pa_conninfo - The PostgreSQL connection string used to connect to the source node.
pa_connretry - The synchronisation delay, in seconds
';


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
);
comment on table @NAMESPACE@.sl_listen is 'Indicates how nodes listen to events from other nodes in the Slony-I network.

li_origin		:	Integer.  The ID # (from sl_node.no_id) of the node this listener is operating on
li_provider		:	Integer.  The ID # (from sl_node.no_id) of the source node for this listening event
li_receiver		:	Integer.  The ID # (from sl_node.no_id) of the target node for this listening event
';


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
);
comment on table @NAMESPACE@.sl_subscribe is 'Not documented yet';


-- ----------------------------------------------------------------------
-- TABLE sl_event
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_event (
	ev_origin			int4,
	ev_seqno			int8,
	ev_timestamp		timestamp,
	ev_minxid			@NAMESPACE@.xxid,
	ev_maxxid			@NAMESPACE@.xxid,
	ev_xip				text,
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
);
comment on table @NAMESPACE@.sl_event is 'Holds information about replication events.

After a period of time, Slony removes old confirmed events from both this table and the sl_confirm table.

ev_origin		:	Integer.  The ID # (from sl_node.no_id) of the source node for this event
ev_seqno		:	Integer.  The ID # for the event
ev_timestamp	:	Timestamp.  When this event record was created
ev_minxid		:
ev_maxxid		:
ev_xip			:	String.
ev_type			:	String.  The type of event this record is for.

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
				FAILOVER_SET		=
				SUBSCRIBE_SET		=
				ENABLE_SUBSCRIPTION	=
				UNSUBSCRIBE_SET		=
				DDL_SCRIPT			=
				ADJUST_SEQ			=
';


-- ----------------------------------------------------------------------
-- TABLE sl_confirm
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_confirm (
	con_origin			int4,
	con_received		int4,
	con_seqno			int8,
	con_timestamp		timestamp DEFAULT timeofday()::timestamp
);
comment on table @NAMESPACE@.sl_confirm is 'Holds confirmation of replication events.

After a period of time, Slony removes old confirmed events from both this table and the sl_event table.

con_origin		:	Integer.  The ID # (from sl_node.no_id) of the source node for this event  
con_received	:	Integer.
con_seqno		:	Integer.  The ID # for the event
con_timestamp	:	Timestamp.  When this event was confirmed
';
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
);
comment on table @NAMESPACE@.sl_seqlog is 'Not documented yet';
create index sl_seqlog_idx on @NAMESPACE@.sl_seqlog
	(seql_origin, seql_ev_seqno, seql_seqid);


-- ----------------------------------------------------------------------
-- FUNCTION sequenceLastValue (seqname)
--
--	Support function used in sl_seqlastvalue view
-- ----------------------------------------------------------------------
create function @NAMESPACE@.sequenceLastValue(text) returns int8
as '
declare
	p_seqname	alias for $1;
	v_seq_row	record;
begin
	for v_seq_row in execute ''select last_value from '' || p_seqname
	loop
		return v_seq_row.last_value;
	end loop;

	-- not reached
end;
' language plpgsql;


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
		

-- ----------------------------------------------------------------------
-- TABLE sl_log_1
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_log_1 (
	log_origin			int4,
	log_xid				@NAMESPACE@.xxid,
	log_tableid			int4,
	log_actionseq		int8,
	log_cmdtype			char,
	log_cmddata			text
);
create index sl_log_1_idx1 on @NAMESPACE@.sl_log_1
	(log_origin, log_xid @NAMESPACE@.xxid_ops, log_actionseq);

comment on table @NAMESPACE@.sl_log_1 is 'Stores each change to be propagated to subscriber nodes

log_origin - origin node from which the change came
log_xid - transaction ID on the origin node
log_table_id - the table ID (from sl_table.tab_id) that this log entry is to affect
log_cmdtype - replication action to take. U = Update, I = Insert, D = DELETE
log_cmddata - the data needed to perform the log action';

-- ----------------------------------------------------------------------
-- TABLE sl_log_2
-- ----------------------------------------------------------------------
create table @NAMESPACE@.sl_log_2 (
	log_origin			int4,
	log_xid				@NAMESPACE@.xxid,
	log_tableid			int4,
	log_actionseq		int8,
	log_cmdtype			char,
	log_cmddata			text
);
comment on table @NAMESPACE@.sl_log_2 is 'Stores each change to be propagated to subscriber nodes

log_origin - origin node from which the change came
log_xid - transaction ID on the origin node
log_table_id - the table ID (from sl_table.tab_id) that this log entry is to affect
log_cmdtype - replication action to take. U = Update, I = Insert, D = DELETE
log_cmddata - the data needed to perform the log action';
create index sl_log_2_idx1 on @NAMESPACE@.sl_log_2
	(log_origin, log_xid @NAMESPACE@.xxid_ops, log_actionseq);


-- **********************************************************************
-- * Sequences
-- **********************************************************************


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
-- SEQUENCE sl_rowid_seq
--
--	Application tables that do not have a natural primary key must
--	be modified and an int8 column added that serves as a rowid for us.
--	The values are assigned with a default from this sequence.
-- ----------------------------------------------------------------------
create sequence @NAMESPACE@.sl_rowid_seq;
grant select, update on @NAMESPACE@.sl_rowid_seq to public;
comment on sequence @NAMESPACE@.sl_rowid_seq is 'Application tables that do not have a natural primary key must be modified and an int8 column added that serves as a rowid for us.  The values are assigned with a default from this sequence.';


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


-- ----------------------------------------------------------------------
-- Last but not least grant USAGE to the replication schema objects.
-- ----------------------------------------------------------------------
grant usage on schema @NAMESPACE@ to public;


