#!/bin/sh
# ----------
# slony1_dump.sh
#
# 
#
#	This script creates a special data only dump from a subscriber
#	node. The stdout of this script, fed into psql for a database that
#	has the user schema of the replicated database installed, will
#	prepare that database for log archive application.
# ----------

# ----
# Check for correct usage
# ----
if test $# -lt 2 ; then
	echo "usage: $0 subscriber-dbname clustername [-omit_copy ]" >&2
	exit 1
fi


# ----
# Remember call arguments and get the nodeId of the DB specified
# ----
dbname=$1
cluster=$2
clname="\"_$2\""
omit_copy=0
if test $# -eq 3; then
	if [ "$3" = "-omit_copy" ];
	then
		omit_copy=1
	else
		echo "usage: $0 subscriber-dbname clustername [-omit_copy ]" >&2
		exit 1	
    fi
fi

pgc="\"pg_catalog\""
nodeid=`psql -q -At -c "select \"_$cluster\".getLocalNodeId('_$cluster')" $dbname`

# ----
# Get a list of all replicated table ID's this subscriber receives,
# and remember the table names.
# ----
tables=`psql -q -At -d $dbname -c \
		"select tab_id from $clname.sl_table, $clname.sl_set
				where tab_set = set_id
					and exists (select 1 from $clname.sl_subscribe
							where sub_set = set_id
								and sub_receiver = $nodeid)"`
for tab in $tables ; do
	eval tabname_$tab=`psql -q -At -d $dbname -c \
			"select $pgc.quote_ident(tab_nspname) || '.' || 
					$pgc.quote_ident(tab_relname) from 
					$clname.sl_table where tab_id = $tab"`
done

# ----
# Get a list of all replicated sequence ID's this subscriber receives,
# and remember the sequence names.
# ----
sequences=`psql -q -At -d $dbname -c \
		"select seq_id from $clname.sl_sequence, $clname.sl_set
				where seq_set = set_id
					and exists (select 1 from $clname.sl_subscribe
							where sub_set = set_id
								and sub_receiver = $nodeid)"`
for seq in $sequences ; do
	eval seqname_$seq=`psql -q -At -d $dbname -c \
			"select $pgc.quote_ident(seq_nspname) || '.' || 
					$pgc.quote_ident(seq_relname) from 
					$clname.sl_sequence where seq_id = $seq"`
done


# ----
# Emit SQL code to create the slony specific object required
# in the remote database.
# ----
cat <<_EOF_
start transaction;


-- ----------------------------------------------------------------------
-- SCHEMA $clname
-- ----------------------------------------------------------------------
create schema $clname;


-- ----------------------------------------------------------------------
-- TABLE sl_archive_tracking
-- ----------------------------------------------------------------------
create table $clname.sl_archive_tracking (
	at_counter			bigint,
	at_created			timestamp,
	at_applied			timestamp
);

-- -----------------------------------------------------------------------------
-- FUNCTION sequenceSetValue_offline (seq_id, last_value)
-- -----------------------------------------------------------------------------
create or replace function $clname.sequenceSetValue_offline(text,text, int8) returns int4
as '
declare
	p_seq_nsp			alias for \$1;
    p_seq_name          alias for \$2;
	p_last_value		alias for \$3;

begin


	-- ----
	-- Update it to the new value
	-- ----
	execute '' select setval(''''''|| p_seq_nsp || ''.'' || 
			p_seq_name || '''''', '''''' || p_last_value || '''''')'';
	return 0;
end;
' language plpgsql;
-- ---------------------------------------------------------------------------------------
-- FUNCTION finishTableAfterCopy(table_id)
-- ---------------------------------------------------------------------------------------
-- This can just be a simple stub function; it does not need to do anything...
-- ---------------------------------------------------------------------------------------
create or replace function $clname.finishTableAfterCopy(int4) returns int4 as
  'select 1'
language sql;

-- ---------------------------------------------------------------------------------------
-- FUNCTION archiveTracking_offline (new_counter, created_timestamp)
-- ---------------------------------------------------------------------------------------
create or replace function $clname.archiveTracking_offline(int8, timestamp) returns int8
as '
declare
	p_new_seq	alias for \$1;
	p_created	alias for \$2;
	v_exp_seq	int8;
	v_old_seq	int8;
begin
	select at_counter into v_old_seq from $clname.sl_archive_tracking;
	if not found then
		raise exception ''Slony-I: current archive tracking status not found'';
	end if;

	v_exp_seq := p_new_seq - 1;
	if v_old_seq <> v_exp_seq then
		raise exception ''Slony-I: node is on archive counter %, this archive log expects %'', 
			v_old_seq, v_exp_seq;
	end if;
	raise notice ''Slony-I: Process archive with counter % created %'', p_new_seq, p_created;

	update $clname.sl_archive_tracking
		set at_counter = p_new_seq,
			at_created = p_created,
			at_applied = CURRENT_TIMESTAMP;
	return p_new_seq;
end;
' language plpgsql;
create table $clname.sl_log_archive (
	log_origin			int4,
	log_txid			bigint,
	log_tableid			int4,
	log_actionseq		int8,
	log_tablenspname	text,
	log_tablerelname	text,
	log_cmdtype			char,
	log_cmdupdncols		int4,
	log_cmdargs			text[]
) WITHOUT OIDS;



create or replace function $clname.slon_quote_brute(p_tab_fqname text) returns text
as \$\$
declare	
    v_fqname text default '';
begin
    v_fqname := '"' || replace(p_tab_fqname,'"','""') || '"';
    return v_fqname;
end;
\$\$ language plpgsql immutable;


create or replace function $clname.log_apply() returns trigger
as \$\$
declare
	v_command	text = 'not implemented yet';
	v_list1		text = '';
	v_list2		text = '';
	v_comma		text = '';
	v_and		text = '';
	v_idx		integer = 1;
	v_nargs		integer;
	v_i			integer = 0;
begin
	v_nargs = array_upper(NEW.log_cmdargs, 1);

	if NEW.log_cmdtype = 'I' then
		while v_idx < v_nargs loop
			v_list1 = v_list1 || v_comma ||
				$clname.slon_quote_brute(NEW.log_cmdargs[v_idx]);
			v_idx = v_idx + 1;
			if NEW.log_cmdargs[v_idx] is null then
			   v_list2 = v_list2 || v_comma || 'null';
			else 
			     v_list2 = v_list2 || v_comma ||
			     	pg_catalog.quote_literal(NEW.log_cmdargs[v_idx]);			end if;
			v_idx = v_idx + 1;

			v_comma = ',';
		end loop;

		v_command = 'INSERT INTO ' || 
			$clname.slon_quote_brute(NEW.log_tablenspname) || '.' ||
			$clname.slon_quote_brute(NEW.log_tablerelname) || ' (' ||
			v_list1 || ') VALUES (' || v_list2 || ')';

		execute v_command;
	end if;
	if NEW.log_cmdtype = 'U' then
		v_command = 'UPDATE ONLY ' ||
			$clname.slon_quote_brute(NEW.log_tablenspname) || '.' ||
			$clname.slon_quote_brute(NEW.log_tablerelname) || ' SET ';
		while v_i < NEW.log_cmdupdncols loop		      	
			v_command = v_command || v_comma ||
				$clname.slon_quote_brute(NEW.log_cmdargs[v_idx]) || '=';
			v_idx = v_idx + 1;
			if NEW.log_cmdargs[v_idx] is null then
			   v_command = v_command || 'null';
			else 
			     v_command = v_command ||
				pg_catalog.quote_literal(NEW.log_cmdargs[v_idx]);
			end if;
			v_idx = v_idx + 1;
			v_comma = ',';
			v_i = v_i + 1;
		end loop;
		if NEW.log_cmdupdncols = 0 then
			v_command = v_command ||
				$clname.slon_quote_brute(NEW.log_cmdargs[1]) || '=' ||
				$clname.slon_quote_brute(NEW.log_cmdargs[1]);
		end if;
		v_command = v_command || ' WHERE ';
		while v_idx < v_nargs loop
			v_command = v_command || v_and ||
				$clname.slon_quote_brute(NEW.log_cmdargs[v_idx]) || '=';
			v_idx = v_idx + 1;
			if NEW.log_cmdargs[v_idx] is null then
			   v_command = v_command || 'null';
			else
				v_command = v_command ||
					  pg_catalog.quote_literal(NEW.log_cmdargs[v_idx]);
			end if;
			v_idx = v_idx + 1;

			v_and = ' AND ';
		end loop;
		execute v_command;
	end if;

	if NEW.log_cmdtype = 'D' then
		v_command = 'DELETE FROM ONLY ' ||
			$clname.slon_quote_brute(NEW.log_tablenspname) || '.' ||
			$clname.slon_quote_brute(NEW.log_tablerelname) || ' WHERE ';
		while v_idx < v_nargs loop
			v_command = v_command || v_and ||
				$clname.slon_quote_brute(NEW.log_cmdargs[v_idx]) || '=';
			v_idx = v_idx + 1;
			if NEW.log_cmdargs[v_idx] is null then
			   v_command = v_command || 'null';
			else
				v_command = v_command ||
					  pg_catalog.quote_literal(NEW.log_cmdargs[v_idx]);
			end if;
			v_idx = v_idx + 1;

			v_and = ' AND ';
		end loop;

		execute v_command;
	end if;
    if NEW.log_cmdtype = 'S' then
          execute 'set session_replication_role to local;';
          execute NEW.log_cmdargs[1];  
          execute 'set session_replication_role to replica;';

    end if;
	if NEW.log_cmdtype = 'T' then
		execute 'TRUNCATE TABLE ONLY ' ||
			$clname.slon_quote_brute(NEW.log_tablenspname) || '.' ||
			$clname.slon_quote_brute(NEW.log_tablerelname) || ' CASCADE';
	end if;

	return NULL;
end;
\$\$ language plpgsql;
create trigger apply_trigger
		before INSERT on $clname.sl_log_archive
		for each row execute procedure $clname.log_apply();
alter table $clname.sl_log_archive
	  enable replica trigger apply_trigger;
set session_replication_role='replica';

_EOF_


if [ "$omit_copy" != "1" ]; then
	for tab in $tables
	do
		eval tabname=\$tabname_$tab
		echo "truncate $tabname cascade;";
	done
fi

# ----
# The remainder of this script is written in a way that
# all output is generated by psql inside of one serializable
# transaction, so that we get a consistent snapshot of the
# replica.
# ----

(
echo "start transaction;"
echo "set transaction isolation level serializable;"

# ----
# Provide initial values for all sequences.
# ----
for seq in $sequences ; do
	eval seqname=\$seqname_$seq
	schema=`echo $seqname|cut -d'.' -f1`
	name=`echo $seqname|cut -d'.' -f2`
	echo "select E'select $clname.sequenceSetValue_offline(''$schema'',''$name'', ''' || last_value::text || E''');' from $seqname;"
done

# ----
# Fill the setsync tracking table with the current status
# ----
echo "select 'insert into $clname.sl_archive_tracking values (' ||
			ac_num::text || ', ''' || ac_timestamp::text || 
			''', CURRENT_TIMESTAMP);'
			from $clname.sl_archive_counter;";

# ----
# Now dump all the user table data
# ----
if [ "$omit_copy" -eq "0" ]; then
	system_type=`uname`
	for tab in $tables ; do
		eval tabname=\$tabname_$tab

		# Get fieldnames...
 		fields=`psql -At -c "select $clname.copyfields($tab);" $dbname`
 		echo "select 'copy $tabname $fields from stdin;';"
		echo "copy $tabname $fields to stdout;"
 		printf "select E'\\\\\\\\.';"
	done
fi

# ----
# Commit the transaction here in the replica that provided us
# with the information.
# ----
echo "commit;"
) | psql -q -At -d $dbname


# ----
# Emit the commit for the dump to stdout.
# ----
echo "commit;"

exit 0

