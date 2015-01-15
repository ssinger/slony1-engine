-- ----------------------------------------------------------------------
-- slony1_funcs.sql
--
--    Declaration of replication support functions.
--
--	Copyright (c) 2003-2010, PostgreSQL Global Development Group
--	Author: Jan Wieck, Afilias USA INC.
--
-- 
-- ----------------------------------------------------------------------

-- **********************************************************************
-- * C functions in src/backend/slony1_base.c
-- **********************************************************************


-- ----------------------------------------------------------------------
-- FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])
--
--	Create an sl_event entry
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text) 
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text, ev_data7 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text, ev_data7 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

create or replace function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text, ev_data7 text, ev_data8 text)
	returns bigint
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_createEvent'
	language C
	called on null input;

comment on function @NAMESPACE@.createEvent (p_cluster_name name, p_event_type text, ev_data1 text, ev_data2 text, ev_data3 text, ev_data4 text, ev_data5 text, ev_data6 text, ev_data7 text, ev_data8 text) is
'FUNCTION createEvent (cluster_name, ev_type [, ev_data [...]])

Create an sl_event entry';

-- ----------------------------------------------------------------------
-- FUNCTION denyAccess ()
--
--	Trigger function to prevent modifications to a table on
--	a subscriber.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.denyAccess ()
	returns trigger
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_denyAccess'
	language C
	security definer;

comment on function @NAMESPACE@.denyAccess () is 
  'Trigger function to prevent modifications to a table on a subscriber';

grant execute on function @NAMESPACE@.denyAccess () to public;


-- ----------------------------------------------------------------------
-- FUNCTION lockedSet (cluster_name)
--
--	Trigger function to prevent modifications to a table before
--	and after a moveSet().
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.lockedSet ()
	returns trigger
	as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_lockedSet'
	language C;

comment on function @NAMESPACE@.lockedSet () is 
  'Trigger function to prevent modifications to a table before and after a moveSet()';

-- ----------------------------------------------------------------------
-- FUNCTION getLocalNodeId (name)
--
--	
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.getLocalNodeId (p_cluster name) returns int4
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_getLocalNodeId'
	language C
	security definer;
grant execute on function @NAMESPACE@.getLocalNodeId (p_cluster name) to public;

comment on function @NAMESPACE@.getLocalNodeId (p_cluster name) is 
  'Returns the node ID of the node being serviced on the local database';

-- ----------------------------------------------------------------------
-- FUNCTION getModuleVersion ()
--
--	Returns the compiled in version number of the Slony-I shared
--	object.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.getModuleVersion () returns text
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_getModuleVersion'
	language C
	security definer;
grant execute on function @NAMESPACE@.getModuleVersion () to public;

comment on function @NAMESPACE@.getModuleVersion () is
  'Returns the compiled-in version number of the Slony-I shared object';


create or replace function @NAMESPACE@.resetSession() returns text
	   as '$libdir/slony1_funcs.@MODULEVERSION@','_Slony_I_@FUNCVERSION@_resetSession'
	   language C;

-- ----------------------------------------------------------------------
-- FUNCTION logApply ()
--
--	A trigger function that is placed on the tables sl_log_1/2 that
--	does the actual work of updating the user tables.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logApply () returns trigger
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_logApply'
	language C
	security definer;

-- ----------------------------------------------------------------------
-- FUNCTION logApplySetCacheSize ()
--
--	A control function for the prepared query plan cache size used
--	in the logApply() trigger.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logApplySetCacheSize (p_size int4) 
returns int4
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_logApplySetCacheSize'
	language C;

-- ----------------------------------------------------------------------
-- FUNCTION logApplySaveStats ()
--
--	A function used by the remote worker to update sl_apply_stats after
--	performing a SYNC.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logApplySaveStats (p_cluster name, p_origin int4, p_duration interval) 
returns int4
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_logApplySaveStats'
	language C;


create or replace function @NAMESPACE@.checkmoduleversion () returns text as $$
declare
  moduleversion	text;
begin
  select into moduleversion @NAMESPACE@.getModuleVersion();
  if moduleversion <> '@MODULEVERSION@' then
      raise exception 'Slonik version: @MODULEVERSION@ != Slony-I version in PG build %',
             moduleversion;
  end if;
  return null;
end;$$ language plpgsql;

comment on function @NAMESPACE@.checkmoduleversion () is 
'Inline test function that verifies that slonik request for STORE
NODE/INIT CLUSTER is being run against a conformant set of
schema/functions.';

select @NAMESPACE@.checkmoduleversion();

create or replace function @NAMESPACE@.decode_tgargs(bytea) returns text[] as 
'$libdir/slony1_funcs.@MODULEVERSION@','_Slony_I_@FUNCVERSION@_slon_decode_tgargs' language C security definer;

comment on function @NAMESPACE@.decode_tgargs(bytea) is 
'Translates the contents of pg_trigger.tgargs to an array of text arguments';

grant execute on function @NAMESPACE@.decode_tgargs(bytea) to public;

-----------------------------------------------------------------------
-- This function checks to see if the namespace name is valid.  
--
-- Apparently pgAdminIII does different validation than Slonik, and so
-- users that set up cluster names using pgAdminIII can get in trouble in
-- that they do not get around to needing Slonik until it is too
-- late...
-----------------------------------------------------------------------

create or replace function @NAMESPACE@.check_namespace_validity () returns boolean as $$
declare
	c_cluster text;
begin
	c_cluster := '@CLUSTERNAME@';
	if c_cluster !~ E'^[[:alpha:]_][[:alnum:]_\$]{0,62}$' then
		raise exception 'Cluster name % is not a valid SQL symbol!', c_cluster;
	else
		raise notice 'checked validity of cluster % namespace - OK!', c_cluster;
	end if;
	return 't';
end
$$ language plpgsql;

select @NAMESPACE@.check_namespace_validity();
drop function @NAMESPACE@.check_namespace_validity();

-- ----------------------------------------------------------------------
-- FUNCTION logTrigger ()
--
--	
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logTrigger () returns trigger
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_logTrigger'
	language C
	security definer;

comment on function @NAMESPACE@.logTrigger () is 
  'This is the trigger that is executed on the origin node that causes
updates to be recorded in sl_log_1/sl_log_2.';

grant execute on function @NAMESPACE@.logTrigger () to public;

-- ----------------------------------------------------------------------
-- FUNCTION terminateNodeConnections (failed_node)
--
--	
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.terminateNodeConnections (p_failed_node int4) returns int4
as $$
declare
	v_row			record;
begin
	for v_row in select nl_nodeid, nl_conncnt,
			nl_backendpid from @NAMESPACE@.sl_nodelock
			where nl_nodeid = p_failed_node for update
	loop
		perform @NAMESPACE@.killBackend(v_row.nl_backendpid, 'TERM');
		delete from @NAMESPACE@.sl_nodelock
			where nl_nodeid = v_row.nl_nodeid
			and nl_conncnt = v_row.nl_conncnt;
	end loop;

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.terminateNodeConnections (p_failed_node int4) is 
  'terminates all backends that have registered to be from the given node';

-- ----------------------------------------------------------------------
-- FUNCTION killBackend (pid, signame)
--
--	
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.killBackend (p_pid int4, p_signame text) returns int4
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_killBackend'
	language C;

comment on function @NAMESPACE@.killBackend(p_pid int4, p_signame text) is
  'Send a signal to a postgres process. Requires superuser rights';

-- ----------------------------------------------------------------------
-- FUNCTION seqtrack (seqid, seqval)
--
--	
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.seqtrack (p_seqid int4, p_seqval int8) returns int8
    as '$libdir/slony1_funcs.@MODULEVERSION@', '_Slony_I_@FUNCVERSION@_seqtrack'
	strict language C;

comment on function @NAMESPACE@.seqtrack(p_seqid int4, p_seqval int8) is
  'Returns NULL if seqval has not changed since the last call for seqid';

-- ----------------------------------------------------------------------
-- FUNCTION slon_quote_brute(text)
--
--	Function that quotes a given string.
--	All existing quotes will be escaped.
--
--	This function will be used to quote output of internal functions.
-- ----------------------------------------------------------------------

create or replace function @NAMESPACE@.slon_quote_brute(p_tab_fqname text) returns text
as $$
declare	
    v_fqname text default '';
begin
    v_fqname := '"' || replace(p_tab_fqname,'"','""') || '"';
    return v_fqname;
end;
$$ language plpgsql immutable;

comment on function @NAMESPACE@.slon_quote_brute(p_tab_fqname text) is
  'Brutally quote the given text';

-- ----------------------------------------------------------------------
-- FUNCTION slon_quote_input(text)
--
--	Function that quotes a given fqn. This function quotes every
--	word that isn't quoted yet. Words or groups of words that are
--	already quoted will be untouched.
--
--	This function will be used to quote user input.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.slon_quote_input(p_tab_fqname text) returns text as $$
  declare
     v_nsp_name text;
     v_tab_name text;
	 v_i integer;
	 v_l integer;
     v_pq2 integer;
begin
	v_l := length(p_tab_fqname);

	-- Let us search for the dot
	if p_tab_fqname like '"%' then
		-- if the first part of the ident starts with a double quote, search
		-- for the closing double quote, skipping over double double quotes.
		v_i := 2;
		while v_i <= v_l loop
			if substr(p_tab_fqname, v_i, 1) != '"' then
				v_i := v_i + 1;
			else
				v_i := v_i + 1;
				if substr(p_tab_fqname, v_i, 1) != '"' then
					exit;
				end if;
				v_i := v_i + 1;
			end if;
		end loop;
	else
		-- first part of ident is not quoted, search for the dot directly
		v_i := 1;
		while v_i <= v_l loop
			if substr(p_tab_fqname, v_i, 1) = '.' then
				exit;
			end if;
			v_i := v_i + 1;
		end loop;
	end if;

	-- v_i now points at the dot or behind the string.

	if substr(p_tab_fqname, v_i, 1) = '.' then
		-- There is a dot now, so split the ident into its namespace
		-- and objname parts and make sure each is quoted
		v_nsp_name := substr(p_tab_fqname, 1, v_i - 1);
		v_tab_name := substr(p_tab_fqname, v_i + 1);
		if v_nsp_name not like '"%' then
			v_nsp_name := '"' || replace(v_nsp_name, '"', '""') ||
						  '"';
		end if;
		if v_tab_name not like '"%' then
			v_tab_name := '"' || replace(v_tab_name, '"', '""') ||
						  '"';
		end if;

		return v_nsp_name || '.' || v_tab_name;
	else
		-- No dot ... must be just an ident without schema
		if p_tab_fqname like '"%' then
			return p_tab_fqname;
		else
			return '"' || replace(p_tab_fqname, '"', '""') || '"';
		end if;
	end if;

end;$$ language plpgsql immutable;

comment on function @NAMESPACE@.slon_quote_input(p_text text) is
  'quote all words that aren''t quoted yet';

-- **********************************************************************
-- * PL/pgSQL functions for administrative tasks
-- **********************************************************************


-- ----------------------------------------------------------------------
-- FUNCTION slonyVersionMajor()
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.slonyVersionMajor()
returns int4
as $$
begin
	return 2;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.slonyVersionMajor () is 
  'Returns the major version number of the slony schema';

-- ----------------------------------------------------------------------
-- FUNCTION slonyVersionMinor()
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.slonyVersionMinor()
returns int4
as $$
begin
	return 2;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.slonyVersionMinor () is 
  'Returns the minor version number of the slony schema';


-- ----------------------------------------------------------------------
-- FUNCTION slonyVersionPatchlevel()
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.slonyVersionPatchlevel()
returns int4
as $$
begin
	return 4;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.slonyVersionPatchlevel () is 
  'Returns the version patch level of the slony schema';


-- ----------------------------------------------------------------------
-- FUNCTION slonyVersion()
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.slonyVersion()
returns text
as $$
begin
	return @NAMESPACE@.slonyVersionMajor()::text || '.' || 
	       @NAMESPACE@.slonyVersionMinor()::text || '.' || 
	       @NAMESPACE@.slonyVersionPatchlevel()::text   ;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.slonyVersion() is 
  'Returns the version number of the slony schema';


-- ----------------------------------------------------------------------
-- FUNCTION registry_set_int4(key, value);
-- FUNCTION registry_get_int4(key, default);
-- FUNCTION registry_set_text(key, value);
-- FUNCTION registry_get_text(key, default);
-- FUNCTION registry_set_timestamp(key, value);
-- FUNCTION registry_get_timestamp(key, default);
--
--	Functions for accessing sl_registry
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.registry_set_int4(p_key text, p_value int4)
returns int4 as $$
BEGIN
	if p_value is null then
		delete from @NAMESPACE@.sl_registry
				where reg_key = p_key;
	else
		lock table @NAMESPACE@.sl_registry;
		update @NAMESPACE@.sl_registry
				set reg_int4 = p_value
				where reg_key = p_key;
		if not found then
			insert into @NAMESPACE@.sl_registry (reg_key, reg_int4)
					values (p_key, p_value);
		end if;
	end if;
	return p_value;
END;
$$ language plpgsql;
comment on function @NAMESPACE@.registry_set_int4(p_key text, p_value int4) is
'registry_set_int4(key, value)

Set or delete a registry value';

create or replace function @NAMESPACE@.registry_get_int4(p_key text, p_default int4)
returns int4 as $$
DECLARE
	v_value		int4;
BEGIN
	select reg_int4 into v_value from @NAMESPACE@.sl_registry
			where reg_key = p_key;
	if not found then 
		v_value = p_default;
		if p_default notnull then
			perform @NAMESPACE@.registry_set_int4(p_key, p_default);
		end if;
	else
		if v_value is null then
			raise exception 'Slony-I: registry key % is not an int4 value',
					p_key;
		end if;
	end if;
	return v_value;
END;
$$ language plpgsql;
comment on function @NAMESPACE@.registry_get_int4(p_key text, p_default int4) is
'registry_get_int4(key, value)

Get a registry value. If not present, set and return the default.';

create or replace function @NAMESPACE@.registry_set_text(p_key text, p_value text)
returns text as $$
BEGIN
	if p_value is null then
		delete from @NAMESPACE@.sl_registry
				where reg_key = p_key;
	else
		lock table @NAMESPACE@.sl_registry;
		update @NAMESPACE@.sl_registry
				set reg_text = p_value
				where reg_key = p_key;
		if not found then
			insert into @NAMESPACE@.sl_registry (reg_key, reg_text)
					values (p_key, p_value);
		end if;
	end if;
	return p_value;
END;
$$ language plpgsql;
comment on function @NAMESPACE@.registry_set_text(text, text) is
'registry_set_text(key, value)

Set or delete a registry value';

create or replace function @NAMESPACE@.registry_get_text(p_key text, p_default text)
returns text as $$
DECLARE
	v_value		text;
BEGIN
	select reg_text into v_value from @NAMESPACE@.sl_registry
			where reg_key = p_key;
	if not found then 
		v_value = p_default;
		if p_default notnull then
			perform @NAMESPACE@.registry_set_text(p_key, p_default);
		end if;
	else
		if v_value is null then
			raise exception 'Slony-I: registry key % is not a text value',
					p_key;
		end if;
	end if;
	return v_value;
END;
$$ language plpgsql;
comment on function @NAMESPACE@.registry_get_text(p_key text, p_default text) is
'registry_get_text(key, value)

Get a registry value. If not present, set and return the default.';

create or replace function @NAMESPACE@.registry_set_timestamp(p_key text, p_value timestamptz)
returns timestamp as $$
BEGIN
	if p_value is null then
		delete from @NAMESPACE@.sl_registry
				where reg_key = p_key;
	else
		lock table @NAMESPACE@.sl_registry;
		update @NAMESPACE@.sl_registry
				set reg_timestamp = p_value
				where reg_key = p_key;
		if not found then
			insert into @NAMESPACE@.sl_registry (reg_key, reg_timestamp)
					values (p_key, p_value);
		end if;
	end if;
	return p_value;
END;
$$ language plpgsql;

comment on function @NAMESPACE@.registry_set_timestamp(p_key text, p_value timestamptz) is
'registry_set_timestamp(key, value)

Set or delete a registry value';

create or replace function @NAMESPACE@.registry_get_timestamp(p_key text, p_default timestamptz)
returns timestamp as $$
DECLARE
	v_value		timestamp;
BEGIN
	select reg_timestamp into v_value from @NAMESPACE@.sl_registry
			where reg_key = p_key;
	if not found then 
		v_value = p_default;
		if p_default notnull then
			perform @NAMESPACE@.registry_set_timestamp(p_key, p_default);
		end if;
	else
		if v_value is null then
			raise exception 'Slony-I: registry key % is not an timestamp value',
					p_key;
		end if;
	end if;
	return v_value;
END;
$$ language plpgsql;

comment on function @NAMESPACE@.registry_get_timestamp(p_key text, p_default timestamptz) is
'registry_get_timestamp(key, value)

Get a registry value. If not present, set and return the default.';


-- ----------------------------------------------------------------------
-- FUNCTION cleanupNodelock ()
--
--	Remove old entries from the nodelock table
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.cleanupNodelock ()
returns int4
as $$
declare
	v_row		record;
begin
	for v_row in select nl_nodeid, nl_conncnt, nl_backendpid
			from @NAMESPACE@.sl_nodelock
			for update
	loop
		if @NAMESPACE@.killBackend(v_row.nl_backendpid, 'NULL') < 0 then
			raise notice 'Slony-I: cleanup stale sl_nodelock entry for pid=%',
					v_row.nl_backendpid;
			delete from @NAMESPACE@.sl_nodelock where
					nl_nodeid = v_row.nl_nodeid and
					nl_conncnt = v_row.nl_conncnt;
		end if;
	end loop;

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.cleanupNodelock() is
'Clean up stale entries when restarting slon';

-- ----------------------------------------------------------------------
-- FUNCTION registerNodeConnection (nodeid)
--
-- register a node connection for a slon so that only that slon services
-- the node
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.registerNodeConnection (p_nodeid int4)
returns int4
as $$
begin
	insert into @NAMESPACE@.sl_nodelock
		(nl_nodeid, nl_backendpid)
		values
		(p_nodeid, pg_backend_pid());

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.registerNodeConnection (p_nodeid int4) is
'Register (uniquely) the node connection so that only one slon can service the node';


-- ----------------------------------------------------------------------
-- FUNCTION initializeLocalNode (no_id, no_comment)
--
--	Initializes a new node.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.initializeLocalNode (p_local_node_id int4, p_comment text)
returns int4
as $$
declare
	v_old_node_id		int4;
	v_first_log_no		int4;
	v_event_seq			int8;
begin
	-- ----
	-- Make sure this node is uninitialized or got reset
	-- ----
	select last_value::int4 into v_old_node_id from @NAMESPACE@.sl_local_node_id;
	if v_old_node_id != -1 then
		raise exception 'Slony-I: This node is already initialized';
	end if;

	-- ----
	-- Set sl_local_node_id to the requested value and add our
	-- own system to sl_node.
	-- ----
	perform setval('@NAMESPACE@.sl_local_node_id', p_local_node_id);
	perform @NAMESPACE@.storeNode_int (p_local_node_id, p_comment);

	if (pg_catalog.current_setting('max_identifier_length')::integer - pg_catalog.length('@NAMESPACE@')) < 5 then
		raise notice 'Slony-I: Cluster name length [%] versus system max_identifier_length [%] ', pg_catalog.length('@NAMESPACE@'), pg_catalog.current_setting('max_identifier_length');
		raise notice 'leaves narrow/no room for some Slony-I-generated objects (such as indexes).';
		raise notice 'You may run into problems later!';
	end if;
	
	--
	-- Put the apply trigger onto sl_log_1 and sl_log_2
	--
	create trigger apply_trigger
		before INSERT on @NAMESPACE@.sl_log_1
		for each row execute procedure @NAMESPACE@.logApply('_@CLUSTERNAME@');
	alter table @NAMESPACE@.sl_log_1
	  enable replica trigger apply_trigger;
	create trigger apply_trigger
		before INSERT on @NAMESPACE@.sl_log_2
		for each row execute procedure @NAMESPACE@.logApply('_@CLUSTERNAME@');
	alter table @NAMESPACE@.sl_log_2
			enable replica trigger apply_trigger;

	return p_local_node_id;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.initializeLocalNode (p_local_node_id int4, p_comment text) is 
  'no_id - Node ID #
no_comment - Human-oriented comment

Initializes the new node, no_id';

-- ----------------------------------------------------------------------
-- FUNCTION storeNode (no_id, no_comment)
--
--	Generate the STORE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeNode (p_no_id int4, p_no_comment text)
returns bigint
as $$
begin
	perform @NAMESPACE@.storeNode_int (p_no_id, p_no_comment);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'STORE_NODE',
									p_no_id::text, p_no_comment::text);
end;
$$ language plpgsql
	called on null input;

comment on function @NAMESPACE@.storeNode(p_no_id int4, p_no_comment text) is
'no_id - Node ID #
no_comment - Human-oriented comment

Generate the STORE_NODE event for node no_id';

-- ----------------------------------------------------------------------
-- FUNCTION storeNode_int (no_id, no_comment)
--
--	Process the STORE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeNode_int (p_no_id int4, p_no_comment text)
returns int4
as $$
declare
	v_old_row		record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check if the node exists
	-- ----
	select * into v_old_row
			from @NAMESPACE@.sl_node
			where no_id = p_no_id
			for update;
	if found then 
		-- ----
		-- Node exists, update the existing row.
		-- ----
		update @NAMESPACE@.sl_node
				set no_comment = p_no_comment
				where no_id = p_no_id;
	else
		-- ----
		-- New node, insert the sl_node row
		-- ----
		insert into @NAMESPACE@.sl_node
				(no_id, no_active, no_comment,no_failed) values
				(p_no_id, 'f', p_no_comment,false);
	end if;

	return p_no_id;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.storeNode_int(p_no_id int4, p_no_comment text) is
'no_id - Node ID #
no_comment - Human-oriented comment

Internal function to process the STORE_NODE event for node no_id';


-- ----------------------------------------------------------------------
-- FUNCTION enableNode (no_id)
--
--	Generate the ENABLE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.enableNode (p_no_id int4)
returns bigint
as $$
declare
	v_local_node_id	int4;
	v_node_row		record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that we are the node to activate and that we are
	-- currently disabled.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select * into v_node_row
			from @NAMESPACE@.sl_node
			where no_id = p_no_id
			for update;
	if not found then 
		raise exception 'Slony-I: node % not found', p_no_id;
	end if;
	if v_node_row.no_active then
		raise exception 'Slony-I: node % is already active', p_no_id;
	end if;

	-- ----
	-- Activate this node and generate the ENABLE_NODE event
	-- ----
	perform @NAMESPACE@.enableNode_int (p_no_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'ENABLE_NODE',
									p_no_id::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.enableNode(p_no_id int4) is
'no_id - Node ID #

Generate the ENABLE_NODE event for node no_id';

-- ----------------------------------------------------------------------
-- FUNCTION enableNode_int (no_id)
--
--	Process the ENABLE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.enableNode_int (p_no_id int4)
returns int4
as $$
declare
	v_local_node_id	int4;
	v_node_row		record;
	v_sub_row		record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that the node is inactive
	-- ----
	select * into v_node_row
			from @NAMESPACE@.sl_node
			where no_id = p_no_id
			for update;
	if not found then 
		raise exception 'Slony-I: node % not found', p_no_id;
	end if;
	if v_node_row.no_active then
		return p_no_id;
	end if;

	-- ----
	-- Activate the node and generate sl_confirm status rows for it.
	-- ----
	update @NAMESPACE@.sl_node
			set no_active = 't'
			where no_id = p_no_id;
	insert into @NAMESPACE@.sl_confirm
			(con_origin, con_received, con_seqno)
			select no_id, p_no_id, 0 from @NAMESPACE@.sl_node
				where no_id != p_no_id
				and no_active;
	insert into @NAMESPACE@.sl_confirm
			(con_origin, con_received, con_seqno)
			select p_no_id, no_id, 0 from @NAMESPACE@.sl_node
				where no_id != p_no_id
				and no_active;

	-- ----
	-- Generate ENABLE_SUBSCRIPTION events for all sets that
	-- origin here and are subscribed by the just enabled node.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	for v_sub_row in select SUB.sub_set, SUB.sub_provider from
			@NAMESPACE@.sl_set S,
			@NAMESPACE@.sl_subscribe SUB
			where S.set_origin = v_local_node_id
			and S.set_id = SUB.sub_set
			and SUB.sub_receiver = p_no_id
			for update of S
	loop
		perform @NAMESPACE@.enableSubscription (v_sub_row.sub_set,
				v_sub_row.sub_provider, p_no_id);
	end loop;

	return p_no_id;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.enableNode_int(p_no_id int4) is
'no_id - Node ID #

Internal function to process the ENABLE_NODE event for node no_id';

-- ----------------------------------------------------------------------
-- FUNCTION disableNode (no_id)
--
--	Generate the DISABLE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.disableNode (p_no_id int4)
returns bigint
as $$
begin
	-- **** TODO ****
	raise exception 'Slony-I: disableNode() not implemented';
end;
$$ language plpgsql;
comment on function @NAMESPACE@.disableNode(p_no_id int4) is
'generate DISABLE_NODE event for node no_id';

-- ----------------------------------------------------------------------
-- FUNCTION disableNode_int (no_id)
--
--	Process the DISABLE_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.disableNode_int (p_no_id int4)
returns int4
as $$
begin
	-- **** TODO ****
	raise exception 'Slony-I: disableNode_int() not implemented';
end;
$$ language plpgsql;

comment on function @NAMESPACE@.disableNode(p_no_id int4) is
'process DISABLE_NODE event for node no_id

NOTE: This is not yet implemented!';

-- ----------------------------------------------------------------------
-- FUNCTION dropNode (no_id)
--
--	Generate the DROP_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropNode (p_no_ids int4[])
returns bigint
as $$
declare
	v_node_row		record;
	v_idx         integer;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that this got called on a different node
	-- ----
	if  @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') = ANY (p_no_ids) then
		raise exception 'Slony-I: DROP_NODE cannot initiate on the dropped node';
	end if;

	--
	-- if any of the deleted nodes are receivers we drop the sl_subscribe line
	--
	delete from @NAMESPACE@.sl_subscribe where sub_receiver = ANY (p_no_ids);

	v_idx:=1;
	LOOP
	  EXIT WHEN v_idx>array_upper(p_no_ids,1) ;
	  select * into v_node_row from @NAMESPACE@.sl_node
			where no_id = p_no_ids[v_idx]
			for update;
	  if not found then
		 raise exception 'Slony-I: unknown node ID % %', p_no_ids[v_idx],v_idx;
	  end if;
	  -- ----
	  -- Make sure we do not break other nodes subscriptions with this
	  -- ----
	  if exists (select true from @NAMESPACE@.sl_subscribe
			where sub_provider = p_no_ids[v_idx])
	  then
		raise exception 'Slony-I: Node % is still configured as a data provider',
				p_no_ids[v_idx];
	  end if;

	  -- ----
	  -- Make sure no set originates there any more
	  -- ----
	  if exists (select true from @NAMESPACE@.sl_set
			where set_origin = p_no_ids[v_idx])
	  then
	  	  raise exception 'Slony-I: Node % is still origin of one or more sets',
				p_no_ids[v_idx];
	  end if;

	  -- ----
	  -- Call the internal drop functionality and generate the event
	  -- ----
	  perform @NAMESPACE@.dropNode_int(p_no_ids[v_idx]);
	  v_idx:=v_idx+1;
	END LOOP;
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'DROP_NODE',
									array_to_string(p_no_ids,','));
end;
$$ language plpgsql;
comment on function @NAMESPACE@.dropNode(p_no_ids int4[]) is
'generate DROP_NODE event to drop node node_id from replication';

-- ----------------------------------------------------------------------
-- FUNCTION dropNode_int (no_id)
--
--	Process the DROP_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropNode_int (p_no_id int4)
returns int4
as $$
declare
	v_tab_row		record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- If the dropped node is a remote node, clean the configuration
	-- from all traces for it.
	-- ----
	if p_no_id <> @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		delete from @NAMESPACE@.sl_subscribe
				where sub_receiver = p_no_id;
		delete from @NAMESPACE@.sl_listen
				where li_origin = p_no_id
					or li_provider = p_no_id
					or li_receiver = p_no_id;
		delete from @NAMESPACE@.sl_path
				where pa_server = p_no_id
					or pa_client = p_no_id;
		delete from @NAMESPACE@.sl_confirm
				where con_origin = p_no_id
					or con_received = p_no_id;
		delete from @NAMESPACE@.sl_event
				where ev_origin = p_no_id;
		delete from @NAMESPACE@.sl_node
				where no_id = p_no_id;

		return p_no_id;
	end if;

	-- ----
	-- This is us ... deactivate the node for now, the daemon
	-- will call uninstallNode() in a separate transaction.
	-- ----
	update @NAMESPACE@.sl_node
			set no_active = false
			where no_id = p_no_id;

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	return p_no_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.dropNode_int(p_no_id int4) is
'internal function to process DROP_NODE event to drop node node_id from replication';


-- ----------------------------------------------------------------------
-- FUNCTION preFailover (failed_node)
--
-- Called on all nodes before the failover.
-- Failover candidates are direct subscribers of the failed node.
-- This function ensures that nodes identified as failover candidates
-- meet the criteria for such a node.
-- For all nodes this function will blank the paths to the failed node and
-- restarts slon.
-- This ensures that slonik will have a stable state to determine 
-- which node is the most-ahead.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.preFailover(p_failed_node int4,p_is_candidate boolean)
returns int4
as $$
declare
	v_row				record;
	v_row2				record;
	v_n					int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- All consistency checks first

	if p_is_candidate then
	   -- ----
	   -- Check all sets originating on the failed node
	   -- ----
	   for v_row in select set_id
			from @NAMESPACE@.sl_set
			where set_origin = p_failed_node
		loop
				-- ----
				-- Check that the backup node is subscribed to all sets
				-- that originate on the failed node
				-- ----
				select into v_row2 sub_forward, sub_active
					   from @NAMESPACE@.sl_subscribe
					   where sub_set = v_row.set_id
					and sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
				if not found then
				   raise exception 'Slony-I: cannot failover - node % is not subscribed to set %',
					@NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'), v_row.set_id;
				end if;

				-- ----
				-- Check that the subscription is active
				-- ----
				if not v_row2.sub_active then
				   raise exception 'Slony-I: cannot failover - subscription for set % is not active',
					v_row.set_id;
				end if;

				-- ----
				-- If there are other subscribers, the backup node needs to
				-- be a forwarder too.
				-- ----
				select into v_n count(*)
					   from @NAMESPACE@.sl_subscribe
					   where sub_set = v_row.set_id
					   and sub_receiver <> @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
				if v_n > 0 and not v_row2.sub_forward then
				raise exception 'Slony-I: cannot failover - node % is not a forwarder of set %',
					 @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'), v_row.set_id;
				end if;
			end loop;
	end if;

	-- ----
	-- Terminate all connections of the failed node the hard way
	-- ----
	perform @NAMESPACE@.terminateNodeConnections(p_failed_node);

	update @NAMESPACE@.sl_path set pa_conninfo='<event pending>' WHERE
	   		  pa_server=p_failed_node;	
	notify "_@CLUSTERNAME@_Restart";
	-- ----
	-- That is it - so far.
	-- ----
	return p_failed_node;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.preFailover(p_failed_node int4,is_failover_candidate boolean) is
'Prepare for a failover.  This function is called on all candidate nodes.
It blanks the paths to the failed node
and then restart of all node daemons.';

-- ----------------------------------------------------------------------
-- FUNCTION failedNode (failed_node, backup_node)
--
--	Initiate a failover. This function must be called on all nodes
--	and then waited for the restart of all node daemons.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.failedNode(p_failed_node int4, p_backup_node int4,p_failed_nodes integer[])
returns int4
as $$
declare
	v_row				record;
	v_row2				record;
	v_failed					boolean;
    v_restart_required          boolean;
begin
	
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	v_restart_required:=false;
	--
	-- any nodes other than the backup receiving
	-- ANY subscription from a failed node
	-- will now get that data from the backup node.
	update @NAMESPACE@.sl_subscribe set 
		   sub_provider=p_backup_node
		   where sub_provider=p_failed_node
		   and sub_receiver<>p_backup_node
		   and sub_receiver <> ALL (p_failed_nodes);
	if found then
	   v_restart_required:=true;
	end if;
	-- 
	-- if this node is receiving a subscription from the backup node
	-- with a failed node as the provider we need to fix this.
	update @NAMESPACE@.sl_subscribe set 
	        sub_provider=p_backup_node
		from @NAMESPACE@.sl_set
		where set_id = sub_set
		and set_origin=p_failed_node
		and sub_provider = ANY(p_failed_nodes)
		and sub_receiver=@NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	-- ----
	-- Terminate all connections of the failed node the hard way
	-- ----
	perform @NAMESPACE@.terminateNodeConnections(p_failed_node);

	-- Clear out the paths for the failed node.
	-- This ensures that *this* node won't be pulling data from
	-- the failed node even if it *does* become accessible

	update @NAMESPACE@.sl_path set pa_conninfo='<event pending>' WHERE
	   		  pa_server=p_failed_node
			  and pa_conninfo<>'<event pending>';

	if found then
	   v_restart_required:=true;
	end if;

	v_failed := exists (select 1 from @NAMESPACE@.sl_node 
		   where no_failed=true and no_id=p_failed_node);

    if not v_failed then
	   	
		update @NAMESPACE@.sl_node set no_failed=true where no_id = ANY (p_failed_nodes)
			   and no_failed=false;
		if found then
	   	   v_restart_required:=true;
		end if;
	end if;	

	if v_restart_required then
	  -- Rewrite sl_listen table
	  perform @NAMESPACE@.RebuildListenEntries();	   
	
	  -- ----
	  -- Make sure the node daemon will restart
 	  -- ----
	  notify "_@CLUSTERNAME@_Restart";
    end if;


	-- ----
	-- That is it - so far.
	-- ----
	return p_failed_node;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.failedNode(p_failed_node int4, p_backup_node int4,p_failed_nodes integer[]) is
'Initiate failover from failed_node to backup_node.  This function must be called on all nodes, 
and then waited for the restart of all node daemons.';



-- ----------------------------------------------------------------------
-- FUNCTION failedNode2 (failed_node, backup_node, set_id, ev_seqno, ev_seqfake)
--
--	On the node that has the highest sequence number of the failed node,
--	fake the FAILED_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.failedNode2 (p_failed_node int4, p_backup_node int4, p_ev_seqno int8, p_failed_nodes integer[])
returns bigint
as $$
declare
	v_row				record;
	v_new_event			bigint;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	select * into v_row
			from @NAMESPACE@.sl_event
			where ev_origin = p_failed_node
			and ev_seqno = p_ev_seqno;
	if not found then
		raise exception 'Slony-I: event %,% not found',
				p_failed_node, p_ev_seqno;
	end if;

	update @NAMESPACE@.sl_node set no_failed=true  where no_id = ANY 
	(p_failed_nodes) and no_failed=false;
	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();
	-- ----
	-- Make sure the node daemon will restart
	-- ----
	raise notice 'calling restart node %',p_failed_node;

	notify "_@CLUSTERNAME@_Restart";

	select @NAMESPACE@.createEvent('_@CLUSTERNAME@','FAILOVER_NODE',
								p_failed_node::text,p_ev_seqno::text,
								array_to_string(p_failed_nodes,','))
			into v_new_event;
		

	return v_new_event;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.failedNode2 (p_failed_node int4, p_backup_node int4, p_ev_seqno int8,p_failed_nodes integer[] ) is
'FUNCTION failedNode2 (failed_node, backup_node, set_id, ev_seqno, ev_seqfake,p_failed_nodes)

On the node that has the highest sequence number of the failed node,
fake the FAILOVER_SET event.';

create or replace function @NAMESPACE@.failedNode3 (p_failed_node int4, p_backup_node int4,p_seq_no bigint)
returns int4
as $$
declare

begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	perform @NAMESPACE@.failoverSet_int(p_failed_node,
		p_backup_node,p_seq_no);

	notify "_@CLUSTERNAME@_Restart";
    return 0;
end;
$$ language plpgsql;

-- ----------------------------------------------------------------------
-- FUNCTION failoverSet_int (failed_node, backup_node, set_id, wait_seqno)
--
--	Finish failover for one set.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.failoverSet_int (p_failed_node int4, p_backup_node int4,p_last_seqno bigint)
returns int4
as $$
declare
	v_row				record;
	v_last_sync			int8;
	v_set				int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	SELECT max(ev_seqno) into v_last_sync FROM @NAMESPACE@.sl_event where
		   ev_origin=p_failed_node;
	if v_last_sync > p_last_seqno then
	   -- this node is ahead of the last sequence number from the
	   -- failed node that the backup node has.
	   -- this node must unsubscribe from all sets from the origin.
	   for v_set in select set_id from @NAMESPACE@.sl_set where 
		set_origin=p_failed_node
		loop
			raise warning 'Slony is dropping the subscription of set % found sync %s bigger than %s '
			, v_set, v_last_sync::text, p_last_seqno::text;
			perform @NAMESPACE@.unsubscribeSet(v_set,
				   @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'),
				   true);
		end loop;
		delete from @NAMESPACE@.sl_event where ev_origin=p_failed_node
			   and ev_seqno > p_last_seqno;
	end if;
	-- ----
	-- Change the origin of the set now to the backup node.
	-- On the backup node this includes changing all the
	-- trigger and protection stuff
	for v_set in select set_id from @NAMESPACE@.sl_set where 
		set_origin=p_failed_node
	loop
	-- ----
	   if p_backup_node = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
	   	  	delete from @NAMESPACE@.sl_setsync
				where ssy_setid = v_set;
			delete from @NAMESPACE@.sl_subscribe
				where sub_set = v_set
					and sub_receiver = p_backup_node;
			update @NAMESPACE@.sl_set
				set set_origin = p_backup_node
				where set_id = v_set;		
			 update @NAMESPACE@.sl_subscribe
						set sub_provider=p_backup_node
					  	FROM @NAMESPACE@.sl_node receive_node
					   where sub_set = v_set
					   and sub_provider=p_failed_node
					   and sub_receiver=receive_node.no_id
					   and receive_node.no_failed=false;			

			for v_row in select * from @NAMESPACE@.sl_table
				where tab_set = v_set
				order by tab_id
			loop
				perform @NAMESPACE@.alterTableConfigureTriggers(v_row.tab_id);
			end loop;
	else
		raise notice 'deleting from sl_subscribe all rows with receiver %',
		p_backup_node;
		
			delete from @NAMESPACE@.sl_subscribe
					  where sub_set = v_set
					  and sub_receiver = p_backup_node;
		
			update @NAMESPACE@.sl_subscribe
				 		set sub_provider=p_backup_node
						FROM @NAMESPACE@.sl_node receive_node
					   where sub_set = v_set
					    and sub_provider=p_failed_node
						 and sub_provider=p_failed_node
					   and sub_receiver=receive_node.no_id
					   and receive_node.no_failed=false;
			update @NAMESPACE@.sl_set
					   set set_origin = p_backup_node
				where set_id = v_set;
			-- ----
			-- If we are a subscriber of the set ourself, change our
			-- setsync status to reflect the new set origin.
			-- ----
			if exists (select true from @NAMESPACE@.sl_subscribe
			   where sub_set = v_set
			   	and sub_receiver = @NAMESPACE@.getLocalNodeId(
						'_@CLUSTERNAME@'))
			then
				delete from @NAMESPACE@.sl_setsync
					   where ssy_setid = v_set;

				select coalesce(max(ev_seqno), 0) into v_last_sync
					   from @NAMESPACE@.sl_event
					   where ev_origin = p_backup_node
					   and ev_type = 'SYNC';
				if v_last_sync > 0 then
				   insert into @NAMESPACE@.sl_setsync
					(ssy_setid, ssy_origin, ssy_seqno,
					ssy_snapshot, ssy_action_list)
					select v_set, p_backup_node, v_last_sync,
					ev_snapshot, NULL
					from @NAMESPACE@.sl_event
					where ev_origin = p_backup_node
						and ev_seqno = v_last_sync;
				else
					insert into @NAMESPACE@.sl_setsync
					(ssy_setid, ssy_origin, ssy_seqno,
					ssy_snapshot, ssy_action_list)
					values (v_set, p_backup_node, '0',
					'1:1:', NULL);
				end if;	
			end if;
		end if;
	end loop;
	
	--If there are any subscriptions with 
	--the failed_node being the provider then
	--we want to redirect those subscriptions
	--to come from the backup node.
	--
	-- The backup node should be a valid
	-- provider for all subscriptions served
	-- by the failed node. (otherwise it
	-- wouldn't be a allowable backup node).
	update @NAMESPACE@.sl_subscribe	       
	       set sub_provider=p_backup_node
	       from @NAMESPACE@.sl_node
	       where sub_provider=p_failed_node
	       and sl_node.no_id=sub_receiver
	       and sl_node.no_failed=false;	

	update @NAMESPACE@.sl_node
		   set no_active=false WHERE 
		   no_id=p_failed_node;

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();


	return p_failed_node;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.failoverSet_int (p_failed_node int4, p_backup_node int4,p_seqno bigint) is
'FUNCTION failoverSet_int (failed_node, backup_node, set_id, wait_seqno)

Finish failover for one set.';

-- ----------------------------------------------------------------------
-- FUNCTION uninstallNode ()
--
--	Reset the whole database to standalone by removing the whole
--	replication system.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.uninstallNode ()
returns int4
as $$
declare
	v_tab_row		record;
begin
	raise notice 'Slony-I: Please drop schema "_@CLUSTERNAME@"';
	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.uninstallNode() is
'Reset the whole database to standalone by removing the whole
replication system.';

--
-- The return type of cloneNodePrepare changed at one point.
-- drop it to make the script upgrade safe.
--
DROP FUNCTION IF EXISTS @NAMESPACE@.cloneNodePrepare(int4,int4,text);
-- ----------------------------------------------------------------------
-- FUNCTION cloneNodePrepare ()
--
--	Duplicate a nodes configuration under a different no_id in
--	preparation for the node to be copied with standard DB tools.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.cloneNodePrepare (p_no_id int4, p_no_provider int4, p_no_comment text)		
returns bigint
as $$
begin
	perform @NAMESPACE@.cloneNodePrepare_int (p_no_id, p_no_provider, p_no_comment);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'CLONE_NODE',
									p_no_id::text, p_no_provider::text,
									p_no_comment::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.cloneNodePrepare(p_no_id int4, p_no_provider int4, p_no_comment text) is
'Prepare for cloning a node.';

-- ----------------------------------------------------------------------
-- FUNCTION cloneNodePrepare_int ()
--
--	Internal part of cloneNodePrepare()
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.cloneNodePrepare_int (p_no_id int4, p_no_provider int4, p_no_comment text)
returns int4
as $$
declare
   v_dummy int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	update @NAMESPACE@.sl_node set
	       no_active = np.no_active,
	       no_comment = np.no_comment,
	       no_failed = np.no_failed
	       from @NAMESPACE@.sl_node np
	       where np.no_id = p_no_provider
	       and sl_node.no_id = p_no_id;
	if not found then
	   insert into @NAMESPACE@.sl_node
		(no_id, no_active, no_comment,no_failed)
		select p_no_id, no_active, p_no_comment, no_failed
		from @NAMESPACE@.sl_node
		where no_id = p_no_provider;
	end if;

       insert into @NAMESPACE@.sl_path
	    (pa_server, pa_client, pa_conninfo, pa_connretry)
	    select pa_server, p_no_id, '<event pending>', pa_connretry
	    from @NAMESPACE@.sl_path
	    where pa_client = p_no_provider
	    and (pa_server, p_no_id) not in (select pa_server, pa_client
	    	    from @NAMESPACE@.sl_path);

       insert into @NAMESPACE@.sl_path
	    (pa_server, pa_client, pa_conninfo, pa_connretry)
	    select p_no_id, pa_client, '<event pending>', pa_connretry
	    from @NAMESPACE@.sl_path
	    where pa_server = p_no_provider
	    and (p_no_id, pa_client) not in (select pa_server, pa_client
	    	    from @NAMESPACE@.sl_path);

	insert into @NAMESPACE@.sl_subscribe
		(sub_set, sub_provider, sub_receiver, sub_forward, sub_active)
		select sub_set, sub_provider, p_no_id, sub_forward, sub_active
		from @NAMESPACE@.sl_subscribe
		where sub_receiver = p_no_provider;

	insert into @NAMESPACE@.sl_confirm
		(con_origin, con_received, con_seqno, con_timestamp)
		select con_origin, p_no_id, con_seqno, con_timestamp
		from @NAMESPACE@.sl_confirm
		where con_received = p_no_provider;

	perform @NAMESPACE@.RebuildListenEntries();

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.cloneNodePrepare_int(p_no_id int4, p_no_provider int4, p_no_comment text) is
'Internal part of cloneNodePrepare().';

-- ----------------------------------------------------------------------
-- FUNCTION cloneNodeFinish ()
--
--	Finish the cloning process on the new node.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.cloneNodeFinish (p_no_id int4, p_no_provider int4)
returns int4
as $$
declare
	v_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	perform "pg_catalog".setval('@NAMESPACE@.sl_local_node_id', p_no_id);
	perform @NAMESPACE@.resetSession();
	for v_row in select sub_set from @NAMESPACE@.sl_subscribe
			where sub_receiver = p_no_id
	loop
		perform @NAMESPACE@.updateReloid(v_row.sub_set, p_no_id);
	end loop;

	perform @NAMESPACE@.RebuildListenEntries();

	delete from @NAMESPACE@.sl_confirm
		where con_received = p_no_id;
	insert into @NAMESPACE@.sl_confirm
		(con_origin, con_received, con_seqno, con_timestamp)
		select con_origin, p_no_id, con_seqno, con_timestamp
		from @NAMESPACE@.sl_confirm
		where con_received = p_no_provider;
	insert into @NAMESPACE@.sl_confirm
		(con_origin, con_received, con_seqno, con_timestamp)
		select p_no_provider, p_no_id, 
				(select max(ev_seqno) from @NAMESPACE@.sl_event
					where ev_origin = p_no_provider), current_timestamp;

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.cloneNodeFinish(p_no_id int4, p_no_provider int4) is
'Internal part of cloneNodePrepare().';

-- ----------------------------------------------------------------------
-- FUNCTION storePath (pa_server, pa_client, pa_conninfo, pa_connretry)
--
--	Generate the STORE_PATH event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storePath (p_pa_server int4, p_pa_client int4, p_pa_conninfo text, p_pa_connretry int4)
returns bigint
as $$
begin
	perform @NAMESPACE@.storePath_int(p_pa_server, p_pa_client,
			p_pa_conninfo, p_pa_connretry);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'STORE_PATH', 
			p_pa_server::text, p_pa_client::text, 
			p_pa_conninfo::text, p_pa_connretry::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.storePath (p_pa_server int4, p_pa_client int4, p_pa_conninfo text, p_pa_connretry int4) is
'FUNCTION storePath (pa_server, pa_client, pa_conninfo, pa_connretry)

Generate the STORE_PATH event indicating that node pa_client can
access node pa_server using DSN pa_conninfo';


-- ----------------------------------------------------------------------
-- FUNCTION storePath_int (pa_server, pa_client, pa_conninfo, pa_connretry)
--
--	Process the STORE_PATH event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storePath_int (p_pa_server int4, p_pa_client int4, p_pa_conninfo text, p_pa_connretry int4)
returns int4
as $$
declare
	v_dummy			int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check if the path already exists
	-- ----
	select 1 into v_dummy
			from @NAMESPACE@.sl_path
			where pa_server = p_pa_server
			and pa_client = p_pa_client
			for update;
	if found then
		-- ----
		-- Path exists, update pa_conninfo
		-- ----
		update @NAMESPACE@.sl_path
				set pa_conninfo = p_pa_conninfo,
					pa_connretry = p_pa_connretry
				where pa_server = p_pa_server
				and pa_client = p_pa_client;
	else
		-- ----
		-- New path
		--
		-- In case we receive STORE_PATH events before we know
		-- about the nodes involved in this, we generate those nodes
		-- as pending.
		-- ----
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_pa_server) then
			perform @NAMESPACE@.storeNode_int (p_pa_server, '<event pending>');
		end if;
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_pa_client) then
			perform @NAMESPACE@.storeNode_int (p_pa_client, '<event pending>');
		end if;
		insert into @NAMESPACE@.sl_path
				(pa_server, pa_client, pa_conninfo, pa_connretry) values
				(p_pa_server, p_pa_client, p_pa_conninfo, p_pa_connretry);
	end if;

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	return 0;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.storePath_int (p_pa_server int4, p_pa_client int4, p_pa_conninfo text, p_pa_connretry int4) is
'FUNCTION storePath (pa_server, pa_client, pa_conninfo, pa_connretry)

Process the STORE_PATH event indicating that node pa_client can
access node pa_server using DSN pa_conninfo';

-- ----------------------------------------------------------------------
-- FUNCTION dropPath (pa_server, pa_client)
--
--	Generate the DROP_PATH event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropPath (p_pa_server int4, p_pa_client int4)
returns bigint
as $$
declare
	v_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- There should be no existing subscriptions. Auto unsubscribing
	-- is considered too dangerous. 
	-- ----
	for v_row in select sub_set, sub_provider, sub_receiver
			from @NAMESPACE@.sl_subscribe
			where sub_provider = p_pa_server
			and sub_receiver = p_pa_client
	loop
		raise exception 
			'Slony-I: Path cannot be dropped, subscription of set % needs it',
			v_row.sub_set;
	end loop;

	-- ----
	-- Drop all sl_listen entries that depend on this path
	-- ----
	for v_row in select li_origin, li_provider, li_receiver
			from @NAMESPACE@.sl_listen
			where li_provider = p_pa_server
			and li_receiver = p_pa_client
	loop
		perform @NAMESPACE@.dropListen(
				v_row.li_origin, v_row.li_provider, v_row.li_receiver);
	end loop;

	-- ----
	-- Now drop the path and create the event
	-- ----
	perform @NAMESPACE@.dropPath_int(p_pa_server, p_pa_client);

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	return  @NAMESPACE@.createEvent ('_@CLUSTERNAME@', 'DROP_PATH',
			p_pa_server::text, p_pa_client::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.dropPath (p_pa_server int4, p_pa_client int4) is
  'Generate DROP_PATH event to drop path from pa_server to pa_client';

-- ----------------------------------------------------------------------
-- FUNCTION dropPath_int (pa_server, pa_client)
--
--	Process the DROP_NODE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropPath_int (p_pa_server int4, p_pa_client int4)
returns int4
as $$
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Remove any dangling sl_listen entries with the server
	-- as provider and the client as receiver. This must have
	-- been cleared out before, but obviously was not.
	-- ----
	delete from @NAMESPACE@.sl_listen
			where li_provider = p_pa_server
			and li_receiver = p_pa_client;

	delete from @NAMESPACE@.sl_path
			where pa_server = p_pa_server
			and pa_client = p_pa_client;

	if found then
		-- Rewrite sl_listen table
		perform @NAMESPACE@.RebuildListenEntries();

		return 1;
	else
		-- Rewrite sl_listen table
		perform @NAMESPACE@.RebuildListenEntries();

		return 0;
	end if;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.dropPath_int (p_pa_server int4, p_pa_client int4) is
'Process DROP_PATH event to drop path from pa_server to pa_client';

-- ----------------------------------------------------------------------
-- FUNCTION storeListen (origin, provider, receiver)
--
--	Generate the STORE_LISTEN event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeListen (p_origin int4, p_provider int4, p_receiver int4)
returns bigint
as $$
begin
	perform @NAMESPACE@.storeListen_int (p_origin, p_provider, p_receiver);
	return  @NAMESPACE@.createEvent ('_@CLUSTERNAME@', 'STORE_LISTEN',
			p_origin::text, p_provider::text, p_receiver::text);
end;
$$ language plpgsql
	called on null input;

comment on function @NAMESPACE@.storeListen(p_origin int4, p_provider int4, p_receiver int4) is
'FUNCTION storeListen (li_origin, li_provider, li_receiver)

generate STORE_LISTEN event, indicating that receiver node li_receiver
listens to node li_provider in order to get messages coming from node
li_origin.';

-- ----------------------------------------------------------------------
-- FUNCTION storeListen_int (li_origin, li_provider, li_receiver)
--
--	Process the STORE_LISTEN event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeListen_int (p_li_origin int4, p_li_provider int4, p_li_receiver int4)
returns int4
as $$
declare
	v_exists		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	select 1 into v_exists
			from @NAMESPACE@.sl_listen
			where li_origin = p_li_origin
			and li_provider = p_li_provider
			and li_receiver = p_li_receiver;
	if not found then
		-- ----
		-- In case we receive STORE_LISTEN events before we know
		-- about the nodes involved in this, we generate those nodes
		-- as pending.
		-- ----
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_li_origin) then
			perform @NAMESPACE@.storeNode_int (p_li_origin, '<event pending>');
		end if;
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_li_provider) then
			perform @NAMESPACE@.storeNode_int (p_li_provider, '<event pending>');
		end if;
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_li_receiver) then
			perform @NAMESPACE@.storeNode_int (p_li_receiver, '<event pending>');
		end if;

		insert into @NAMESPACE@.sl_listen
				(li_origin, li_provider, li_receiver) values
				(p_li_origin, p_li_provider, p_li_receiver);
	end if;

	return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.storeListen_int(p_li_origin int4, p_li_provider int4, p_li_receiver int4) is
'FUNCTION storeListen_int (li_origin, li_provider, li_receiver)

Process STORE_LISTEN event, indicating that receiver node li_receiver
listens to node li_provider in order to get messages coming from node
li_origin.';


-- ----------------------------------------------------------------------
-- FUNCTION dropListen (li_origin, li_provider, li_receiver)
--
--	Generate the DROP_LISTEN event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropListen (p_li_origin int4, p_li_provider int4, p_li_receiver int4)
returns bigint
as $$
begin
	perform @NAMESPACE@.dropListen_int(p_li_origin, 
			p_li_provider, p_li_receiver);
	
	return  @NAMESPACE@.createEvent ('_@CLUSTERNAME@', 'DROP_LISTEN',
			p_li_origin::text, p_li_provider::text, p_li_receiver::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.dropListen(p_li_origin int4, p_li_provider int4, p_li_receiver int4) is
'dropListen (li_origin, li_provider, li_receiver)

Generate the DROP_LISTEN event.';

-- ----------------------------------------------------------------------
-- FUNCTION dropListen_int (li_origin, li_provider, li_receiver)
--
--	Process the DROP_LISTEN event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropListen_int (p_li_origin int4, p_li_provider int4, p_li_receiver int4)
returns int4
as $$
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	delete from @NAMESPACE@.sl_listen
			where li_origin = p_li_origin
			and li_provider = p_li_provider
			and li_receiver = p_li_receiver;
	if found then
		return 1;
	else
		return 0;
	end if;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.dropListen_int(p_li_origin int4, p_li_provider int4, p_li_receiver int4) is
'dropListen (li_origin, li_provider, li_receiver)

Process the DROP_LISTEN event, deleting the sl_listen entry for
the indicated (origin,provider,receiver) combination.';


-- ----------------------------------------------------------------------
-- FUNCTION storeSet (set_id, set_comment)
--
--	Generate the STORE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeSet (p_set_id int4, p_set_comment text)
returns bigint
as $$
declare
	v_local_node_id		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	insert into @NAMESPACE@.sl_set
			(set_id, set_origin, set_comment) values
			(p_set_id, v_local_node_id, p_set_comment);

	return @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'STORE_SET', 
			p_set_id::text, v_local_node_id::text, p_set_comment::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.storeSet(p_set_id int4, p_set_comment text) is
'Generate STORE_SET event for set set_id with human readable comment set_comment';

-- ----------------------------------------------------------------------
-- FUNCTION storeSet_int (set_id, set_origin, set_comment)
--
--	Process the STORE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.storeSet_int (p_set_id int4, p_set_origin int4, p_set_comment text)
returns int4
as $$
declare
	v_dummy				int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	select 1 into v_dummy
			from @NAMESPACE@.sl_set
			where set_id = p_set_id
			for update;
	if found then 
		update @NAMESPACE@.sl_set
				set set_comment = p_set_comment
				where set_id = p_set_id;
	else
		if not exists (select 1 from @NAMESPACE@.sl_node
						where no_id = p_set_origin) then
			perform @NAMESPACE@.storeNode_int (p_set_origin, '<event pending>');
		end if;
		insert into @NAMESPACE@.sl_set
				(set_id, set_origin, set_comment) values
				(p_set_id, p_set_origin, p_set_comment);
	end if;

	-- Run addPartialLogIndices() to try to add indices to unused sl_log_? table
	perform @NAMESPACE@.addPartialLogIndices();

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.storeSet_int(p_set_id int4, p_set_origin int4, p_set_comment text) is
'storeSet_int (set_id, set_origin, set_comment)

Process the STORE_SET event, indicating the new set with given ID,
origin node, and human readable comment.';


-- ----------------------------------------------------------------------
-- FUNCTION lockSet (set_id)
--
--	Add a special trigger to all tables of a set that disables
--	access to it.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.lockSet (p_set_id int4)
returns int4
as $$
declare
	v_local_node_id		int4;
	v_set_row			record;
	v_tab_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that the set exists and that we are the origin
	-- and that it is not already locked.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select * into v_set_row from @NAMESPACE@.sl_set
			where set_id = p_set_id
			for update;
	if not found then
		raise exception 'Slony-I: set % not found', p_set_id;
	end if;
	if v_set_row.set_origin <> v_local_node_id then
		raise exception 'Slony-I: set % does not originate on local node',
				p_set_id;
	end if;
	if v_set_row.set_locked notnull then
		raise exception 'Slony-I: set % is already locked', p_set_id;
	end if;

	-- ----
	-- Place the lockedSet trigger on all tables in the set.
	-- ----
	for v_tab_row in select T.tab_id,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
			from @NAMESPACE@.sl_table T,
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
			where T.tab_set = p_set_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid
			order by tab_id
	loop
		execute 'create trigger "_@CLUSTERNAME@_lockedset" ' || 
				'before insert or update or delete on ' ||
				v_tab_row.tab_fqname || ' for each row execute procedure
				@NAMESPACE@.lockedSet (''_@CLUSTERNAME@'');';
	end loop;

	-- ----
	-- Remember our snapshots xmax as for the set locking
	-- ----
	update @NAMESPACE@.sl_set
			set set_locked = "pg_catalog".txid_snapshot_xmax("pg_catalog".txid_current_snapshot())
			where set_id = p_set_id;

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.lockSet(p_set_id int4) is 
'lockSet(set_id)

Add a special trigger to all tables of a set that disables access to
it.';


-- ----------------------------------------------------------------------
-- FUNCTION unlockSet (set_id)
--
--	Remove the special trigger from all tables of a set that disables
--	access to it.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.unlockSet (p_set_id int4)
returns int4
as $$
declare
	v_local_node_id		int4;
	v_set_row			record;
	v_tab_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that the set exists and that we are the origin
	-- and that it is not already locked.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select * into v_set_row from @NAMESPACE@.sl_set
			where set_id = p_set_id
			for update;
	if not found then
		raise exception 'Slony-I: set % not found', p_set_id;
	end if;
	if v_set_row.set_origin <> v_local_node_id then
		raise exception 'Slony-I: set % does not originate on local node',
				p_set_id;
	end if;
	if v_set_row.set_locked isnull then
		raise exception 'Slony-I: set % is not locked', p_set_id;
	end if;

	-- ----
	-- Drop the lockedSet trigger from all tables in the set.
	-- ----
	for v_tab_row in select T.tab_id,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
			from @NAMESPACE@.sl_table T,
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
			where T.tab_set = p_set_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid
			order by tab_id
	loop
		execute 'drop trigger "_@CLUSTERNAME@_lockedset" ' || 
				'on ' || v_tab_row.tab_fqname;
	end loop;

	-- ----
	-- Clear out the set_locked field
	-- ----
	update @NAMESPACE@.sl_set
			set set_locked = NULL
			where set_id = p_set_id;

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.unlockSet(p_set_id int4) is
'Remove the special trigger from all tables of a set that disables access to it.';

-- ----------------------------------------------------------------------
-- FUNCTION moveSet (set_id, new_origin)
--
--	Generate the MOVE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.moveSet (p_set_id int4, p_new_origin int4)
returns bigint
as $$
declare
	v_local_node_id		int4;
	v_set_row			record;
	v_sub_row			record;
	v_sync_seqno		int8;
	v_lv_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that the set is locked and that this locking
	-- happened long enough ago.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select * into v_set_row from @NAMESPACE@.sl_set
			where set_id = p_set_id
			for update;
	if not found then
		raise exception 'Slony-I: set % not found', p_set_id;
	end if;
	if v_set_row.set_origin <> v_local_node_id then
		raise exception 'Slony-I: set % does not originate on local node',
				p_set_id;
	end if;
	if v_set_row.set_locked isnull then
		raise exception 'Slony-I: set % is not locked', p_set_id;
	end if;
	if v_set_row.set_locked > "pg_catalog".txid_snapshot_xmin("pg_catalog".txid_current_snapshot()) then
		raise exception 'Slony-I: cannot move set % yet, transactions < % are still in progress',
				p_set_id, v_set_row.set_locked;
	end if;

	-- ----
	-- Unlock the set
	-- ----
	perform @NAMESPACE@.unlockSet(p_set_id);

	-- ----
	-- Check that the new_origin is an active subscriber of the set
	-- ----
	select * into v_sub_row from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id
			and sub_receiver = p_new_origin;
	if not found then
		raise exception 'Slony-I: set % is not subscribed by node %',
				p_set_id, p_new_origin;
	end if;
	if not v_sub_row.sub_active then
		raise exception 'Slony-I: subsctiption of node % for set % is inactive',
				p_new_origin, p_set_id;
	end if;

	-- ----
	-- Reconfigure everything
	-- ----
	perform @NAMESPACE@.moveSet_int(p_set_id, v_local_node_id,
			p_new_origin, 0);

	perform @NAMESPACE@.RebuildListenEntries();

	-- ----
	-- At this time we hold access exclusive locks for every table
	-- in the set. But we did move the set to the new origin, so the
	-- createEvent() we are doing now will not record the sequences.
	-- ----
	v_sync_seqno := @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SYNC');
	insert into @NAMESPACE@.sl_seqlog 
			(seql_seqid, seql_origin, seql_ev_seqno, seql_last_value)
			select seq_id, v_local_node_id, v_sync_seqno, seq_last_value
			from @NAMESPACE@.sl_seqlastvalue
			where seq_set = p_set_id;
					
	-- ----
	-- Finally we generate the real event
	-- ----
	return @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'MOVE_SET', 
			p_set_id::text, v_local_node_id::text, p_new_origin::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.moveSet(p_set_id int4, p_new_origin int4) is 
'moveSet(set_id, new_origin)

Generate MOVE_SET event to request that the origin for set set_id be moved to node new_origin';

-- ----------------------------------------------------------------------
-- FUNCTION moveSet_int (set_id, old_origin, new_origin, wait_seqno)
--
--	Process the MOVE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.moveSet_int (p_set_id int4, p_old_origin int4, p_new_origin int4, p_wait_seqno int8)
returns int4
as $$
declare
	v_local_node_id		int4;
	v_tab_row			record;
	v_sub_row			record;
	v_sub_node			int4;
	v_sub_last			int4;
	v_sub_next			int4;
	v_last_sync			int8;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get our local node ID
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	-- On the new origin, raise an event - ACCEPT_SET
	if v_local_node_id = p_new_origin then
		-- Create a SYNC event as well so that the ACCEPT_SET has
		-- the same snapshot as the last SYNC generated by the new
		-- origin. This snapshot will be used by other nodes to
		-- finalize the setsync status.
		perform @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SYNC', NULL);
		perform @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'ACCEPT_SET', 
			p_set_id::text, p_old_origin::text, 
			p_new_origin::text, p_wait_seqno::text);
	end if;

	-- ----
	-- Next we have to reverse the subscription path
	-- ----
	v_sub_last = p_new_origin;
	select sub_provider into v_sub_node
			from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id
			and sub_receiver = p_new_origin;
	if not found then
		raise exception 'Slony-I: subscription path broken in moveSet_int';
	end if;
	while v_sub_node <> p_old_origin loop
		-- ----
		-- Tracing node by node, the old receiver is now in
		-- v_sub_last and the old provider is in v_sub_node.
		-- ----

		-- ----
		-- Get the current provider of this node as next
		-- and change the provider to the previous one in
		-- the reverse chain.
		-- ----
		select sub_provider into v_sub_next
				from @NAMESPACE@.sl_subscribe
				where sub_set = p_set_id
					and sub_receiver = v_sub_node
				for update;
		if not found then
			raise exception 'Slony-I: subscription path broken in moveSet_int';
		end if;
		update @NAMESPACE@.sl_subscribe
				set sub_provider = v_sub_last
				where sub_set = p_set_id
					and sub_receiver = v_sub_node
					and sub_receiver <> v_sub_last;

		v_sub_last = v_sub_node;
		v_sub_node = v_sub_next;
	end loop;

	-- ----
	-- This includes creating a subscription for the old origin
	-- ----
	insert into @NAMESPACE@.sl_subscribe
			(sub_set, sub_provider, sub_receiver,
			sub_forward, sub_active)
			values (p_set_id, v_sub_last, p_old_origin, true, true);
	if v_local_node_id = p_old_origin then
		select coalesce(max(ev_seqno), 0) into v_last_sync 
				from @NAMESPACE@.sl_event
				where ev_origin = p_new_origin
					and ev_type = 'SYNC';
		if v_last_sync > 0 then
			insert into @NAMESPACE@.sl_setsync
					(ssy_setid, ssy_origin, ssy_seqno,
					ssy_snapshot, ssy_action_list)
					select p_set_id, p_new_origin, v_last_sync,
					ev_snapshot, NULL
					from @NAMESPACE@.sl_event
					where ev_origin = p_new_origin
						and ev_seqno = v_last_sync;
		else
			insert into @NAMESPACE@.sl_setsync
					(ssy_setid, ssy_origin, ssy_seqno,
					ssy_snapshot, ssy_action_list)
					values (p_set_id, p_new_origin, '0',
					'1:1:', NULL);
		end if;
	end if;

	-- ----
	-- Now change the ownership of the set.
	-- ----
	update @NAMESPACE@.sl_set
			set set_origin = p_new_origin
			where set_id = p_set_id;

	-- ----
	-- On the new origin, delete the obsolete setsync information
	-- and the subscription.
	-- ----
	if v_local_node_id = p_new_origin then
		delete from @NAMESPACE@.sl_setsync
				where ssy_setid = p_set_id;
	else
		if v_local_node_id <> p_old_origin then
			--
			-- On every other node, change the setsync so that it will
			-- pick up from the new origins last known sync.
			--
			delete from @NAMESPACE@.sl_setsync
					where ssy_setid = p_set_id;
			select coalesce(max(ev_seqno), 0) into v_last_sync
					from @NAMESPACE@.sl_event
					where ev_origin = p_new_origin
						and ev_type = 'SYNC';
			if v_last_sync > 0 then
				insert into @NAMESPACE@.sl_setsync
						(ssy_setid, ssy_origin, ssy_seqno,
						ssy_snapshot, ssy_action_list)
						select p_set_id, p_new_origin, v_last_sync,
						ev_snapshot, NULL
						from @NAMESPACE@.sl_event
						where ev_origin = p_new_origin
							and ev_seqno = v_last_sync;
			else
				insert into @NAMESPACE@.sl_setsync
						(ssy_setid, ssy_origin, ssy_seqno,
						ssy_snapshot, ssy_action_list)
						values (p_set_id, p_new_origin,
						'0', '1:1:', NULL);
			end if;
		end if;
	end if;
	delete from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id
			and sub_receiver = p_new_origin;

	-- Regenerate sl_listen since we revised the subscriptions
	perform @NAMESPACE@.RebuildListenEntries();

	-- Run addPartialLogIndices() to try to add indices to unused sl_log_? table
	perform @NAMESPACE@.addPartialLogIndices();

	-- ----
	-- If we are the new or old origin, we have to
	-- adjust the log and deny access trigger configuration.
	-- ----
	if v_local_node_id = p_old_origin or v_local_node_id = p_new_origin then
		for v_tab_row in select tab_id from @NAMESPACE@.sl_table
				where tab_set = p_set_id
				order by tab_id
		loop
			perform @NAMESPACE@.alterTableConfigureTriggers(v_tab_row.tab_id);
		end loop;
	end if;

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.moveSet_int(p_set_id int4, p_old_origin int4, p_new_origin int4, p_wait_seqno int8) is 
'moveSet(set_id, old_origin, new_origin, wait_seqno)

Process MOVE_SET event to request that the origin for set set_id be
moved from old_origin to node new_origin';

-- ----------------------------------------------------------------------
-- FUNCTION dropSet (set_id)
--
--	Generate the DROP_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropSet (p_set_id int4)
returns bigint
as $$
declare
	v_origin			int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that the set exists and originates here
	-- ----
	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: set % not found', p_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				p_set_id;
	end if;

	-- ----
	-- Call the internal drop set functionality and generate the event
	-- ----
	perform @NAMESPACE@.dropSet_int(p_set_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'DROP_SET', 
			p_set_id::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.dropSet(p_set_id int4) is 
'Generate DROP_SET event to drop replication of set set_id';

-- ----------------------------------------------------------------------
-- FUNCTION dropSet_int (set_id)
--
--	Process the DROP_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.dropSet_int (p_set_id int4)
returns int4
as $$
declare
	v_tab_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Restore all tables original triggers and rules and remove
	-- our replication stuff.
	-- ----
	for v_tab_row in select tab_id from @NAMESPACE@.sl_table
			where tab_set = p_set_id
			order by tab_id
	loop
		perform @NAMESPACE@.alterTableDropTriggers(v_tab_row.tab_id);
	end loop;

	-- ----
	-- Remove all traces of the set configuration
	-- ----
	delete from @NAMESPACE@.sl_sequence
			where seq_set = p_set_id;
	delete from @NAMESPACE@.sl_table
			where tab_set = p_set_id;
	delete from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id;
	delete from @NAMESPACE@.sl_setsync
			where ssy_setid = p_set_id;
	delete from @NAMESPACE@.sl_set
			where set_id = p_set_id;

	-- Regenerate sl_listen since we revised the subscriptions
	perform @NAMESPACE@.RebuildListenEntries();

	-- Run addPartialLogIndices() to try to add indices to unused sl_log_? table
	perform @NAMESPACE@.addPartialLogIndices();

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.dropSet(p_set_id int4) is 
'Process DROP_SET event to drop replication of set set_id.  This involves:
- Removing log and deny access triggers
- Removing all traces of the set configuration, including sequences, tables, subscribers, syncs, and the set itself';

-- ----------------------------------------------------------------------
-- FUNCTION mergeSet (set_id, add_id)
--
--	Generate the MERGE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.mergeSet (p_set_id int4, p_add_id int4) 
returns bigint
as $$
declare
	v_origin			int4;
	in_progress			boolean;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that both sets exist and originate here
	-- ----
	if p_set_id = p_add_id then
		raise exception 'Slony-I: merged set ids cannot be identical';
	end if;
	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: set % not found', p_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				p_set_id;
	end if;

	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = p_add_id;
	if not found then
		raise exception 'Slony-I: set % not found', p_add_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				p_add_id;
	end if;

	-- ----
	-- Check that both sets are subscribed by the same set of nodes
	-- ----
	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = p_set_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = p_add_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				p_set_id, p_add_id;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = p_add_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = p_set_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				p_add_id, p_set_id;
	end if;

	-- ----
	-- Check that all ENABLE_SUBSCRIPTION events for the set are confirmed
	-- ----
	select @NAMESPACE@.isSubscriptionInProgress(p_add_id) into in_progress ;
	
	if in_progress then
		raise exception 'Slony-I: set % has subscriptions in progress - cannot merge',
				p_add_id;
	end if;

	-- ----
	-- Create a SYNC event, merge the sets, create a MERGE_SET event
	-- ----
	perform @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SYNC', NULL);
	perform @NAMESPACE@.mergeSet_int(p_set_id, p_add_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'MERGE_SET', 
			p_set_id::text, p_add_id::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.mergeSet(p_set_id int4, p_add_id int4) is 
'Generate MERGE_SET event to request that sets be merged together.

Both sets must exist, and originate on the same node.  They must be
subscribed by the same set of nodes.';


create or replace function @NAMESPACE@.isSubscriptionInProgress(p_add_id int4)
returns boolean
as $$
begin
	if exists (select true from @NAMESPACE@.sl_event
			where ev_type = 'ENABLE_SUBSCRIPTION'
			and ev_data1 = p_add_id::text
			and ev_seqno > (select max(con_seqno) from @NAMESPACE@.sl_confirm
					where con_origin = ev_origin
					and con_received::text = ev_data3))
	then
		return true;
	else
		return false;
	end if;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.isSubscriptionInProgress(p_add_id int4) is
'Checks to see if a subscription for the indicated set is in progress.
Returns true if a subscription is in progress. Otherwise false';

-- ----------------------------------------------------------------------
-- FUNCTION mergeSet_int (set_id, add_id)
--
--	Process the MERGE_SET event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.mergeSet_int (p_set_id int4, p_add_id int4)
returns int4
as $$
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	update @NAMESPACE@.sl_sequence
			set seq_set = p_set_id
			where seq_set = p_add_id;
	update @NAMESPACE@.sl_table
			set tab_set = p_set_id
			where tab_set = p_add_id;
	delete from @NAMESPACE@.sl_subscribe
			where sub_set = p_add_id;
	delete from @NAMESPACE@.sl_setsync
			where ssy_setid = p_add_id;
	delete from @NAMESPACE@.sl_set
			where set_id = p_add_id;

	return p_set_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.mergeSet_int(p_set_id int4, p_add_id int4) is
'mergeSet_int(set_id, add_id) - Perform MERGE_SET event, merging all objects from 
set add_id into set set_id.';

-- ----------------------------------------------------------------------
-- FUNCTION setAddTable (set_id, tab_id, tab_fqname, tab_idxname,
--					tab_comment)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setAddTable(p_set_id int4, p_tab_id int4, p_fqname text, p_tab_idxname name, p_tab_comment text)
returns bigint
as $$
declare
	v_set_origin		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that we are the origin of the set
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: setAddTable(): set % not found', p_set_id;
	end if;
	if v_set_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: setAddTable(): set % has remote origin', p_set_id;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id)
	then
		raise exception 'Slony-I: cannot add table to currently subscribed set % - must attach to an unsubscribed set',
				p_set_id;
	end if;

	-- ----
	-- Add the table to the set and generate the SET_ADD_TABLE event
	-- ----
	perform @NAMESPACE@.setAddTable_int(p_set_id, p_tab_id, p_fqname,
			p_tab_idxname, p_tab_comment);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_ADD_TABLE',
			p_set_id::text, p_tab_id::text, p_fqname::text,
			p_tab_idxname::text, p_tab_comment::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setAddTable(p_set_id int4, p_tab_id int4, p_fqname text, p_tab_idxname name, p_tab_comment text) is
'setAddTable (set_id, tab_id, tab_fqname, tab_idxname, tab_comment)

Add table tab_fqname to replication set on origin node, and generate
SET_ADD_TABLE event to allow this to propagate to other nodes.

Note that the table id, tab_id, must be unique ACROSS ALL SETS.';

-- ----------------------------------------------------------------------
-- FUNCTION setAddTable_int (set_id, tab_id, tab_fqname, tab_idxname,
--						tab_comment)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setAddTable_int(p_set_id int4, p_tab_id int4, p_fqname text, p_tab_idxname name, p_tab_comment text) 
returns int4
as $$
declare
	v_tab_relname		name;
	v_tab_nspname		name;
	v_local_node_id		int4;
	v_set_origin		int4;
	v_sub_provider		int4;
	v_relkind		char;
	v_tab_reloid		oid;
	v_pkcand_nn		boolean;
	v_prec			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- For sets with a remote origin, check that we are subscribed 
	-- to that set. Otherwise we ignore the table because it might 
	-- not even exist in our database.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: setAddTable_int(): set % not found',
				p_set_id;
	end if;
	if v_set_origin != v_local_node_id then
		select sub_provider into v_sub_provider
				from @NAMESPACE@.sl_subscribe
				where sub_set = p_set_id
				and sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
		if not found then
			return 0;
		end if;
	end if;
	
	-- ----
	-- Get the tables OID and check that it is a real table
	-- ----
	select PGC.oid, PGC.relkind, PGC.relname, PGN.nspname into v_tab_reloid, v_relkind, v_tab_relname, v_tab_nspname
			from "pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
			where PGC.relnamespace = PGN.oid
			and @NAMESPACE@.slon_quote_input(p_fqname) = @NAMESPACE@.slon_quote_brute(PGN.nspname) ||
					'.' || @NAMESPACE@.slon_quote_brute(PGC.relname);
	if not found then
		raise exception 'Slony-I: setAddTable_int(): table % not found', 
				p_fqname;
	end if;
	if v_relkind != 'r' then
		raise exception 'Slony-I: setAddTable_int(): % is not a regular table',
				p_fqname;
	end if;

	if not exists (select indexrelid
			from "pg_catalog".pg_index PGX, "pg_catalog".pg_class PGC
			where PGX.indrelid = v_tab_reloid
				and PGX.indexrelid = PGC.oid
				and PGC.relname = p_tab_idxname)
	then
		raise exception 'Slony-I: setAddTable_int(): table % has no index %',
				p_fqname, p_tab_idxname;
	end if;

	-- ----
	-- Verify that the columns in the PK (or candidate) are not NULLABLE
	-- ----

	v_pkcand_nn := 'f';
	for v_prec in select attname from "pg_catalog".pg_attribute where attrelid = 
                        (select oid from "pg_catalog".pg_class where oid = v_tab_reloid) 
                    and attname in (select attname from "pg_catalog".pg_attribute where 
                                    attrelid = (select oid from "pg_catalog".pg_class PGC, 
                                    "pg_catalog".pg_index PGX where 
                                    PGC.relname = p_tab_idxname and PGX.indexrelid=PGC.oid and
                                    PGX.indrelid = v_tab_reloid)) and attnotnull <> 't'
	loop
		raise notice 'Slony-I: setAddTable_int: table % PK column % nullable', p_fqname, v_prec.attname;
		v_pkcand_nn := 't';
	end loop;
	if v_pkcand_nn then
		raise exception 'Slony-I: setAddTable_int: table % not replicable!', p_fqname;
	end if;

	select * into v_prec from @NAMESPACE@.sl_table where tab_id = p_tab_id;
	if not found then
		v_pkcand_nn := 't';  -- No-op -- All is well
	else
		raise exception 'Slony-I: setAddTable_int: table id % has already been assigned!', p_tab_id;
	end if;

	-- ----
	-- Add the table to sl_table and create the trigger on it.
	-- ----
	insert into @NAMESPACE@.sl_table
			(tab_id, tab_reloid, tab_relname, tab_nspname, 
			tab_set, tab_idxname, tab_altered, tab_comment) 
			values
			(p_tab_id, v_tab_reloid, v_tab_relname, v_tab_nspname,
			p_set_id, p_tab_idxname, false, p_tab_comment);
	perform @NAMESPACE@.alterTableAddTriggers(p_tab_id);

	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setAddTable_int(p_set_id int4, p_tab_id int4, p_fqname text, p_tab_idxname name, p_tab_comment text) is
'setAddTable_int (set_id, tab_id, tab_fqname, tab_idxname, tab_comment)

This function processes the SET_ADD_TABLE event on remote nodes,
adding a table to replication if the remote node is subscribing to its
replication set.';

-- ----------------------------------------------------------------------
-- FUNCTION setDropTable (tab_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setDropTable(p_tab_id int4)
returns bigint
as $$
declare
	v_set_id		int4;
	v_set_origin		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

    -- ----
	-- Determine the set_id
    -- ----
	select tab_set into v_set_id from @NAMESPACE@.sl_table where tab_id = p_tab_id;

	-- ----
	-- Ensure table exists
	-- ----
	if not found then
		raise exception 'Slony-I: setDropTable_int(): table % not found',
			p_tab_id;
	end if;

	-- ----
	-- Check that we are the origin of the set
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = v_set_id;
	if not found then
		raise exception 'Slony-I: setDropTable(): set % not found', v_set_id;
	end if;
	if v_set_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: setDropTable(): set % has remote origin', v_set_id;
	end if;

	-- ----
	-- Drop the table from the set and generate the SET_ADD_TABLE event
	-- ----
	perform @NAMESPACE@.setDropTable_int(p_tab_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_DROP_TABLE', 
				p_tab_id::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setDropTable(p_tab_id int4) is
'setDropTable (tab_id)

Drop table tab_id from set on origin node, and generate SET_DROP_TABLE
event to allow this to propagate to other nodes.';

-- ----------------------------------------------------------------------
-- FUNCTION setDropTable_int (tab_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setDropTable_int(p_tab_id int4)
returns int4
as $$
declare
	v_set_id		int4;
	v_local_node_id		int4;
	v_set_origin		int4;
	v_sub_provider		int4;
	v_tab_reloid		oid;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

    -- ----
	-- Determine the set_id
    -- ----
	select tab_set into v_set_id from @NAMESPACE@.sl_table where tab_id = p_tab_id;

	-- ----
	-- Ensure table exists
	-- ----
	if not found then
		return 0;
	end if;

	-- ----
	-- For sets with a remote origin, check that we are subscribed 
	-- to that set. Otherwise we ignore the table because it might 
	-- not even exist in our database.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = v_set_id;
	if not found then
		raise exception 'Slony-I: setDropTable_int(): set % not found',
				v_set_id;
	end if;
	if v_set_origin != v_local_node_id then
		select sub_provider into v_sub_provider
				from @NAMESPACE@.sl_subscribe
				where sub_set = v_set_id
				and sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
		if not found then
			return 0;
		end if;
	end if;
	
	-- ----
	-- Drop the table from sl_table and drop trigger from it.
	-- ----
	perform @NAMESPACE@.alterTableDropTriggers(p_tab_id);
	delete from @NAMESPACE@.sl_table where tab_id = p_tab_id;
	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setDropTable_int(p_tab_id int4) is
'setDropTable_int (tab_id)

This function processes the SET_DROP_TABLE event on remote nodes,
dropping a table from replication if the remote node is subscribing to
its replication set.';

-- ----------------------------------------------------------------------
-- FUNCTION setAddSequence (set_id, seq_id, seq_fqname, seq_comment)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setAddSequence (p_set_id int4, p_seq_id int4, p_fqname text, p_seq_comment text)
returns bigint
as $$
declare
	v_set_origin		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that we are the origin of the set
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: setAddSequence(): set % not found', p_set_id;
	end if;
	if v_set_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: setAddSequence(): set % has remote origin - submit to origin node', p_set_id;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe
			where sub_set = p_set_id)
	then
		raise exception 'Slony-I: cannot add sequence to currently subscribed set %',
				p_set_id;
	end if;

	-- ----
	-- Add the sequence to the set and generate the SET_ADD_SEQUENCE event
	-- ----
	perform @NAMESPACE@.setAddSequence_int(p_set_id, p_seq_id, p_fqname,
			p_seq_comment);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_ADD_SEQUENCE',
						p_set_id::text, p_seq_id::text, 
						p_fqname::text, p_seq_comment::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setAddSequence (p_set_id int4, p_seq_id int4, p_fqname text, p_seq_comment text) is
'setAddSequence (set_id, seq_id, seq_fqname, seq_comment)

On the origin node for set set_id, add sequence seq_fqname to the
replication set, and raise SET_ADD_SEQUENCE to cause this to replicate
to subscriber nodes.';

-- ----------------------------------------------------------------------
-- FUNCTION setAddSequence_int (set_id, seq_id, seq_fqname, seq_comment
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setAddSequence_int(p_set_id int4, p_seq_id int4, p_fqname text, p_seq_comment text)
returns int4
as $$
declare
	v_local_node_id		int4;
	v_set_origin		int4;
	v_sub_provider		int4;
	v_relkind			char;
	v_seq_reloid		oid;
	v_seq_relname		name;
	v_seq_nspname		name;
	v_sync_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- For sets with a remote origin, check that we are subscribed 
	-- to that set. Otherwise we ignore the sequence because it might 
	-- not even exist in our database.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_set_id;
	if not found then
		raise exception 'Slony-I: setAddSequence_int(): set % not found',
				p_set_id;
	end if;
	if v_set_origin != v_local_node_id then
		select sub_provider into v_sub_provider
				from @NAMESPACE@.sl_subscribe
				where sub_set = p_set_id
				and sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
		if not found then
			return 0;
		end if;
	end if;
	
	-- ----
	-- Get the sequences OID and check that it is a sequence
	-- ----
	select PGC.oid, PGC.relkind, PGC.relname, PGN.nspname 
		into v_seq_reloid, v_relkind, v_seq_relname, v_seq_nspname
			from "pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
			where PGC.relnamespace = PGN.oid
			and @NAMESPACE@.slon_quote_input(p_fqname) = @NAMESPACE@.slon_quote_brute(PGN.nspname) ||
					'.' || @NAMESPACE@.slon_quote_brute(PGC.relname);
	if not found then
		raise exception 'Slony-I: setAddSequence_int(): sequence % not found', 
				p_fqname;
	end if;
	if v_relkind != 'S' then
		raise exception 'Slony-I: setAddSequence_int(): % is not a sequence',
				p_fqname;
	end if;

        select 1 into v_sync_row from @NAMESPACE@.sl_sequence where seq_id = p_seq_id;
	if not found then
               v_relkind := 'o';   -- all is OK
        else
                raise exception 'Slony-I: setAddSequence_int(): sequence ID % has already been assigned', p_seq_id;
        end if;

	-- ----
	-- Add the sequence to sl_sequence
	-- ----
	insert into @NAMESPACE@.sl_sequence
		(seq_id, seq_reloid, seq_relname, seq_nspname, seq_set, seq_comment) 
		values
		(p_seq_id, v_seq_reloid, v_seq_relname, v_seq_nspname,  p_set_id, p_seq_comment);

	-- ----
	-- On the set origin, fake a sl_seqlog row for the last sync event
	-- ----
	if v_set_origin = v_local_node_id then
		for v_sync_row in select coalesce (max(ev_seqno), 0) as ev_seqno
				from @NAMESPACE@.sl_event
				where ev_origin = v_local_node_id
					and ev_type = 'SYNC'
		loop
			insert into @NAMESPACE@.sl_seqlog
					(seql_seqid, seql_origin, seql_ev_seqno, 
					seql_last_value) values
					(p_seq_id, v_local_node_id, v_sync_row.ev_seqno,
					@NAMESPACE@.sequenceLastValue(p_fqname));
		end loop;
	end if;

	return p_seq_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setAddSequence_int(p_set_id int4, p_seq_id int4, p_fqname text, p_seq_comment text) is
'setAddSequence_int (set_id, seq_id, seq_fqname, seq_comment)

This processes the SET_ADD_SEQUENCE event.  On remote nodes that
subscribe to set_id, add the sequence to the replication set.';

-- ----------------------------------------------------------------------
-- FUNCTION setDropSequence (seq_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setDropSequence (p_seq_id int4)
returns bigint
as $$
declare
	v_set_id		int4;
	v_set_origin		int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Determine set id for this sequence
	-- ----
	select seq_set into v_set_id from @NAMESPACE@.sl_sequence where seq_id = p_seq_id;

	-- ----
	-- Ensure sequence exists
	-- ----
	if not found then
		raise exception 'Slony-I: setDropSequence_int(): sequence % not found',
			p_seq_id;
	end if;

	-- ----
	-- Check that we are the origin of the set
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = v_set_id;
	if not found then
		raise exception 'Slony-I: setDropSequence(): set % not found', v_set_id;
	end if;
	if v_set_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: setDropSequence(): set % has origin at another node - submit this to that node', v_set_id;
	end if;

	-- ----
	-- Add the sequence to the set and generate the SET_ADD_SEQUENCE event
	-- ----
	perform @NAMESPACE@.setDropSequence_int(p_seq_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_DROP_SEQUENCE',
					p_seq_id::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setDropSequence (p_seq_id int4) is
'setDropSequence (seq_id)

On the origin node for the set, drop sequence seq_id from replication
set, and raise SET_DROP_SEQUENCE to cause this to replicate to
subscriber nodes.';

-- ----------------------------------------------------------------------
-- FUNCTION setDropSequence_int (seq_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setDropSequence_int(p_seq_id int4)
returns int4
as $$
declare
	v_set_id		int4;
	v_local_node_id		int4;
	v_set_origin		int4;
	v_sub_provider		int4;
	v_relkind			char;
	v_sync_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Determine set id for this sequence
	-- ----
	select seq_set into v_set_id from @NAMESPACE@.sl_sequence where seq_id = p_seq_id;

	-- ----
	-- Ensure sequence exists
	-- ----
	if not found then
		return 0;
	end if;

	-- ----
	-- For sets with a remote origin, check that we are subscribed 
	-- to that set. Otherwise we ignore the sequence because it might 
	-- not even exist in our database.
	-- ----
	v_local_node_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = v_set_id;
	if not found then
		raise exception 'Slony-I: setDropSequence_int(): set % not found',
				v_set_id;
	end if;
	if v_set_origin != v_local_node_id then
		select sub_provider into v_sub_provider
				from @NAMESPACE@.sl_subscribe
				where sub_set = v_set_id
				and sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
		if not found then
			return 0;
		end if;
	end if;

	-- ----
	-- drop the sequence from sl_sequence, sl_seqlog
	-- ----
	delete from @NAMESPACE@.sl_seqlog where seql_seqid = p_seq_id;
	delete from @NAMESPACE@.sl_sequence where seq_id = p_seq_id;

	return p_seq_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setDropSequence_int(p_seq_id int4) is
'setDropSequence_int (seq_id)

This processes the SET_DROP_SEQUENCE event.  On remote nodes that
subscribe to the set containing sequence seq_id, drop the sequence
from the replication set.';


-- ----------------------------------------------------------------------
-- FUNCTION setMoveTable (tab_id, new_set_id)
--
--	Generate the SET_MOVE_TABLE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setMoveTable (p_tab_id int4, p_new_set_id int4)
returns bigint
as $$
declare
	v_old_set_id		int4;
	v_origin			int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get the tables current set
	-- ----
	select tab_set into v_old_set_id from @NAMESPACE@.sl_table
			where tab_id = p_tab_id;
	if not found then
		raise exception 'Slony-I: table %d not found', p_tab_id;
	end if;
	
	-- ----
	-- Check that both sets exist and originate here
	-- ----
	if p_new_set_id = v_old_set_id then
		raise exception 'Slony-I: set ids cannot be identical';
	end if;
	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = p_new_set_id;
	if not found then
		raise exception 'Slony-I: set % not found', p_new_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				p_new_set_id;
	end if;

	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = v_old_set_id;
	if not found then
		raise exception 'Slony-I: set % not found', v_old_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				v_old_set_id;
	end if;

	-- ----
	-- Check that both sets are subscribed by the same set of nodes
	-- ----
	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = p_new_set_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = v_old_set_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				p_new_set_id, v_old_set_id;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = v_old_set_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = p_new_set_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				v_old_set_id, p_new_set_id;
	end if;

	-- ----
	-- Change the set the table belongs to
	-- ----
	perform @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SYNC', NULL);
	perform @NAMESPACE@.setMoveTable_int(p_tab_id, p_new_set_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_MOVE_TABLE', 
			p_tab_id::text, p_new_set_id::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.setMoveTable(p_tab_id int4, p_new_set_id int4) is
'This generates the SET_MOVE_TABLE event.  If the set that the table is
in is identically subscribed to the set that the table is to be moved 
into, then the SET_MOVE_TABLE event is raised.';


-- ----------------------------------------------------------------------
-- FUNCTION setMoveTable_int (tab_id, new_set_id)
--
--	Process the SET_MOVE_TABLE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setMoveTable_int (p_tab_id int4, p_new_set_id int4)
returns int4
as $$
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Move the table to the new set
	-- ----
	update @NAMESPACE@.sl_table
			set tab_set = p_new_set_id
			where tab_id = p_tab_id;

	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.setMoveTable(p_tab_id int4, p_new_set_id int4) is
'This processes the SET_MOVE_TABLE event.  The table is moved 
to the destination set.';

-- ----------------------------------------------------------------------
-- FUNCTION setMoveSequence (seq_id, new_set_id)
--
--	Generate the SET_MOVE_SEQUENCE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setMoveSequence (p_seq_id int4, p_new_set_id int4)
returns bigint
as $$
declare
	v_old_set_id		int4;
	v_origin			int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get the sequences current set
	-- ----
	select seq_set into v_old_set_id from @NAMESPACE@.sl_sequence
			where seq_id = p_seq_id;
	if not found then
		raise exception 'Slony-I: setMoveSequence(): sequence %d not found', p_seq_id;
	end if;
	
	-- ----
	-- Check that both sets exist and originate here
	-- ----
	if p_new_set_id = v_old_set_id then
		raise exception 'Slony-I: setMoveSequence(): set ids cannot be identical';
	end if;
	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = p_new_set_id;
	if not found then
		raise exception 'Slony-I: setMoveSequence(): set % not found', p_new_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: setMoveSequence(): set % does not originate on local node',
				p_new_set_id;
	end if;

	select set_origin into v_origin from @NAMESPACE@.sl_set
			where set_id = v_old_set_id;
	if not found then
		raise exception 'Slony-I: set % not found', v_old_set_id;
	end if;
	if v_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: set % does not originate on local node',
				v_old_set_id;
	end if;

	-- ----
	-- Check that both sets are subscribed by the same set of nodes
	-- ----
	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = p_new_set_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = v_old_set_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				p_new_set_id, v_old_set_id;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe SUB1
				where SUB1.sub_set = v_old_set_id
				and SUB1.sub_receiver not in (select SUB2.sub_receiver
						from @NAMESPACE@.sl_subscribe SUB2
						where SUB2.sub_set = p_new_set_id))
	then
		raise exception 'Slony-I: subscriber lists of set % and % are different',
				v_old_set_id, p_new_set_id;
	end if;

	-- ----
	-- Change the set the sequence belongs to
	-- ----
	perform @NAMESPACE@.setMoveSequence_int(p_seq_id, p_new_set_id);
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SET_MOVE_SEQUENCE', 
			p_seq_id::text, p_new_set_id::text);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.setMoveSequence (p_seq_id int4, p_new_set_id int4) is
'setMoveSequence(p_seq_id, p_new_set_id) - This generates the
SET_MOVE_SEQUENCE event, after validation, notably that both sets
exist, are distinct, and have exactly the same subscription lists';


-- ----------------------------------------------------------------------
-- FUNCTION setMoveSequence_int (seq_id, new_set_id)
--
--	Process the SET_MOVE_SEQUENCE event.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.setMoveSequence_int (p_seq_id int4, p_new_set_id int4)
returns int4
as $$
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Move the sequence to the new set
	-- ----
	update @NAMESPACE@.sl_sequence
			set seq_set = p_new_set_id
			where seq_id = p_seq_id;

	return p_seq_id;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.setMoveSequence_int (p_seq_id int4, p_new_set_id int4) is
'setMoveSequence_int(p_seq_id, p_new_set_id) - processes the
SET_MOVE_SEQUENCE event, moving a sequence to another replication
set.';

-- ----------------------------------------------------------------------
-- FUNCTION sequenceSetValue (seq_id, seq_origin, ev_seqno, last_value)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.sequenceSetValue(p_seq_id int4, p_seq_origin int4, p_ev_seqno int8, p_last_value int8,p_ignore_missing bool) returns int4
as $$
declare
	v_fqname			text;
	v_found                         integer;
begin
	-- ----
	-- Get the sequences fully qualified name
	-- ----
	select @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) into v_fqname
		from @NAMESPACE@.sl_sequence SQ,
			"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
		where SQ.seq_id = p_seq_id
			and SQ.seq_reloid = PGC.oid
			and PGC.relnamespace = PGN.oid;
	if not found then
	        if p_ignore_missing then
                       return null;
                end if;
		raise exception 'Slony-I: sequenceSetValue(): sequence % not found', p_seq_id;
	end if;

	-- ----
	-- Update it to the new value
	-- ----
	execute 'select setval(''' || v_fqname ||
			''', ' || p_last_value::text || ')';

	if p_ev_seqno is not null then
	   insert into @NAMESPACE@.sl_seqlog
			(seql_seqid, seql_origin, seql_ev_seqno, seql_last_value)
			values (p_seq_id, p_seq_origin, p_ev_seqno, p_last_value);
	end if;
	return p_seq_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.sequenceSetValue(p_seq_id int4, p_seq_origin int4, p_ev_seqno int8, p_last_value int8,p_ignore_missing bool) is
'sequenceSetValue (seq_id, seq_origin, ev_seqno, last_value,ignore_missing)
Set sequence seq_id to have new value last_value.
';

--
-- 2.2.3 changed the return type of ddlCapture 
-- drop it to allow a new return type.
drop function if exists @NAMESPACE@.ddlCapture (p_statement text, p_nodes text);
-- ----------------------------------------------------------------------
-- FUNCTION ddlCapture (statement, nodes)
--
--	Capture DDL into sl_log_script
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.ddlCapture (p_statement text, p_nodes text)
returns bigint
as $$
declare
	c_local_node	integer;
	c_found_origin	boolean;
	c_node			text;
	c_cmdargs		text[];
	c_nodeargs      text;
	c_delim         text;
begin
	c_local_node := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	c_cmdargs = array_append('{}'::text[], p_statement);
	c_nodeargs = '';
	if p_nodes is not null then
		c_found_origin := 'f';
		-- p_nodes list needs to consist of a list of nodes that exist
		-- and that include the current node ID
		for c_node in select trim(node) from
				pg_catalog.regexp_split_to_table(p_nodes, ',') as node loop
			if not exists 
					(select 1 from @NAMESPACE@.sl_node 
					where no_id = (c_node::integer)) then
				raise exception 'ddlcapture(%,%) - node % does not exist!', 
					p_statement, p_nodes, c_node;
		   end if;

		   if c_local_node = (c_node::integer) then
		   	  c_found_origin := 't';
		   end if;
		   if length(c_nodeargs)>0 then
		   	  c_nodeargs = c_nodeargs ||','|| c_node;
		   else
				c_nodeargs=c_node;
			end if;
	   end loop;

		if not c_found_origin then
			raise exception 
				'ddlcapture(%,%) - origin node % not included in ONLY ON list!',
				p_statement, p_nodes, c_local_node;
       end if;
    end if;
	c_cmdargs = array_append(c_cmdargs,c_nodeargs);
	c_delim=',';
	c_cmdargs = array_append(c_cmdargs, 

           (select @NAMESPACE@.string_agg( seq_id::text || c_delim
		   || c_local_node ||
		    c_delim || seq_last_value)
		    FROM (
		       select seq_id,
           	   seq_last_value from @NAMESPACE@.sl_seqlastvalue
           	   where seq_origin = c_local_node) as FOO
			where NOT @NAMESPACE@.seqtrack(seq_id,seq_last_value) is NULL));
	insert into @NAMESPACE@.sl_log_script
			(log_origin, log_txid, log_actionseq, log_cmdtype, log_cmdargs)
		values 
			(c_local_node, pg_catalog.txid_current(), 
			nextval('@NAMESPACE@.sl_action_seq'), 'S', c_cmdargs);
	execute p_statement;
	return currval('@NAMESPACE@.sl_action_seq');
end;
$$ language plpgsql;

comment on function @NAMESPACE@.ddlCapture (p_statement text, p_nodes text) is
'Capture an SQL statement (usually DDL) that is to be literally replayed on subscribers';


-- ----------------------------------------------------------------------
-- FUNCTION ddlScript_complete (p_nodes)
--
--	Actions to be performed after DDL script execution
-- ----------------------------------------------------------------------
drop function if exists @NAMESPACE@.ddlScript_complete (int4, text, int4);  -- Needed because function signature has changed!

create or replace function @NAMESPACE@.ddlScript_complete (p_nodes text)
returns bigint
as $$
declare
	c_local_node	integer;
	c_found_origin	boolean;
	c_node			text;
	c_cmdargs		text[];
begin
	c_local_node := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	c_cmdargs = '{}'::text[];
	if p_nodes is not null then
		c_found_origin := 'f';
		-- p_nodes list needs to consist o a list of nodes that exist
		-- and that include the current node ID
		for c_node in select trim(node) from
				pg_catalog.regexp_split_to_table(p_nodes, ',') as node loop
			if not exists 
					(select 1 from @NAMESPACE@.sl_node 
					where no_id = (c_node::integer)) then
				raise exception 'ddlcapture(%,%) - node % does not exist!', 
					p_statement, p_nodes, c_node;
		   end if;

		   if c_local_node = (c_node::integer) then
		   	  c_found_origin := 't';
		   end if;

		   c_cmdargs = array_append(c_cmdargs, c_node);
	   end loop;

		if not c_found_origin then
			raise exception 
				'ddlScript_complete(%) - origin node % not included in ONLY ON list!',
				p_nodes, c_local_node;
       end if;
    end if;

	perform @NAMESPACE@.ddlScript_complete_int();

	insert into @NAMESPACE@.sl_log_script
			(log_origin, log_txid, log_actionseq, log_cmdtype, log_cmdargs)
		values 
			(c_local_node, pg_catalog.txid_current(), 
			nextval('@NAMESPACE@.sl_action_seq'), 's', c_cmdargs);

	return currval('@NAMESPACE@.sl_action_seq');
end;
$$ language plpgsql;

comment on function @NAMESPACE@.ddlScript_complete(p_nodes text) is
'ddlScript_complete(set_id, script, only_on_node)

After script has run on origin, this fixes up relnames and
log trigger arguments and inserts the "fire ddlScript_complete_int()
log row into sl_log_script.';

-- ----------------------------------------------------------------------
-- FUNCTION ddlScript_complete_int ()
--
--	Complete the DDL_SCRIPT event
-- ----------------------------------------------------------------------
drop function if exists @NAMESPACE@.ddlScript_complete_int(int4, int4);
create or replace function @NAMESPACE@.ddlScript_complete_int ()
returns int4
as $$
begin
	perform @NAMESPACE@.updateRelname();
	perform @NAMESPACE@.repair_log_triggers(true);
	return 0;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.ddlScript_complete_int() is
'ddlScript_complete_int()

Complete processing the DDL_SCRIPT event.';

-- ----------------------------------------------------------------------
-- FUNCTION alterTableAddTriggers (tab_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.alterTableAddTriggers (p_tab_id int4)
returns int4
as $$
declare
	v_no_id				int4;
	v_tab_row			record;
	v_tab_fqname		text;
	v_tab_attkind		text;
	v_n					int4;
	v_trec	record;
	v_tgbad	boolean;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get our local node ID
	-- ----
	v_no_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	-- ----
	-- Get the sl_table row and the current origin of the table. 
	-- ----
	select T.tab_reloid, T.tab_set, T.tab_idxname, 
			S.set_origin, PGX.indexrelid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
			into v_tab_row
			from @NAMESPACE@.sl_table T, @NAMESPACE@.sl_set S,
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN,
				"pg_catalog".pg_index PGX, "pg_catalog".pg_class PGXC
			where T.tab_id = p_tab_id
				and T.tab_set = S.set_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid
				and PGX.indrelid = T.tab_reloid
				and PGX.indexrelid = PGXC.oid
				and PGXC.relname = T.tab_idxname
				for update;
	if not found then
		raise exception 'Slony-I: alterTableAddTriggers(): Table with id % not found', p_tab_id;
	end if;
	v_tab_fqname = v_tab_row.tab_fqname;

	v_tab_attkind := @NAMESPACE@.determineAttKindUnique(v_tab_row.tab_fqname, 
						v_tab_row.tab_idxname);

	execute 'lock table ' || v_tab_fqname || ' in access exclusive mode';

	-- ----
	-- Create the log and the deny access triggers
	-- ----
	execute 'create trigger "_@CLUSTERNAME@_logtrigger"' || 
			' after insert or update or delete on ' ||
			v_tab_fqname || ' for each row execute procedure @NAMESPACE@.logTrigger (' ||
                               pg_catalog.quote_literal('_@CLUSTERNAME@') || ',' || 
				pg_catalog.quote_literal(p_tab_id::text) || ',' || 
				pg_catalog.quote_literal(v_tab_attkind) || ');';

	execute 'create trigger "_@CLUSTERNAME@_denyaccess" ' || 
			'before insert or update or delete on ' ||
			v_tab_fqname || ' for each row execute procedure ' ||
			'@NAMESPACE@.denyAccess (' || pg_catalog.quote_literal('_@CLUSTERNAME@') || ');';

	perform @NAMESPACE@.alterTableAddTruncateTrigger(v_tab_fqname, p_tab_id);

	perform @NAMESPACE@.alterTableConfigureTriggers (p_tab_id);
	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.alterTableAddTriggers(p_tab_id int4) is
'alterTableAddTriggers(tab_id)

Adds the log and deny access triggers to a replicated table.';

-- ----------------------------------------------------------------------
-- FUNCTION alterTableDropTriggers (tab_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.alterTableDropTriggers (p_tab_id int4)
returns int4
as $$
declare
	v_no_id				int4;
	v_tab_row			record;
	v_tab_fqname		text;
	v_n					int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get our local node ID
	-- ----
	v_no_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	-- ----
	-- Get the sl_table row and the current tables origin.
	-- ----
	select T.tab_reloid, T.tab_set,
			S.set_origin, PGX.indexrelid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
			into v_tab_row
			from @NAMESPACE@.sl_table T, @NAMESPACE@.sl_set S,
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN,
				"pg_catalog".pg_index PGX, "pg_catalog".pg_class PGXC
			where T.tab_id = p_tab_id
				and T.tab_set = S.set_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid
				and PGX.indrelid = T.tab_reloid
				and PGX.indexrelid = PGXC.oid
				and PGXC.relname = T.tab_idxname
				for update;
	if not found then
		raise exception 'Slony-I: alterTableDropTriggers(): Table with id % not found', p_tab_id;
	end if;
	v_tab_fqname = v_tab_row.tab_fqname;

	execute 'lock table ' || v_tab_fqname || ' in access exclusive mode';

	-- ----
	-- Drop both triggers
	-- ----
	execute 'drop trigger "_@CLUSTERNAME@_logtrigger" on ' || 
			v_tab_fqname;

	execute 'drop trigger "_@CLUSTERNAME@_denyaccess" on ' || 
			v_tab_fqname;
				
	perform @NAMESPACE@.alterTableDropTruncateTrigger(v_tab_fqname, p_tab_id);

	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.alterTableDropTriggers (p_tab_id int4) is
'alterTableDropTriggers (tab_id)

Remove the log and deny access triggers from a table.';

-- ----------------------------------------------------------------------
-- FUNCTION alterTableConfigureTriggers (tab_id)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.alterTableConfigureTriggers (p_tab_id int4)
returns int4
as $$
declare
	v_no_id				int4;
	v_tab_row			record;
	v_tab_fqname		text;
	v_n					int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Get our local node ID
	-- ----
	v_no_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');

	-- ----
	-- Get the sl_table row and the current tables origin.
	-- ----
	select T.tab_reloid, T.tab_set,
			S.set_origin, PGX.indexrelid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
			into v_tab_row
			from @NAMESPACE@.sl_table T, @NAMESPACE@.sl_set S,
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN,
				"pg_catalog".pg_index PGX, "pg_catalog".pg_class PGXC
			where T.tab_id = p_tab_id
				and T.tab_set = S.set_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid
				and PGX.indrelid = T.tab_reloid
				and PGX.indexrelid = PGXC.oid
				and PGXC.relname = T.tab_idxname
				for update;
	if not found then
		raise exception 'Slony-I: alterTableConfigureTriggers(): Table with id % not found', p_tab_id;
	end if;
	v_tab_fqname = v_tab_row.tab_fqname;

	-- ----
	-- Configuration depends on the origin of the table
	-- ----
	if v_tab_row.set_origin = v_no_id then
		-- ----
		-- On the origin the log trigger is configured like a default
		-- user trigger and the deny access trigger is disabled.
		-- ----
		execute 'alter table ' || v_tab_fqname ||
				' enable trigger "_@CLUSTERNAME@_logtrigger"';
		execute 'alter table ' || v_tab_fqname ||
				' disable trigger "_@CLUSTERNAME@_denyaccess"';
        perform @NAMESPACE@.alterTableConfigureTruncateTrigger(v_tab_fqname,
				'enable', 'disable');
	else
		-- ----
		-- On a replica the log trigger is disabled and the
		-- deny access trigger fires in origin session role.
		-- ----
		execute 'alter table ' || v_tab_fqname ||
				' disable trigger "_@CLUSTERNAME@_logtrigger"';
		execute 'alter table ' || v_tab_fqname ||
				' enable trigger "_@CLUSTERNAME@_denyaccess"';
        perform @NAMESPACE@.alterTableConfigureTruncateTrigger(v_tab_fqname,
				'disable', 'enable');

	end if;

	return p_tab_id;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.alterTableConfigureTriggers (p_tab_id int4) is
'alterTableConfigureTriggers (tab_id)

Set the enable/disable configuration for the replication triggers
according to the origin of the set.';



-- ----------------------------------------------------------------------
-- FUNCTION subscribeSet (sub_set, sub_provider, sub_receiver, sub_forward, omit_copy)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.resubscribeNode (p_origin int4, 
p_provider int4, p_receiver int4)
returns bigint
as $$
declare
	v_record record;
	v_missing_sets text;
	v_ev_seqno bigint;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	--
	-- Check that the receiver exists
	--
	if not exists (select no_id from @NAMESPACE@.sl_node where no_id=
	       	      p_receiver) then
		      raise exception 'Slony-I: subscribeSet() receiver % does not exist' , p_receiver;
	end if;

	--
	-- Check that the provider exists
	--
	if not exists (select no_id from @NAMESPACE@.sl_node where no_id=
	       	      p_provider) then
		      raise exception 'Slony-I: subscribeSet() provider % does not exist' , p_provider;
	end if;

	
	-- ----
	-- Check that this is called on the origin node
	-- ----
	if p_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: subscribeSet() must be called on origin';
	end if;

	-- ---
	-- Verify that the provider is either the origin or an active subscriber
	-- Bug report #1362
	-- ---
	if p_origin <> p_provider then
	   for v_record in select sub1.sub_set from 
		    @NAMESPACE@.sl_subscribe sub1			
		    left outer join  (@NAMESPACE@.sl_subscribe sub2 
				 inner join
				 @NAMESPACE@.sl_set  on (
								sl_set.set_id=sub2.sub_set
								and sub2.sub_set=p_origin)
								)
			ON (  sub1.sub_set = sub2.sub_set and 
                  sub1.sub_receiver = p_provider and
			      sub1.sub_forward and sub1.sub_active
				  and sub2.sub_receiver=p_receiver)
		
			where sub2.sub_set is null 
		loop
				v_missing_sets=v_missing_sets || ' ' || v_record.sub_set;
		end loop;
		if v_missing_sets is not null then
			raise exception 'Slony-I: subscribeSet(): provider % is not an active forwarding node for replication set %', p_sub_provider, v_missing_sets;
		end if;
	end if;

	for v_record in select *  from 
		@NAMESPACE@.sl_subscribe, @NAMESPACE@.sl_set where 
		sub_set=set_id and
		sub_receiver=p_receiver
		and set_origin=p_origin
	loop
	-- ----
	-- Create the SUBSCRIBE_SET event
	-- ----
	   v_ev_seqno :=  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SUBSCRIBE_SET', 
				  v_record.sub_set::text, p_provider::text, p_receiver::text, 
				  case v_record.sub_forward when true then 't' else 'f' end,
				  	   'f' );

		-- ----
		-- Call the internal procedure to store the subscription
		-- ----
		perform @NAMESPACE@.subscribeSet_int(v_record.sub_set, 
				p_provider,
				p_receiver, v_record.sub_forward, false);
	end loop;

	return v_ev_seqno;	
end;
$$
language plpgsql;

-- ----------------------------------------------------------------------
-- FUNCTION subscribeSet (sub_set, sub_provider, sub_receiver, sub_forward, omit_copy)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.subscribeSet (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4, p_sub_forward bool, p_omit_copy bool)
returns bigint
as $$
declare
	v_set_origin		int4;
	v_ev_seqno			int8;
	v_ev_seqno2			int8;
	v_rec			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	--
	-- Check that the receiver exists
	--
	if not exists (select no_id from @NAMESPACE@.sl_node where no_id=
	       	      p_sub_receiver) then
		      raise exception 'Slony-I: subscribeSet() receiver % does not exist' , p_sub_receiver;
	end if;

	--
	-- Check that the provider exists
	--
	if not exists (select no_id from @NAMESPACE@.sl_node where no_id=
	       	      p_sub_provider) then
		      raise exception 'Slony-I: subscribeSet() provider % does not exist' , p_sub_provider;
	end if;

	-- ----
	-- Check that the origin and provider of the set are remote
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_sub_set;
	if not found then
		raise exception 'Slony-I: subscribeSet(): set % not found', p_sub_set;
	end if;
	if v_set_origin = p_sub_receiver then
		raise exception 
				'Slony-I: subscribeSet(): set origin and receiver cannot be identical';
	end if;
	if p_sub_receiver = p_sub_provider then
		raise exception 
				'Slony-I: subscribeSet(): set provider and receiver cannot be identical';
	end if;
	-- ----
	-- Check that this is called on the origin node
	-- ----
	if v_set_origin != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: subscribeSet() must be called on origin';
	end if;

	-- ---
	-- Verify that the provider is either the origin or an active subscriber
	-- Bug report #1362
	-- ---
	if v_set_origin <> p_sub_provider then
		if not exists (select 1 from @NAMESPACE@.sl_subscribe
			where sub_set = p_sub_set and 
                              sub_receiver = p_sub_provider and
			      sub_forward and sub_active) then
			raise exception 'Slony-I: subscribeSet(): provider % is not an active forwarding node for replication set %', p_sub_provider, p_sub_set;
		end if;
	end if;

	-- ---
	-- Enforce that all sets from one origin are subscribed
	-- using the same data provider per receiver.
	-- ----
	if not exists (select 1 from @NAMESPACE@.sl_subscribe
			where sub_set = p_sub_set and sub_receiver = p_sub_receiver) then
		--
		-- New subscription - error out if we have any other subscription
		-- from that origin with a different data provider.
		--
		for v_rec in select sub_provider from @NAMESPACE@.sl_subscribe
				join @NAMESPACE@.sl_set on set_id = sub_set
				where set_origin = v_set_origin and sub_receiver = p_sub_receiver
		loop
			if v_rec.sub_provider <> p_sub_provider then
				raise exception 'Slony-I: subscribeSet(): wrong provider % - existing subscription from origin % users provider %',
					p_sub_provider, v_set_origin, v_rec.sub_provider;
			end if;
		end loop;
	else
		--
		-- Existing subscription - in case the data provider changes and
		-- there are other subscriptions, warn here. subscribeSet_int()
		-- will currently change the data provider for those sets as well.
		--
		for v_rec in select set_id, sub_provider from @NAMESPACE@.sl_subscribe
				join @NAMESPACE@.sl_set on set_id = sub_set
				where set_origin = v_set_origin and sub_receiver = p_sub_receiver
				and set_id <> p_sub_set
		loop
			if v_rec.sub_provider <> p_sub_provider then
				raise exception 'Slony-I: subscribeSet(): also data provider for set % use resubscribe instead',
					v_rec.set_id;
			end if;
		end loop;
	end if;

	-- ----
	-- Create the SUBSCRIBE_SET event
	-- ----
	v_ev_seqno :=  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SUBSCRIBE_SET', 
			p_sub_set::text, p_sub_provider::text, p_sub_receiver::text, 
			case p_sub_forward when true then 't' else 'f' end,
			case p_omit_copy when true then 't' else 'f' end
                        );

	-- ----
	-- Call the internal procedure to store the subscription
	-- ----
	v_ev_seqno2:=@NAMESPACE@.subscribeSet_int(p_sub_set, p_sub_provider,
			p_sub_receiver, p_sub_forward, p_omit_copy);
	
	if v_ev_seqno2 is not null then
	   v_ev_seqno:=v_ev_seqno2;
	 end if;

	return v_ev_seqno;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.subscribeSet (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4, p_sub_forward bool, p_omit_copy bool) is

'subscribeSet (sub_set, sub_provider, sub_receiver, sub_forward, omit_copy)

Makes sure that the receiver is not the provider, then stores the
subscription, and publishes the SUBSCRIBE_SET event to other nodes.

If omit_copy is true, then no data copy will be done.
';

-- -------------------------------------------------------------------------------------------
-- FUNCTION subscribeSet_int (sub_set, sub_provider, sub_receiver, sub_forward, omit_copy)
-- -------------------------------------------------------------------------------------------
DROP FUNCTION IF EXISTS @NAMESPACE@.subscribeSet_int(int4,int4,int4,bool,bool);
--
-- TODO MONDAY.
--    When this function adds in the subscribe line as a result of a failover
--   it needs the subscription to be enabled so slon pays attention to it.
--   add a parameter to this function for this purpose?
--
--  Also remember to look at the interview questions
create or replace function @NAMESPACE@.subscribeSet_int (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4, p_sub_forward bool, p_omit_copy bool)
returns int4
as $$
declare
	v_set_origin		int4;
	v_sub_row			record;
	v_seq_id			bigint;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Lookup the set origin
	-- ----
	select set_origin into v_set_origin
			from @NAMESPACE@.sl_set
			where set_id = p_sub_set;
	if not found then
		raise exception 'Slony-I: subscribeSet_int(): set % not found', p_sub_set;
	end if;

	-- ----
	-- Provider change is only allowed for active sets
	-- ----
	if p_sub_receiver = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		select sub_active into v_sub_row from @NAMESPACE@.sl_subscribe
				where sub_set = p_sub_set
				and sub_receiver = p_sub_receiver;
		if found then
			if not v_sub_row.sub_active then
				raise exception 'Slony-I: subscribeSet_int(): set % is not active, cannot change provider',
						p_sub_set;
			end if;
		end if;
	end if;

	-- ----
	-- Try to change provider and/or forward for an existing subscription
	-- ----
	update @NAMESPACE@.sl_subscribe
			set sub_provider = p_sub_provider,
				sub_forward = p_sub_forward
			where sub_set = p_sub_set
			and sub_receiver = p_sub_receiver;
	if found then
	  
		-- ----
		-- This is changing a subscriptoin. Make sure all sets from
		-- this origin are subscribed using the same data provider.
		-- For this we first check that the requested data provider
		-- is subscribed to all the sets, the receiver is subscribed to.
		-- ----
		for v_sub_row in select set_id from @NAMESPACE@.sl_set
				join @NAMESPACE@.sl_subscribe on set_id = sub_set
				where set_origin = v_set_origin
				and sub_receiver = p_sub_receiver
				and sub_set <> p_sub_set
		loop
			if not exists (select 1 from @NAMESPACE@.sl_subscribe
					where sub_set = v_sub_row.set_id
					and sub_receiver = p_sub_provider
					and sub_active and sub_forward)
				and not exists (select 1 from @NAMESPACE@.sl_set
					where set_id = v_sub_row.set_id
					and set_origin = p_sub_provider)
			then
				raise exception 'Slony-I: subscribeSet_int(): node % is not a forwarding subscriber for set %',
						p_sub_provider, v_sub_row.set_id;
			end if;

			-- ----
			-- New data provider offers this set as well, change that
			-- subscription too.
			-- ----
			update @NAMESPACE@.sl_subscribe
					set sub_provider = p_sub_provider
					where sub_set = v_sub_row.set_id
					and sub_receiver = p_sub_receiver;
		end loop;

		-- ----
		-- Rewrite sl_listen table
		-- ----
		perform @NAMESPACE@.RebuildListenEntries();

		return p_sub_set;
	end if;

	-- ----
	-- Not found, insert a new one
	-- ----
	if not exists (select true from @NAMESPACE@.sl_path
			where pa_server = p_sub_provider
			and pa_client = p_sub_receiver)
	then
		insert into @NAMESPACE@.sl_path
				(pa_server, pa_client, pa_conninfo, pa_connretry)
				values 
				(p_sub_provider, p_sub_receiver, 
				'<event pending>', 10);
	end if;
	insert into @NAMESPACE@.sl_subscribe
			(sub_set, sub_provider, sub_receiver, sub_forward, sub_active)
			values (p_sub_set, p_sub_provider, p_sub_receiver,
				p_sub_forward, false);

	-- ----
	-- If the set origin is here, then enable the subscription
	-- ----
	if v_set_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		select @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'ENABLE_SUBSCRIPTION', 
				p_sub_set::text, p_sub_provider::text, p_sub_receiver::text, 
				case p_sub_forward when true then 't' else 'f' end,
				case p_omit_copy when true then 't' else 'f' end
				) into v_seq_id;
		perform @NAMESPACE@.enableSubscription(p_sub_set, 
				p_sub_provider, p_sub_receiver);
	end if;
	
	-- ----
	-- Rewrite sl_listen table
	-- ----
	perform @NAMESPACE@.RebuildListenEntries();

	return p_sub_set;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.subscribeSet_int (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4, p_sub_forward bool, p_omit_copy bool) is
'subscribeSet_int (sub_set, sub_provider, sub_receiver, sub_forward, omit_copy)

Internal actions for subscribing receiver sub_receiver to subscription
set sub_set.';


drop function IF EXISTS @NAMESPACE@.unsubscribeSet(int4,int4,boolean);
-- ----------------------------------------------------------------------
-- FUNCTION unsubscribeSet (sub_set, sub_receiver,force)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.unsubscribeSet (p_sub_set int4, p_sub_receiver int4,p_force boolean)
returns bigint
as $$
declare
	v_tab_row			record;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- Check that this is called on the receiver node
	-- ----
	if p_sub_receiver != @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') then
		raise exception 'Slony-I: unsubscribeSet() must be called on receiver';
	end if;



	-- ----
	-- Check that this does not break any chains
	-- ----
	if p_force=false and exists (select true from @NAMESPACE@.sl_subscribe
			 where sub_set = p_sub_set
				and sub_provider = p_sub_receiver)
	then
		raise exception 'Slony-I: Cannot unsubscribe set % while being provider',
				p_sub_set;
	end if;

	if exists (select true from @NAMESPACE@.sl_subscribe
			where sub_set = p_sub_set
				and sub_provider = p_sub_receiver)
	then
		--delete the receivers of this provider.
		--unsubscribeSet_int() will generate the event
		--when it runs on the receiver.
		delete from @NAMESPACE@.sl_subscribe 
			   where sub_set=p_sub_set
			   and sub_provider=p_sub_receiver;
	end if;

	-- ----
	-- Remove the replication triggers.
	-- ----
	for v_tab_row in select tab_id from @NAMESPACE@.sl_table
			where tab_set = p_sub_set
			order by tab_id
	loop
		perform @NAMESPACE@.alterTableDropTriggers(v_tab_row.tab_id);
	end loop;

	-- ----
	-- Remove the setsync status. This will also cause the
	-- worker thread to ignore the set and stop replicating
	-- right now.
	-- ----
	delete from @NAMESPACE@.sl_setsync
			where ssy_setid = p_sub_set;

	-- ----
	-- Remove all sl_table and sl_sequence entries for this set.
	-- Should we ever subscribe again, the initial data
	-- copy process will create new ones.
	-- ----
	delete from @NAMESPACE@.sl_table
			where tab_set = p_sub_set;
	delete from @NAMESPACE@.sl_sequence
			where seq_set = p_sub_set;

	-- ----
	-- Call the internal procedure to drop the subscription
	-- ----
	perform @NAMESPACE@.unsubscribeSet_int(p_sub_set, p_sub_receiver);

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	-- ----
	-- Create the UNSUBSCRIBE_SET event
	-- ----
	return  @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'UNSUBSCRIBE_SET', 
			p_sub_set::text, p_sub_receiver::text);
end;
$$ language plpgsql;
comment on function @NAMESPACE@.unsubscribeSet (p_sub_set int4, p_sub_receiver int4,force boolean) is
'unsubscribeSet (sub_set, sub_receiver,force) 

Unsubscribe node sub_receiver from subscription set sub_set.  This is
invoked on the receiver node.  It verifies that this does not break
any chains (e.g. - where sub_receiver is a provider for another node),
then restores tables, drops Slony-specific keys, drops table entries
for the set, drops the subscription, and generates an UNSUBSCRIBE_SET
node to publish that the node is being dropped.';

-- ----------------------------------------------------------------------
-- FUNCTION unsubscribeSet_int (sub_set, sub_receiver)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.unsubscribeSet_int (p_sub_set int4, p_sub_receiver int4)
returns int4
as $$
declare
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- All the real work is done before event generation on the
	-- subscriber.
	-- ----

	--if this event unsubscribes the provider of this node
	--then this node should unsubscribe itself from the set as well.
	
	if exists (select true from 
		   @NAMESPACE@.sl_subscribe where 
		   sub_set=p_sub_set and sub_provider=p_sub_receiver
		   and sub_receiver=@NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'))
	then
	   perform @NAMESPACE@.unsubscribeSet(p_sub_set,@NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@'),true);
	end if;
	

	delete from @NAMESPACE@.sl_subscribe
			where sub_set = p_sub_set
				and sub_receiver = p_sub_receiver;

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	return p_sub_set;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.unsubscribeSet_int (p_sub_set int4, p_sub_receiver int4) is
'unsubscribeSet_int (sub_set, sub_receiver)

All the REAL work of removing the subscriber is done before the event
is generated, so this function just has to drop the references to the
subscription in sl_subscribe.';

-- ----------------------------------------------------------------------
-- FUNCTION enableSubscription (sub_set, sub_provider, sub_receiver, omit_copy)
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.enableSubscription (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4)
returns int4
as $$
begin
	return  @NAMESPACE@.enableSubscription_int (p_sub_set, 
			p_sub_provider, p_sub_receiver);
end;
$$ language plpgsql;

comment on function @NAMESPACE@.enableSubscription (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4) is 
'enableSubscription (sub_set, sub_provider, sub_receiver)

Indicates that sub_receiver intends subscribing to set sub_set from
sub_provider.  Work is all done by the internal function
enableSubscription_int (sub_set, sub_provider, sub_receiver).';

-- -----------------------------------------------------------------------------------
-- FUNCTION enableSubscription_int (sub_set, sub_provider, sub_receiver)
-- -----------------------------------------------------------------------------------
create or replace function @NAMESPACE@.enableSubscription_int (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4)
returns int4
as $$
declare
	v_n					int4;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- ----
	-- The real work is done in the replication engine. All
	-- we have to do here is remembering that it happened.
	-- ----

	-- ----
	-- Well, not only ... we might be missing an important event here
	-- ----
	if not exists (select true from @NAMESPACE@.sl_path
			where pa_server = p_sub_provider
			and pa_client = p_sub_receiver)
	then
		insert into @NAMESPACE@.sl_path
				(pa_server, pa_client, pa_conninfo, pa_connretry)
				values 
				(p_sub_provider, p_sub_receiver, 
				'<event pending>', 10);
	end if;

	update @NAMESPACE@.sl_subscribe
			set sub_active = 't'
			where sub_set = p_sub_set
			and sub_receiver = p_sub_receiver;
	get diagnostics v_n = row_count;
	if v_n = 0 then
		insert into @NAMESPACE@.sl_subscribe
				(sub_set, sub_provider, sub_receiver,
				sub_forward, sub_active)
				values
				(p_sub_set, p_sub_provider, p_sub_receiver,
				false, true);
	end if;

	-- Rewrite sl_listen table
	perform @NAMESPACE@.RebuildListenEntries();

	return p_sub_set;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.enableSubscription_int (p_sub_set int4, p_sub_provider int4, p_sub_receiver int4) is
'enableSubscription_int (sub_set, sub_provider, sub_receiver)

Internal function to enable subscription of node sub_receiver to set
sub_set via node sub_provider.

slon does most of the work; all we need do here is to remember that it
happened.  The function updates sl_subscribe, indicating that the
subscription has become active.';

-- ----------------------------------------------------------------------
-- FUNCTION forwardConfirm (p_con_origin, p_con_received, p_con_seqno, p_con_timestamp)
--
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.forwardConfirm (p_con_origin int4, p_con_received int4, p_con_seqno int8, p_con_timestamp timestamp)
returns bigint
as $$
declare
	v_max_seqno		bigint;
begin
	select into v_max_seqno coalesce(max(con_seqno), 0)
			from @NAMESPACE@.sl_confirm
			where con_origin = p_con_origin
			and con_received = p_con_received;
	if v_max_seqno < p_con_seqno then
		insert into @NAMESPACE@.sl_confirm 
				(con_origin, con_received, con_seqno, con_timestamp)
				values (p_con_origin, p_con_received, p_con_seqno,
					p_con_timestamp);
		v_max_seqno = p_con_seqno;
	end if;

	return v_max_seqno;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.forwardConfirm (p_con_origin int4, p_con_received int4, p_con_seqno int8, p_con_timestamp timestamp) is
'forwardConfirm (p_con_origin, p_con_received, p_con_seqno, p_con_timestamp)

Confirms (recorded in sl_confirm) that items from p_con_origin up to
p_con_seqno have been received by node p_con_received as of
p_con_timestamp, and raises an event to forward this confirmation.';

-- ----------------------------------------------------------------------
-- FUNCTION cleanupEvent (interval)
--
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.cleanupEvent (p_interval interval)
returns int4
as $$
declare
	v_max_row	record;
	v_min_row	record;
	v_max_sync	int8;
	v_origin	int8;
	v_seqno		int8;
	v_xmin		bigint;
	v_rc            int8;
begin
	-- ----
	-- First remove all confirmations where origin/receiver no longer exist
	-- ----
	delete from @NAMESPACE@.sl_confirm
				where con_origin not in (select no_id from @NAMESPACE@.sl_node);
	delete from @NAMESPACE@.sl_confirm
				where con_received not in (select no_id from @NAMESPACE@.sl_node);
	-- ----
	-- Next remove all but the oldest confirm row per origin,receiver pair.
	-- Ignore confirmations that are younger than 10 minutes. We currently
	-- have an not confirmed suspicion that a possibly lost transaction due
	-- to a server crash might have been visible to another session, and
	-- that this led to log data that is needed again got removed.
	-- ----
	for v_max_row in select con_origin, con_received, max(con_seqno) as con_seqno
				from @NAMESPACE@.sl_confirm
				where con_timestamp < (CURRENT_TIMESTAMP - p_interval)
				group by con_origin, con_received
	loop
		delete from @NAMESPACE@.sl_confirm
				where con_origin = v_max_row.con_origin
				and con_received = v_max_row.con_received
				and con_seqno < v_max_row.con_seqno;
	end loop;

	-- ----
	-- Then remove all events that are confirmed by all nodes in the
	-- whole cluster up to the last SYNC
	-- ----
	for v_min_row in select con_origin, min(con_seqno) as con_seqno
				from @NAMESPACE@.sl_confirm
				group by con_origin
	loop
		select coalesce(max(ev_seqno), 0) into v_max_sync
				from @NAMESPACE@.sl_event
				where ev_origin = v_min_row.con_origin
				and ev_seqno <= v_min_row.con_seqno
				and ev_type = 'SYNC';
		if v_max_sync > 0 then
			delete from @NAMESPACE@.sl_event
					where ev_origin = v_min_row.con_origin
					and ev_seqno < v_max_sync;
		end if;
	end loop;

	-- ----
	-- If cluster has only one node, then remove all events up to
	-- the last SYNC - Bug #1538
        -- http://gborg.postgresql.org/project/slony1/bugs/bugupdate.php?1538
	-- ----

	select * into v_min_row from @NAMESPACE@.sl_node where
			no_id <> @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') limit 1;
	if not found then
		select ev_origin, ev_seqno into v_min_row from @NAMESPACE@.sl_event
		where ev_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@')
		order by ev_origin desc, ev_seqno desc limit 1;
		raise notice 'Slony-I: cleanupEvent(): Single node - deleting events < %', v_min_row.ev_seqno;
			delete from @NAMESPACE@.sl_event
			where
				ev_origin = v_min_row.ev_origin and
				ev_seqno < v_min_row.ev_seqno;

        end if;

	if exists (select * from "pg_catalog".pg_class c, "pg_catalog".pg_namespace n, "pg_catalog".pg_attribute a where c.relname = 'sl_seqlog' and n.oid = c.relnamespace and a.attrelid = c.oid and a.attname = 'oid') then
                execute 'alter table @NAMESPACE@.sl_seqlog set without oids;';
	end if;		
	-- ----
	-- Also remove stale entries from the nodelock table.
	-- ----
	perform @NAMESPACE@.cleanupNodelock();

	-- ----
	-- Find the eldest event left, for each origin
	-- ----
    for v_origin, v_seqno, v_xmin in
	select ev_origin, ev_seqno, "pg_catalog".txid_snapshot_xmin(ev_snapshot) from @NAMESPACE@.sl_event
          where (ev_origin, ev_seqno) in (select ev_origin, min(ev_seqno) from @NAMESPACE@.sl_event where ev_type = 'SYNC' group by ev_origin)
	loop
		delete from @NAMESPACE@.sl_seqlog where seql_origin = v_origin and seql_ev_seqno < v_seqno;
		delete from @NAMESPACE@.sl_log_script where log_origin = v_origin and log_txid < v_xmin;
    end loop;
	
	v_rc := @NAMESPACE@.logswitch_finish();
	if v_rc = 0 then   -- no switch in progress
		perform @NAMESPACE@.logswitch_start();
	end if;

	return 0;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.cleanupEvent (p_interval interval) is 
'cleaning old data out of sl_confirm, sl_event.  Removes all but the
last sl_confirm row per (origin,receiver), and then removes all events
that are confirmed by all nodes in the whole cluster up to the last
SYNC.';


-- ----------------------------------------------------------------------
-- FUNCTION determineIdxnameUnique (tab_fqname, indexname)
--
--	Given a tablename, check that a unique index exists or return
--	the tables primary key index name.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.determineIdxnameUnique(p_tab_fqname text, p_idx_name name) returns name
as $$
declare
	v_tab_fqname_quoted	text default '';
	v_idxrow		record;
begin
	v_tab_fqname_quoted := @NAMESPACE@.slon_quote_input(p_tab_fqname);
	--
	-- Ensure that the table exists
	--
	if (select PGC.relname
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN
				where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
					@NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
					and PGN.oid = PGC.relnamespace) is null then
		raise exception 'Slony-I: determineIdxnameUnique(): table % not found', v_tab_fqname_quoted;
	end if;

	--
	-- Lookup the tables primary key or the specified unique index
	--
	if p_idx_name isnull then
		select PGXC.relname
				into v_idxrow
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN,
					"pg_catalog".pg_index PGX,
					"pg_catalog".pg_class PGXC
				where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
					@NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
					and PGN.oid = PGC.relnamespace
					and PGX.indrelid = PGC.oid
					and PGX.indexrelid = PGXC.oid
					and PGX.indisprimary;
		if not found then
			raise exception 'Slony-I: table % has no primary key',
					v_tab_fqname_quoted;
		end if;
	else
		select PGXC.relname
				into v_idxrow
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN,
					"pg_catalog".pg_index PGX,
					"pg_catalog".pg_class PGXC
				where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
					@NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
					and PGN.oid = PGC.relnamespace
					and PGX.indrelid = PGC.oid
					and PGX.indexrelid = PGXC.oid
					and PGX.indisunique
					and @NAMESPACE@.slon_quote_brute(PGXC.relname) = @NAMESPACE@.slon_quote_input(p_idx_name);
		if not found then
			raise exception 'Slony-I: table % has no unique index %',
					v_tab_fqname_quoted, p_idx_name;
		end if;
	end if;

	--
	-- Return the found index name
	--
	return v_idxrow.relname;
end;
$$ language plpgsql called on null input;
comment on function @NAMESPACE@.determineIdxnameUnique(p_tab_fqname text, p_idx_name name) is
'FUNCTION determineIdxnameUnique (tab_fqname, indexname)

Given a tablename, tab_fqname, check that the unique index, indexname,
exists or return the primary key index name for the table.  If there
is no unique index, it raises an exception.';


-- ----------------------------------------------------------------------
-- FUNCTION determineAttKindUnique (tab_fqname, indexname)
--
--	Given a tablename, return the Slony-I specific attkind (used for
--	the log trigger) of the table. Use the specified unique index or
--	the primary key (if indexname is NULL).
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.determineAttkindUnique(p_tab_fqname text, p_idx_name name) returns text
as $$
declare
	v_tab_fqname_quoted	text default '';
	v_idx_name_quoted	text;
	v_idxrow		record;
	v_attrow		record;
	v_i				integer;
	v_attno			int2;
	v_attkind		text default '';
	v_attfound		bool;
begin
	v_tab_fqname_quoted := @NAMESPACE@.slon_quote_input(p_tab_fqname);
	v_idx_name_quoted := @NAMESPACE@.slon_quote_brute(p_idx_name);
	--
	-- Ensure that the table exists
	--
	if (select PGC.relname
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN
				where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
					@NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
					and PGN.oid = PGC.relnamespace) is null then
		raise exception 'Slony-I: table % not found', v_tab_fqname_quoted;
	end if;

	--
	-- Lookup the tables primary key or the specified unique index
	--
	if p_idx_name isnull then
		raise exception 'Slony-I: index name must be specified';
	else
		select PGXC.relname, PGX.indexrelid, PGX.indkey
				into v_idxrow
				from "pg_catalog".pg_class PGC,
					"pg_catalog".pg_namespace PGN,
					"pg_catalog".pg_index PGX,
					"pg_catalog".pg_class PGXC
				where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
					@NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
					and PGN.oid = PGC.relnamespace
					and PGX.indrelid = PGC.oid
					and PGX.indexrelid = PGXC.oid
					and PGX.indisunique
					and @NAMESPACE@.slon_quote_brute(PGXC.relname) = v_idx_name_quoted;
		if not found then
			raise exception 'Slony-I: table % has no unique index %',
					v_tab_fqname_quoted, v_idx_name_quoted;
		end if;
	end if;

	--
	-- Loop over the tables attributes and check if they are
	-- index attributes. If so, add a "k" to the return value,
	-- otherwise add a "v".
	--
	for v_attrow in select PGA.attnum, PGA.attname
			from "pg_catalog".pg_class PGC,
			    "pg_catalog".pg_namespace PGN,
				"pg_catalog".pg_attribute PGA
			where @NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			    @NAMESPACE@.slon_quote_brute(PGC.relname) = v_tab_fqname_quoted
				and PGN.oid = PGC.relnamespace
				and PGA.attrelid = PGC.oid
				and not PGA.attisdropped
				and PGA.attnum > 0
			order by attnum
	loop
		v_attfound = 'f';

		v_i := 0;
		loop
			select indkey[v_i] into v_attno from "pg_catalog".pg_index
					where indexrelid = v_idxrow.indexrelid;
			if v_attno isnull or v_attno = 0 then
				exit;
			end if;
			if v_attrow.attnum = v_attno then
				v_attfound = 't';
				exit;
			end if;
			v_i := v_i + 1;
		end loop;

		if v_attfound then
			v_attkind := v_attkind || 'k';
		else
			v_attkind := v_attkind || 'v';
		end if;
	end loop;

	-- Strip off trailing v characters as they are not needed by the logtrigger
	v_attkind := pg_catalog.rtrim(v_attkind, 'v');

	--
	-- Return the resulting attkind
	--
	return v_attkind;
end;
$$ language plpgsql called on null input;

comment on function @NAMESPACE@.determineAttkindUnique(p_tab_fqname text, p_idx_name name) is
'determineAttKindUnique (tab_fqname, indexname)

Given a tablename, return the Slony-I specific attkind (used for the
log trigger) of the table. Use the specified unique index or the
primary key (if indexname is NULL).';


-- ----------------------------------------------------------------------
-- FUNCTION RebuildListenEntries ()
--
--	Revises sl_listen rules based on contents of sl_path and
--              sl_subscribe
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.RebuildListenEntries()
returns int
as $$
declare
	v_row	record;
        v_cnt  integer;
begin
	-- ----
	-- Grab the central configuration lock
	-- ----
	lock table @NAMESPACE@.sl_config_lock;

	-- First remove the entire configuration
	delete from @NAMESPACE@.sl_listen;

	-- Second populate the sl_listen configuration with a full
	-- network of all possible paths.
	insert into @NAMESPACE@.sl_listen
				(li_origin, li_provider, li_receiver)
			select pa_server, pa_server, pa_client from @NAMESPACE@.sl_path;
	while true loop
		insert into @NAMESPACE@.sl_listen
					(li_origin, li_provider, li_receiver)
			select distinct li_origin, pa_server, pa_client
				from @NAMESPACE@.sl_listen, @NAMESPACE@.sl_path
				where li_receiver = pa_server
				  and li_origin <> pa_client
				  and pa_conninfo<>'<event pending>'
			except
			select li_origin, li_provider, li_receiver
				from @NAMESPACE@.sl_listen;

		if not found then
			exit;
		end if;
	end loop;

	-- We now replace specific event-origin,receiver combinations
	-- with a configuration that tries to avoid events arriving at
	-- a node before the data provider actually has the data ready.

	-- Loop over every possible pair of receiver and event origin
	for v_row in select N1.no_id as receiver, N2.no_id as origin,
			  N2.no_failed as failed
			from @NAMESPACE@.sl_node as N1, @NAMESPACE@.sl_node as N2
			where N1.no_id <> N2.no_id
	loop
		-- 1st choice:
		-- If we use the event origin as a data provider for any
		-- set that originates on that very node, we are a direct
		-- subscriber to that origin and listen there only.
		if exists (select true from @NAMESPACE@.sl_set, @NAMESPACE@.sl_subscribe				, @NAMESPACE@.sl_node p		   		
				where set_origin = v_row.origin
				  and sub_set = set_id
				  and sub_provider = v_row.origin
				  and sub_receiver = v_row.receiver
				  and sub_active
				  and p.no_active
				  and p.no_id=sub_provider
				  )
		then
			delete from @NAMESPACE@.sl_listen
				where li_origin = v_row.origin
				  and li_receiver = v_row.receiver;
			insert into @NAMESPACE@.sl_listen (li_origin, li_provider, li_receiver)
				values (v_row.origin, v_row.origin, v_row.receiver);
		
		-- 2nd choice:
		-- If we are subscribed to any set originating on this
		-- event origin, we want to listen on all data providers
		-- we use for this origin. We are a cascaded subscriber
		-- for sets from this node.
		else
				if exists (select true from @NAMESPACE@.sl_set, @NAMESPACE@.sl_subscribe,
				   	  	       @NAMESPACE@.sl_node provider
						where set_origin = v_row.origin
						  and sub_set = set_id
						  and sub_provider=provider.no_id
						  and provider.no_failed = false
						  and sub_receiver = v_row.receiver
						  and sub_active)
				then
						delete from @NAMESPACE@.sl_listen
							   where li_origin = v_row.origin
					  		   and li_receiver = v_row.receiver;
						insert into @NAMESPACE@.sl_listen (li_origin, li_provider, li_receiver)
						select distinct set_origin, sub_provider, v_row.receiver
							   from @NAMESPACE@.sl_set, @NAMESPACE@.sl_subscribe
						where set_origin = v_row.origin
						  and sub_set = set_id
						  and sub_receiver = v_row.receiver
						  and sub_active;
				end if;
		end if;

		if v_row.failed then		

		--for every failed node we delete all sl_listen entries
		--except via providers (listed in sl_subscribe)
		--or failover candidates (sl_failover_targets)
		--we do this to prevent a non-failover candidate
		--that is more ahead of the failover candidate from
		--sending events to the failover candidate that
		--are 'too far ahead'

		--if the failed node is not an origin for any
                --node then we don't delete all listen paths
		--for events from it.  Instead we leave
                --the listen network alone.
		
		select count(*) into v_cnt from @NAMESPACE@.sl_subscribe sub,
		       @NAMESPACE@.sl_set s
                       where s.set_origin=v_row.origin and s.set_id=sub.sub_set;
                if v_cnt > 0 then
		    delete from @NAMESPACE@.sl_listen where
			   li_origin=v_row.origin and
			   li_receiver=v_row.receiver			
			   and li_provider not in 
			       (select sub_provider from
			       @NAMESPACE@.sl_subscribe,
			       @NAMESPACE@.sl_set where
			       sub_set=set_id
			       and set_origin=v_row.origin);
		    end if;
               end if;
--		   insert into @NAMESPACE@.sl_listen
--		   		  (li_origin,li_provider,li_receiver)
--				  SELECT v_row.origin, pa_server
--				  ,v_row.receiver
--				  FROM @NAMESPACE@.sl_path where
--				  	   pa_client=v_row.receiver
--				  and (v_row.origin,pa_server,v_row.receiver) not in
--				  	  		(select li_origin,li_provider,li_receiver
--					  		from @NAMESPACE@.sl_listen);
--		end if;
	end loop ;

	return null ;
end ;
$$ language 'plpgsql';

comment on function @NAMESPACE@.RebuildListenEntries() is
'RebuildListenEntries()

Invoked by various subscription and path modifying functions, this
rewrites the sl_listen entries, adding in all the ones required to
allow communications between nodes in the Slony-I cluster.';


-- ----------------------------------------------------------------------
-- FUNCTION generate_sync_event (interval)
--
--	This code can be used to create SYNC events every once in a while
--      even if the 'master' slon daemon is down
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.generate_sync_event(p_interval interval)
returns int4
as $$
declare
	v_node_row     record;

BEGIN
	select 1 into v_node_row from @NAMESPACE@.sl_event 
       	  where ev_type = 'SYNC' and ev_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@')
          and ev_timestamp > now() - p_interval limit 1;
	if not found then
		-- If there has been no SYNC in the last interval, then push one
		perform @NAMESPACE@.createEvent('_@CLUSTERNAME@', 'SYNC', NULL);
		return 1;
	else
		return 0;
	end if;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.generate_sync_event(p_interval interval) is
  'Generate a sync event if there has not been one in the requested interval, and this is a provider node.';

-- ----------------------------------------------------------------------
-- FUNCTION updateRelname (set_id, only_on_node)
--
--      Reset the relnames          
-- ----------------------------------------------------------------------
drop function if exists @NAMESPACE@.updateRelname(int4, int4);
create or replace function @NAMESPACE@.updateRelname ()
returns int4
as $$
declare
        v_no_id                 int4;
        v_set_origin            int4;
begin
        -- ----
        -- Grab the central configuration lock
        -- ----
        lock table @NAMESPACE@.sl_config_lock;

        update @NAMESPACE@.sl_table set 
                tab_relname = PGC.relname, tab_nspname = PGN.nspname
                from pg_catalog.pg_class PGC, pg_catalog.pg_namespace PGN 
                where @NAMESPACE@.sl_table.tab_reloid = PGC.oid
                        and PGC.relnamespace = PGN.oid and
                    (tab_relname <> PGC.relname or tab_nspname <> PGN.nspname);
        update @NAMESPACE@.sl_sequence set
                seq_relname = PGC.relname, seq_nspname = PGN.nspname
                from pg_catalog.pg_class PGC, pg_catalog.pg_namespace PGN
                where @NAMESPACE@.sl_sequence.seq_reloid = PGC.oid
                and PGC.relnamespace = PGN.oid and
 		    (seq_relname <> PGC.relname or seq_nspname <> PGN.nspname);
        return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.updateRelname() is
'updateRelname()';

-- ----------------------------------------------------------------------
-- FUNCTION updateReloid (set_id, only_on_node)
--
--      Reset the relnames
-- ----------------------------------------------------------------------
drop function if exists @NAMESPACE@.updateReloid (int4, int4);
create or replace function @NAMESPACE@.updateReloid (p_set_id int4, p_only_on_node int4)
returns bigint
as $$
declare
        v_no_id                 int4;
        v_set_origin            int4;
	prec			record;
begin
        -- ----
        -- Check that we either are the set origin or a current
        -- subscriber of the set.
        -- ----
        v_no_id := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
        select set_origin into v_set_origin
                        from @NAMESPACE@.sl_set
                        where set_id = p_set_id
                        for update;
        if not found then
                raise exception 'Slony-I: set % not found', p_set_id;
        end if;
        if v_set_origin <> v_no_id
                and not exists (select 1 from @NAMESPACE@.sl_subscribe
                        where sub_set = p_set_id
                        and sub_receiver = v_no_id)
        then
                return 0;
        end if;

        -- ----
        -- If execution on only one node is requested, check that
        -- we are that node.
        -- ----
        if p_only_on_node > 0 and p_only_on_node <> v_no_id then
                return 0;
        end if;

	-- Update OIDs for tables to values pulled from non-table objects in pg_class
	-- This ensures that we won't have collisions when repairing the oids
	for prec in select tab_id from @NAMESPACE@.sl_table loop
		update @NAMESPACE@.sl_table set tab_reloid = (select oid from pg_class pc where relkind <> 'r' and not exists (select 1 from @NAMESPACE@.sl_table t2 where t2.tab_reloid = pc.oid) limit 1)
		where tab_id = prec.tab_id;
	end loop;

	for prec in select tab_id, tab_relname, tab_nspname from @NAMESPACE@.sl_table loop
	        update @NAMESPACE@.sl_table set
        	        tab_reloid = (select PGC.oid
	                from pg_catalog.pg_class PGC, pg_catalog.pg_namespace PGN
	                where @NAMESPACE@.slon_quote_brute(PGC.relname) = @NAMESPACE@.slon_quote_brute(prec.tab_relname)
	                        and PGC.relnamespace = PGN.oid
				and @NAMESPACE@.slon_quote_brute(PGN.nspname) = @NAMESPACE@.slon_quote_brute(prec.tab_nspname))
		where tab_id = prec.tab_id;
	end loop;

	for prec in select seq_id from @NAMESPACE@.sl_sequence loop
		update @NAMESPACE@.sl_sequence set seq_reloid = (select oid from pg_class pc where relkind <> 'S' and not exists (select 1 from @NAMESPACE@.sl_sequence t2 where t2.seq_reloid = pc.oid) limit 1)
		where seq_id = prec.seq_id;
	end loop;

	for prec in select seq_id, seq_relname, seq_nspname from @NAMESPACE@.sl_sequence loop
	        update @NAMESPACE@.sl_sequence set
	                seq_reloid = (select PGC.oid
	                from pg_catalog.pg_class PGC, pg_catalog.pg_namespace PGN
	                where @NAMESPACE@.slon_quote_brute(PGC.relname) = @NAMESPACE@.slon_quote_brute(prec.seq_relname)
                	and PGC.relnamespace = PGN.oid
			and @NAMESPACE@.slon_quote_brute(PGN.nspname) = @NAMESPACE@.slon_quote_brute(prec.seq_nspname))
		where seq_id = prec.seq_id;
	end loop;

	return 1;
end;
$$ language plpgsql;
comment on function @NAMESPACE@.updateReloid(p_set_id int4, p_only_on_node int4) is
'updateReloid(set_id, only_on_node)

Updates the respective reloids in sl_table and sl_seqeunce based on
their respective FQN';


-- ----------------------------------------------------------------------
-- FUNCTION logswitch_start()
--
--	Called by slonik to initiate a switch from sl_log_1 to sl_log_2 and
--  visa versa.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logswitch_start()
returns int4 as $$
DECLARE
	v_current_status	int4;
BEGIN
	-- ----
	-- Get the current log status.
	-- ----
	select last_value into v_current_status from @NAMESPACE@.sl_log_status;

	-- ----
	-- status = 0: sl_log_1 active, sl_log_2 clean
	-- Initiate a switch to sl_log_2.
	-- ----
	if v_current_status = 0 then
		perform "pg_catalog".setval('@NAMESPACE@.sl_log_status', 3);
		perform @NAMESPACE@.registry_set_timestamp(
				'logswitch.laststart', now());
		raise notice 'Slony-I: Logswitch to sl_log_2 initiated';
		return 2;
	end if;

	-- ----
	-- status = 1: sl_log_2 active, sl_log_1 clean
	-- Initiate a switch to sl_log_1.
	-- ----
	if v_current_status = 1 then
		perform "pg_catalog".setval('@NAMESPACE@.sl_log_status', 2);
		perform @NAMESPACE@.registry_set_timestamp(
				'logswitch.laststart', now());
		raise notice 'Slony-I: Logswitch to sl_log_1 initiated';
		return 1;
	end if;

	raise exception 'Previous logswitch still in progress';
END;
$$ language plpgsql;
comment on function @NAMESPACE@.logswitch_start() is
'logswitch_start()

Initiate a log table switch if none is in progress';

-- ----------------------------------------------------------------------
-- FUNCTION logswitch_finish()
--
--	Called from the cleanup thread to eventually finish a logswitch
--  that is in progress.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.logswitch_finish()
returns int4 as $$
DECLARE
	v_current_status	int4;
	v_dummy				record;
	v_origin	int8;
	v_seqno		int8;
	v_xmin		bigint;
	v_purgeable boolean;
BEGIN
	-- ----
	-- Get the current log status.
	-- ----
	select last_value into v_current_status from @NAMESPACE@.sl_log_status;

	-- ----
	-- status value 0 or 1 means that there is no log switch in progress
	-- ----
	if v_current_status = 0 or v_current_status = 1 then
		return 0;
	end if;

	-- ----
	-- status = 2: sl_log_1 active, cleanup sl_log_2
	-- ----
	if v_current_status = 2 then
		v_purgeable := 'true';
		
		-- ----
		-- Attempt to lock sl_log_2 in order to make sure there are no other transactions 
		-- currently writing to it. Exit if it is still in use. This prevents TRUNCATE from 
		-- blocking writers to sl_log_2 while it is waiting for a lock. It also prevents it 
		-- immediately truncating log data generated inside the transaction which was active 
		-- when logswitch_finish() was called (and was blocking TRUNCATE) as soon as that 
		-- transaction is committed.
		-- ----
		begin
			lock table @NAMESPACE@.sl_log_2 in access exclusive mode nowait;
		exception when lock_not_available then
			raise notice 'Slony-I: could not lock sl_log_2 - sl_log_2 not truncated';
			return -1;
		end;

		-- ----
		-- The cleanup thread calls us after it did the delete and
		-- vacuum of both log tables. If sl_log_2 is empty now, we
		-- can truncate it and the log switch is done.
		-- ----
	        for v_origin, v_seqno, v_xmin in
		  select ev_origin, ev_seqno, "pg_catalog".txid_snapshot_xmin(ev_snapshot) from @NAMESPACE@.sl_event
	          where (ev_origin, ev_seqno) in (select ev_origin, min(ev_seqno) from @NAMESPACE@.sl_event where ev_type = 'SYNC' group by ev_origin)
		loop
			if exists (select 1 from @NAMESPACE@.sl_log_2 where log_origin = v_origin and log_txid >= v_xmin limit 1) then
				v_purgeable := 'false';
			end if;
	        end loop;
		if not v_purgeable then
			-- ----
			-- Found a row ... log switch is still in progress.
			-- ----
			raise notice 'Slony-I: log switch to sl_log_1 still in progress - sl_log_2 not truncated';
			return -1;
		end if;

		raise notice 'Slony-I: log switch to sl_log_1 complete - truncate sl_log_2';
		truncate @NAMESPACE@.sl_log_2;
		if exists (select * from "pg_catalog".pg_class c, "pg_catalog".pg_namespace n, "pg_catalog".pg_attribute a where c.relname = 'sl_log_2' and n.oid = c.relnamespace and a.attrelid = c.oid and a.attname = 'oid') then
	                execute 'alter table @NAMESPACE@.sl_log_2 set without oids;';
		end if;		
		perform "pg_catalog".setval('@NAMESPACE@.sl_log_status', 0);
		-- Run addPartialLogIndices() to try to add indices to unused sl_log_? table
		perform @NAMESPACE@.addPartialLogIndices();

		return 1;
	end if;

	-- ----
	-- status = 3: sl_log_2 active, cleanup sl_log_1
	-- ----
	if v_current_status = 3 then
		v_purgeable := 'true';

		-- ----
		-- Attempt to lock sl_log_1 in order to make sure there are no other transactions 
		-- currently writing to it. Exit if it is still in use. This prevents TRUNCATE from 
		-- blocking writes to sl_log_1 while it is waiting for a lock. It also prevents it 
		-- immediately truncating log data generated inside the transaction which was active 
		-- when logswitch_finish() was called (and was blocking TRUNCATE) as soon as that 
		-- transaction is committed.
		-- ----
		begin
			lock table @NAMESPACE@.sl_log_1 in access exclusive mode nowait;
		exception when lock_not_available then
			raise notice 'Slony-I: could not lock sl_log_1 - sl_log_1 not truncated';
			return -1;
		end;

		-- ----
		-- The cleanup thread calls us after it did the delete and
		-- vacuum of both log tables. If sl_log_2 is empty now, we
		-- can truncate it and the log switch is done.
		-- ----
	        for v_origin, v_seqno, v_xmin in
		  select ev_origin, ev_seqno, "pg_catalog".txid_snapshot_xmin(ev_snapshot) from @NAMESPACE@.sl_event
	          where (ev_origin, ev_seqno) in (select ev_origin, min(ev_seqno) from @NAMESPACE@.sl_event where ev_type = 'SYNC' group by ev_origin)
		loop
			if (exists (select 1 from @NAMESPACE@.sl_log_1 where log_origin = v_origin and log_txid >= v_xmin limit 1)) then
				v_purgeable := 'false';
			end if;
	        end loop;
		if not v_purgeable then
			-- ----
			-- Found a row ... log switch is still in progress.
			-- ----
			raise notice 'Slony-I: log switch to sl_log_2 still in progress - sl_log_1 not truncated';
			return -1;
		end if;

		raise notice 'Slony-I: log switch to sl_log_2 complete - truncate sl_log_1';
		truncate @NAMESPACE@.sl_log_1;
		if exists (select * from "pg_catalog".pg_class c, "pg_catalog".pg_namespace n, "pg_catalog".pg_attribute a where c.relname = 'sl_log_1' and n.oid = c.relnamespace and a.attrelid = c.oid and a.attname = 'oid') then
	                execute 'alter table @NAMESPACE@.sl_log_1 set without oids;';
		end if;		
		perform "pg_catalog".setval('@NAMESPACE@.sl_log_status', 1);
		-- Run addPartialLogIndices() to try to add indices to unused sl_log_? table
		perform @NAMESPACE@.addPartialLogIndices();
		return 2;
	end if;
END;
$$ language plpgsql;
comment on function @NAMESPACE@.logswitch_finish() is
'logswitch_finish()

Attempt to finalize a log table switch in progress
return values:
  -1 if switch in progress, but not complete
   0 if no switch in progress
   1 if performed truncate on sl_log_2
   2 if performed truncate on sl_log_1
';


-- ----------------------------------------------------------------------
-- FUNCTION addPartialLogIndices ()
-- Add partial indices to sl_log_? tables that aren't currently in use
-- ----------------------------------------------------------------------

create or replace function @NAMESPACE@.addPartialLogIndices () returns integer as $$
DECLARE
	v_current_status	int4;
	v_log			int4;
	v_dummy		record;
	v_dummy2	record;
	idef 		text;
	v_count		int4;
        v_iname         text;
	v_ilen int4;
	v_maxlen int4;
BEGIN
	v_count := 0;
	select last_value into v_current_status from @NAMESPACE@.sl_log_status;

	-- If status is 2 or 3 --> in process of cleanup --> unsafe to create indices
	if v_current_status in (2, 3) then
		return 0;
	end if;

	if v_current_status = 0 then   -- Which log should get indices?
		v_log := 2;
	else
		v_log := 1;
	end if;
--                                       PartInd_test_db_sl_log_2-node-1
	-- Add missing indices...
	for v_dummy in select distinct set_origin from @NAMESPACE@.sl_set loop
            v_iname := 'PartInd_@CLUSTERNAME@_sl_log_' || v_log::text || '-node-' 
			|| v_dummy.set_origin::text;
	   -- raise notice 'Consider adding partial index % on sl_log_%', v_iname, v_log;
	   -- raise notice 'schema: [_@CLUSTERNAME@] tablename:[sl_log_%]', v_log;
            select * into v_dummy2 from pg_catalog.pg_indexes where tablename = 'sl_log_' || v_log::text and  indexname = v_iname;
            if not found then
		-- raise notice 'index was not found - add it!';
        v_iname := 'PartInd_@CLUSTERNAME@_sl_log_' || v_log::text || '-node-' || v_dummy.set_origin::text;
		v_ilen := pg_catalog.length(v_iname);
		v_maxlen := pg_catalog.current_setting('max_identifier_length'::text)::int4;
                if v_ilen > v_maxlen then
		   raise exception 'Length of proposed index name [%] > max_identifier_length [%] - cluster name probably too long', v_ilen, v_maxlen;
		end if;

		idef := 'create index "' || v_iname || 
                        '" on @NAMESPACE@.sl_log_' || v_log::text || ' USING btree(log_txid) where (log_origin = ' || v_dummy.set_origin::text || ');';
		execute idef;
		v_count := v_count + 1;
            else
                -- raise notice 'Index % already present - skipping', v_iname;
            end if;
	end loop;

	-- Remove unneeded indices...
	for v_dummy in select indexname from pg_catalog.pg_indexes i where i.tablename = 'sl_log_' || v_log::text and
                       i.indexname like ('PartInd_@CLUSTERNAME@_sl_log_' || v_log::text || '-node-%') and
                       not exists (select 1 from @NAMESPACE@.sl_set where
				i.indexname = 'PartInd_@CLUSTERNAME@_sl_log_' || v_log::text || '-node-' || set_origin::text)
	loop
		-- raise notice 'Dropping obsolete index %d', v_dummy.indexname;
		idef := 'drop index @NAMESPACE@."' || v_dummy.indexname || '";';
		execute idef;
		v_count := v_count - 1;
	end loop;
	return v_count;
END
$$ language plpgsql;


comment on function @NAMESPACE@.addPartialLogIndices () is 
'Add partial indexes, if possible, to the unused sl_log_? table for
all origin nodes, and drop any that are no longer needed.

This function presently gets run any time set origins are manipulated
(FAILOVER, STORE SET, MOVE SET, DROP SET), as well as each time the
system switches between sl_log_1 and sl_log_2.';


-- ----------------------------------------------------------------------
-- FUNCTION upgradeSchema(old_version)
-- upgrade sl_node
--
--	Called by slonik during the function upgrade process. 
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.check_table_field_exists (p_namespace text, p_table text, p_field text)
returns bool as $$
BEGIN
	return exists (
			select 1 from "information_schema".columns
				where table_schema = p_namespace
				and table_name = p_table
				and column_name = p_field
		);
END;$$ language plpgsql;

comment on function @NAMESPACE@.check_table_field_exists (p_namespace text, p_table text, p_field text)
is 'Check if a table has a specific attribute';

create or replace function @NAMESPACE@.add_missing_table_field (p_namespace text, p_table text, p_field text, p_type text) 
returns bool as $$
DECLARE
  v_row       record;
  v_query     text;
BEGIN
  if not @NAMESPACE@.check_table_field_exists(p_namespace, p_table, p_field) then
    raise notice 'Upgrade table %.% - add field %', p_namespace, p_table, p_field;
    v_query := 'alter table ' || p_namespace || '.' || p_table || ' add column ';
    v_query := v_query || p_field || ' ' || p_type || ';';
    execute v_query;
    return 't';
  else
    return 'f';
  end if;
END;$$ language plpgsql;

comment on function @NAMESPACE@.add_missing_table_field (p_namespace text, p_table text, p_field text, p_type text)
is 'Add a column of a given type to a table if it is missing';

create or replace function @NAMESPACE@.upgradeSchema(p_old text)
returns text as $$
declare
	v_tab_row	record;
	v_query text;
	v_keepstatus text;
begin
	-- If old version is pre-2.0, then we require a special upgrade process
	if p_old like '1.%' then
		raise exception 'Upgrading to Slony-I 2.x requires running slony_upgrade_20';
	end if;

	perform @NAMESPACE@.upgradeSchemaAddTruncateTriggers();

	-- Change all Slony-I-defined columns that are "timestamp without time zone" to "timestamp *WITH* time zone"
	if exists (select 1 from information_schema.columns c
            where table_schema = '_@CLUSTERNAME@' and data_type = 'timestamp without time zone'
	    and exists (select 1 from information_schema.tables t where t.table_schema = c.table_schema and t.table_name = c.table_name and t.table_type = 'BASE TABLE')
		and (c.table_name, c.column_name) in (('sl_confirm', 'con_timestamp'), ('sl_event', 'ev_timestamp'), ('sl_registry', 'reg_timestamp'),('sl_archive_counter', 'ac_timestamp')))
	then

	  -- Preserve sl_status
	  select pg_get_viewdef('@NAMESPACE@.sl_status') into v_keepstatus;
	  execute 'drop view sl_status';
	  for v_tab_row in select table_schema, table_name, column_name from information_schema.columns c
            where table_schema = '_@CLUSTERNAME@' and data_type = 'timestamp without time zone'
	    and exists (select 1 from information_schema.tables t where t.table_schema = c.table_schema and t.table_name = c.table_name and t.table_type = 'BASE TABLE')
		and (table_name, column_name) in (('sl_confirm', 'con_timestamp'), ('sl_event', 'ev_timestamp'), ('sl_registry', 'reg_timestamp'),('sl_archive_counter', 'ac_timestamp'))
	  loop
		raise notice 'Changing Slony-I column [%.%] to timestamp WITH time zone', v_tab_row.table_name, v_tab_row.column_name;
		v_query := 'alter table ' || @NAMESPACE@.slon_quote_brute(v_tab_row.table_schema) ||
                   '.' || v_tab_row.table_name || ' alter column ' || v_tab_row.column_name ||
                   ' type timestamp with time zone;';
		execute v_query;
	  end loop;
	  -- restore sl_status
	  execute 'create view sl_status as ' || v_keepstatus;
        end if;

	if not exists (select 1 from information_schema.tables where table_schema = '_@CLUSTERNAME@' and table_name = 'sl_components') then
	   v_query := '
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
';
  	   execute v_query;
	end if;

	



	if not exists (select 1 from information_schema.tables t where table_schema = '_@CLUSTERNAME@' and table_name = 'sl_event_lock') then
	   v_query := 'create table @NAMESPACE@.sl_event_lock (dummy integer);';
	   execute v_query;
        end if;
	
	if not exists (select 1 from information_schema.tables t 
			where table_schema = '_@CLUSTERNAME@' 
			and table_name = 'sl_apply_stats') then
		v_query := '
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
			) WITHOUT OIDS;';
		execute v_query;
	end if;
	
	--
	-- On the upgrade to 2.2, we change the layout of sl_log_N by
	-- adding columns log_tablenspname, log_tablerelname, and
	-- log_cmdupdncols as well as changing log_cmddata into
	-- log_cmdargs, which is a text array.
	--
	if not @NAMESPACE@.check_table_field_exists('_@CLUSTERNAME@', 'sl_log_1', 'log_cmdargs') then
		--
		-- Check that the cluster is completely caught up
		--
		if @NAMESPACE@.check_unconfirmed_log() then
			raise EXCEPTION 'cannot upgrade to new sl_log_N format due to existing unreplicated data';
		end if;

		--
		-- Drop tables sl_log_1 and sl_log_2
		--
		drop table @NAMESPACE@.sl_log_1;
		drop table @NAMESPACE@.sl_log_2;

		--
		-- Create the new sl_log_1
		--
		create table @NAMESPACE@.sl_log_1 (
			log_origin          int4,
			log_txid            bigint,
			log_tableid         int4,
			log_actionseq       int8,
			log_tablenspname    text,
			log_tablerelname    text,
			log_cmdtype         "char",
			log_cmdupdncols     int4,
			log_cmdargs         text[]
		) without oids;
		create index sl_log_1_idx1 on @NAMESPACE@.sl_log_1
			(log_origin, log_txid, log_actionseq);

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

		--
		-- Create the new sl_log_2
		--
		create table @NAMESPACE@.sl_log_2 (
			log_origin          int4,
			log_txid            bigint,
			log_tableid         int4,
			log_actionseq       int8,
			log_tablenspname    text,
			log_tablerelname    text,
			log_cmdtype         "char",
			log_cmdupdncols     int4,
			log_cmdargs         text[]
		) without oids;
		create index sl_log_2_idx1 on @NAMESPACE@.sl_log_2
			(log_origin, log_txid, log_actionseq);

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

		--
		-- Put the log apply triggers back onto sl_log_1/2
		--
		create trigger apply_trigger
			before INSERT on @NAMESPACE@.sl_log_1
			for each row execute procedure @NAMESPACE@.logApply('_@CLUSTERNAME@');
		alter table @NAMESPACE@.sl_log_1
			enable replica trigger apply_trigger;
		create trigger apply_trigger
			before INSERT on @NAMESPACE@.sl_log_2
			for each row execute procedure @NAMESPACE@.logApply('_@CLUSTERNAME@');
		alter table @NAMESPACE@.sl_log_2
			enable replica trigger apply_trigger;
	end if;
	if not exists (select 1 from information_schema.routines where routine_schema = '_@CLUSTERNAME@' and routine_name = 'string_agg') then
	       CREATE AGGREGATE @NAMESPACE@.string_agg(text) (
	   	      SFUNC=@NAMESPACE@.agg_text_sum,
		      STYPE=text,
		      INITCOND=''
		      );
	end if;
	if not exists (select 1 from information_schema.views where table_schema='_@CLUSTERNAME@' and table_name='sl_failover_targets') then
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
	end if;

	if not @NAMESPACE@.check_table_field_exists('_@CLUSTERNAME@', 'sl_node', 'no_failed') then
	   alter table @NAMESPACE@.sl_node add column no_failed bool;
	   update @NAMESPACE@.sl_node set no_failed=false;
	end if;
	return p_old;
end;
$$ language plpgsql;

create or replace function @NAMESPACE@.check_unconfirmed_log ()
returns bool as $$
declare
	v_rc		bool = false;
	v_error		bool = false;
	v_origin	integer;
	v_allconf	bigint;
	v_allsnap	txid_snapshot;
	v_count		bigint;
begin
	--
	-- Loop over all nodes that are the origin of at least one set
	--
	for v_origin in select distinct set_origin as no_id
			from @NAMESPACE@.sl_set loop
		--
		-- Per origin determine which is the highest event seqno
		-- that is confirmed by all subscribers to any of the
		-- origins sets.
		--
		select into v_allconf min(max_seqno) from (
				select con_received, max(con_seqno) as max_seqno
					from @NAMESPACE@.sl_confirm
					where con_origin = v_origin
					and con_received in (
						select distinct sub_receiver
							from @NAMESPACE@.sl_set as SET,
								@NAMESPACE@.sl_subscribe as SUB
							where SET.set_id = SUB.sub_set
							and SET.set_origin = v_origin
						)
					group by con_received
			) as maxconfirmed;
		if not found then
			raise NOTICE 'check_unconfirmed_log(): cannot determine highest ev_seqno for node % confirmed by all subscribers', v_origin;
			v_error = true;
			continue;
		end if;

		--
		-- Get the txid snapshot that corresponds with that event
		--
		select into v_allsnap ev_snapshot
			from @NAMESPACE@.sl_event
			where ev_origin = v_origin
			and ev_seqno = v_allconf;
		if not found then
			raise NOTICE 'check_unconfirmed_log(): cannot find event %,% in sl_event', v_origin, v_allconf;
			v_error = true;
			continue;
		end if;

		--
		-- Count the number of log rows that appeard after that event.
		--
		select into v_count count(*) from (
			select 1 from @NAMESPACE@.sl_log_1
				where log_origin = v_origin
				and log_txid >= "pg_catalog".txid_snapshot_xmax(v_allsnap)
			union all
			select 1 from @NAMESPACE@.sl_log_1
				where log_origin = v_origin
				and log_txid in (
					select * from "pg_catalog".txid_snapshot_xip(v_allsnap)
				)
			union all
			select 1 from @NAMESPACE@.sl_log_2
				where log_origin = v_origin
				and log_txid >= "pg_catalog".txid_snapshot_xmax(v_allsnap)
			union all
			select 1 from @NAMESPACE@.sl_log_2
				where log_origin = v_origin
				and log_txid in (
					select * from "pg_catalog".txid_snapshot_xip(v_allsnap)
				)
		) as cnt;

		if v_count > 0 then
			raise NOTICE 'check_unconfirmed_log(): origin % has % log rows that have not propagated to all subscribers yet', v_origin, v_count;
			v_rc = true;
		end if;
	end loop;

	if v_error then
		raise EXCEPTION 'check_unconfirmed_log(): aborting due to previous inconsistency';
	end if;

	return v_rc;
end;
$$ language plpgsql;

--
-- XXX What is this doing here?
--
set search_path to @NAMESPACE@
;

comment on function @NAMESPACE@.upgradeSchema(p_old text) is
    'Called during "update functions" by slonik to perform schema changes';

-- ----------------------------------------------------------------------
-- VIEW sl_status
--
--	This view shows the local nodes last event sequence number
--	and how far all remote nodes have processed events.
--
--	This view can NOT be loaded in slony1_base.sql (where it
--	naturally would belong) because of using a C function that
--	is defined in this file.
-- ----------------------------------------------------------------------
create or replace view @NAMESPACE@.sl_status as select
	E.ev_origin as st_origin,
	C.con_received as st_received,
	E.ev_seqno as st_last_event,
	E.ev_timestamp as st_last_event_ts,
	C.con_seqno as st_last_received,
	C.con_timestamp as st_last_received_ts,
	CE.ev_timestamp as st_last_received_event_ts,
	E.ev_seqno - C.con_seqno as st_lag_num_events,
	current_timestamp - CE.ev_timestamp as st_lag_time
	from @NAMESPACE@.sl_event E, @NAMESPACE@.sl_confirm C,
		@NAMESPACE@.sl_event CE
	where E.ev_origin = C.con_origin
	and CE.ev_origin = E.ev_origin
	and CE.ev_seqno = C.con_seqno
	and (E.ev_origin, E.ev_seqno) in 
		(select ev_origin, max(ev_seqno)
			from @NAMESPACE@.sl_event
			where ev_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@')
			group by 1
		)
	and (C.con_origin, C.con_received, C.con_seqno) in
		(select con_origin, con_received, max(con_seqno)
			from @NAMESPACE@.sl_confirm
			where con_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@')
			group by 1, 2
		);

comment on view @NAMESPACE@.sl_status is 'View showing how far behind remote nodes are.';

create or replace function @NAMESPACE@.copyFields(p_tab_id integer) 
returns text
as $$
declare
	result text;
	prefix text;
	prec record;
begin
	result := '';
	prefix := '(';   -- Initially, prefix is the opening paren

	for prec in select @NAMESPACE@.slon_quote_input(a.attname) as column from @NAMESPACE@.sl_table t, pg_catalog.pg_attribute a where t.tab_id = p_tab_id and t.tab_reloid = a.attrelid and a.attnum > 0 and a.attisdropped = false order by attnum
	loop
		result := result || prefix || prec.column;
		prefix := ',';   -- Subsequently, prepend columns with commas
	end loop;
	result := result || ')';
	return result;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.copyFields(p_tab_id integer) is
'Return a string consisting of what should be appended to a COPY statement
to specify fields for the passed-in tab_id.  

In PG versions > 7.3, this looks like (field1,field2,...fieldn)';

-- ----------------------------------------------------------------------
-- FUNCTION prepareTableForCopy(tab_id)
--
--	Remove all content from a table before the subscription
--	content is loaded via COPY and disable index maintenance.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.prepareTableForCopy(p_tab_id int4)
returns int4
as $$
declare
	v_tab_oid		oid;
	v_tab_fqname	text;
begin
	-- ----
	-- Get the OID and fully qualified name for the table
	-- ---
	select	PGC.oid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
		into v_tab_oid, v_tab_fqname
			from @NAMESPACE@.sl_table T,   
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
				where T.tab_id = p_tab_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid;
	if not found then
		raise exception 'Table with ID % not found in sl_table', p_tab_id;
	end if;

	-- ----
	-- Try using truncate to empty the table and fallback to
	-- delete on error.
	-- ----
	perform @NAMESPACE@.TruncateOnlyTable(v_tab_fqname);
	raise notice 'truncate of % succeeded', v_tab_fqname;

	-- suppress index activity
        perform @NAMESPACE@.disable_indexes_on_table(v_tab_oid);

	return 1;
	exception when others then
		raise notice 'truncate of % failed - doing delete', v_tab_fqname;
		perform @NAMESPACE@.disable_indexes_on_table(v_tab_oid);
		execute 'delete from only ' || @NAMESPACE@.slon_quote_input(v_tab_fqname);
		return 0;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.prepareTableForCopy(p_tab_id int4) is
'Delete all data and suppress index maintenance';

-- ----------------------------------------------------------------------
-- FUNCTION finishTableAfterCopy(tab_id)
--
--	Reenable index maintenance and reindex the table after COPY.
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.finishTableAfterCopy(p_tab_id int4)
returns int4
as $$
declare
	v_tab_oid		oid;
	v_tab_fqname	text;
begin
	-- ----
	-- Get the tables OID and fully qualified name
	-- ---
	select	PGC.oid,
			@NAMESPACE@.slon_quote_brute(PGN.nspname) || '.' ||
			@NAMESPACE@.slon_quote_brute(PGC.relname) as tab_fqname
		into v_tab_oid, v_tab_fqname
			from @NAMESPACE@.sl_table T,   
				"pg_catalog".pg_class PGC, "pg_catalog".pg_namespace PGN
				where T.tab_id = p_tab_id
				and T.tab_reloid = PGC.oid
				and PGC.relnamespace = PGN.oid;
	if not found then
		raise exception 'Table with ID % not found in sl_table', p_tab_id;
	end if;

	-- ----
	-- Reenable indexes and reindex the table.
	-- ----
	perform @NAMESPACE@.enable_indexes_on_table(v_tab_oid);
	execute 'reindex table ' || @NAMESPACE@.slon_quote_input(v_tab_fqname);

	return 1;
end;
$$ language plpgsql;

comment on function @NAMESPACE@.finishTableAfterCopy(p_tab_id int4) is
'Reenable index maintenance and reindex the table';

create or replace function @NAMESPACE@.setup_vactables_type () returns integer as $$
begin
	if not exists (select 1 from pg_catalog.pg_type t, pg_catalog.pg_namespace n
			where n.nspname = '_@CLUSTERNAME@' and t.typnamespace = n.oid and
			t.typname = 'vactables') then
		execute 'create type @NAMESPACE@.vactables as (nspname name, relname name);';
	end if;
	return 1;
end
$$ language plpgsql;

comment on function @NAMESPACE@.setup_vactables_type () is 
'Function to be run as part of loading slony1_funcs.sql that creates the vactables type if it is missing';

select @NAMESPACE@.setup_vactables_type();

drop function @NAMESPACE@.setup_vactables_type ();

-- ----------------------------------------------------------------------
-- FUNCTION TablesToVacuum()
--
--	Make function be STRICT
-- ----------------------------------------------------------------------
create or replace function @NAMESPACE@.TablesToVacuum () returns setof @NAMESPACE@.vactables as $$
declare
	prec @NAMESPACE@.vactables%rowtype;
begin
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_event';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_confirm';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_setsync';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_seqlog';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_archive_counter';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_components';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := '_@CLUSTERNAME@';
	prec.relname := 'sl_log_script';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := 'pg_catalog';
	prec.relname := 'pg_listener';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;
	prec.nspname := 'pg_catalog';
	prec.relname := 'pg_statistic';
	if @NAMESPACE@.ShouldSlonyVacuumTable(prec.nspname, prec.relname) then
		return next prec;
	end if;

   return;
end
$$ language plpgsql;

comment on function @NAMESPACE@.TablesToVacuum () is 
'Return a list of tables that require frequent vacuuming.  The
function is used so that the list is not hardcoded into C code.';

-- -------------------------------------------------------------------------
-- FUNCTION add_empty_table_to_replication (set_id, tab_id, tab_nspname,
--         tab_tabname, tab_idxname, tab_comment)
-- -------------------------------------------------------------------------
create or replace function @NAMESPACE@.add_empty_table_to_replication(p_set_id int4, p_tab_id int4, p_nspname text, p_tabname text, p_idxname text, p_comment text) returns bigint as $$
declare

  prec record;
  v_origin int4;
  v_isorigin boolean;
  v_fqname text;
  v_query text;
  v_rows integer;
  v_idxname text;

begin
-- Need to validate that the set exists; the set will tell us if this is the origin
  select set_origin into v_origin from @NAMESPACE@.sl_set where set_id = p_set_id;
  if not found then
	raise exception 'add_empty_table_to_replication: set % not found!', p_set_id;
  end if;

-- Need to be aware of whether or not this node is origin for the set
   v_isorigin := ( v_origin = @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') );

   v_fqname := '"' || p_nspname || '"."' || p_tabname || '"';
-- Take out a lock on the table
   v_query := 'lock ' || v_fqname || ';';
   execute v_query;

   if v_isorigin then
	-- On the origin, verify that the table is empty, failing if it has any tuples
        v_query := 'select 1 as tuple from ' || v_fqname || ' limit 1;';
	execute v_query into prec;
        GET DIAGNOSTICS v_rows = ROW_COUNT;
	if v_rows = 0 then
		raise notice 'add_empty_table_to_replication: table % empty on origin - OK', v_fqname;
	else
		raise exception 'add_empty_table_to_replication: table % contained tuples on origin node %', v_fqname, v_origin;
	end if;
   else
	-- On other nodes, TRUNCATE the table
        v_query := 'truncate ' || v_fqname || ';';
	execute v_query;
   end if;
-- If p_idxname is NULL, then look up the PK index, and RAISE EXCEPTION if one does not exist
   if p_idxname is NULL then
	select c2.relname into prec from pg_catalog.pg_index i, pg_catalog.pg_class c1, pg_catalog.pg_class c2, pg_catalog.pg_namespace n where i.indrelid = c1.oid and i.indexrelid = c2.oid and c1.relname = p_tabname and i.indisprimary and n.nspname = p_nspname and n.oid = c1.relnamespace;
	if not found then
		raise exception 'add_empty_table_to_replication: table % has no primary key and no candidate specified!', v_fqname;
	else
		v_idxname := prec.relname;
	end if;
   else
	v_idxname := p_idxname;
   end if;
   return @NAMESPACE@.setAddTable_int(p_set_id, p_tab_id, v_fqname, v_idxname, p_comment);
end
$$ language plpgsql;

comment on function @NAMESPACE@.add_empty_table_to_replication(p_set_id int4, p_tab_id int4, p_nspname text, p_tabname text, p_idxname text, p_comment text) is
'Verify that a table is empty, and add it to replication.  
tab_idxname is optional - if NULL, then we use the primary key.

Note that this function is to be run within an EXECUTE SCRIPT script,
so it runs at the right place in the transaction stream on all
nodes.';

-- -------------------------------------------------------------------------
-- FUNCTION replicate_partition (tab_id, tab_nspname, tab_tabname,
--         tab_idxname, tab_comment)
-- -------------------------------------------------------------------------
create or replace function @NAMESPACE@.replicate_partition(p_tab_id int4, p_nspname text, p_tabname text, p_idxname text, p_comment text) returns bigint as $$
declare
  prec record;
  prec2 record;
  v_set_id int4;

begin
-- Look up the parent table; fail if it does not exist
   select c1.oid into prec from pg_catalog.pg_class c1, pg_catalog.pg_class c2, pg_catalog.pg_inherits i, pg_catalog.pg_namespace n where c1.oid = i.inhparent  and c2.oid = i.inhrelid and n.oid = c2.relnamespace and n.nspname = p_nspname and c2.relname = p_tabname;
   if not found then
	raise exception 'replicate_partition: No parent table found for %.%!', p_nspname, p_tabname;
   end if;

-- The parent table tells us what replication set to use
   select tab_set into prec2 from @NAMESPACE@.sl_table where tab_reloid = prec.oid;
   if not found then
	raise exception 'replicate_partition: Parent table % for new partition %.% is not replicated!', prec.oid, p_nspname, p_tabname;
   end if;

   v_set_id := prec2.tab_set;

-- Now, we have all the parameters necessary to run add_empty_table_to_replication...
   return @NAMESPACE@.add_empty_table_to_replication(v_set_id, p_tab_id, p_nspname, p_tabname, p_idxname, p_comment);
end
$$ language plpgsql;

comment on function @NAMESPACE@.replicate_partition(p_tab_id int4, p_nspname text, p_tabname text, p_idxname text, p_comment text) is
'Add a partition table to replication.
tab_idxname is optional - if NULL, then we use the primary key.
This function looks up replication configuration via the parent table.

Note that this function is to be run within an EXECUTE SCRIPT script,
so it runs at the right place in the transaction stream on all
nodes.';


-- -------------------------------------------------------------------------
-- FUNCTION disable_indexes (oid)
-- -------------------------------------------------------------------------
create or replace function @NAMESPACE@.disable_indexes_on_table (i_oid oid) 
returns integer as $$
begin
	-- Setting pg_class.relhasindex to false will cause copy not to
	-- maintain any indexes. At the end of the copy we will reenable
	-- them and reindex the table. This bulk creating of indexes is
	-- faster.

	update pg_catalog.pg_class set relhasindex ='f' where oid = i_oid;
	return 1;
end $$
language plpgsql;

comment on function @NAMESPACE@.disable_indexes_on_table(i_oid oid) is
'disable indexes on the specified table.
Used during subscription process to suppress indexes, which allows
COPY to go much faster.

This may be set as a SECURITY DEFINER in order to eliminate the need
for superuser access by Slony-I.
';

-- -------------------------------------------------------------------------
-- FUNCTION enable_indexes_on_table (oid)
-- -------------------------------------------------------------------------
create or replace function @NAMESPACE@.enable_indexes_on_table (i_oid oid) 
returns integer as $$
begin
	update pg_catalog.pg_class set relhasindex ='t' where oid = i_oid;
	return 1;
end $$
language plpgsql
security definer;

comment on function @NAMESPACE@.enable_indexes_on_table(i_oid oid) is
're-enable indexes on the specified table.

This may be set as a SECURITY DEFINER in order to eliminate the need
for superuser access by Slony-I.
';
drop function if exists @NAMESPACE@.reshapeSubscription(int4,int4,int4);

create or replace function @NAMESPACE@.reshapeSubscription (p_sub_origin int4, p_sub_provider int4, p_sub_receiver int4) returns int4 as $$
begin
	update @NAMESPACE@.sl_subscribe
		   set sub_provider=p_sub_provider
		   from @NAMESPACE@.sl_set
		   WHERE sub_set=sl_set.set_id
		   and sl_set.set_origin=p_sub_origin and sub_receiver=p_sub_receiver;
	if found then
	   perform @NAMESPACE@.RebuildListenEntries();
	   notify "_@CLUSTERNAME@_Restart";
	end if;
	return 0;
end
$$ language plpgsql;

comment on function @NAMESPACE@.reshapeSubscription(p_sub_origin int4, p_sub_provider int4, p_sub_receiver int4) is
'Run on a receiver/subscriber node when the provider for that
subscription is being changed.  Slonik will invoke this method
before the SUBSCRIBE_SET event propogates to the receiver
so listen paths can be updated.';

create or replace function @NAMESPACE@.slon_node_health_check() returns boolean as $$
declare
		prec record;
		all_ok boolean;
begin
		all_ok := 't'::boolean;
		-- validate that all tables in sl_table have:
		--      sl_table agreeing with pg_class
		for prec in select tab_id, tab_relname, tab_nspname from
		@NAMESPACE@.sl_table t where not exists (select 1 from pg_catalog.pg_class c, pg_catalog.pg_namespace n
				where c.oid = t.tab_reloid and c.relname = t.tab_relname and c.relnamespace = n.oid and n.nspname = t.tab_nspname) loop
				all_ok := 'f'::boolean;
				raise warning 'table [id,nsp,name]=[%,%,%] - sl_table does not match pg_class/pg_namespace', prec.tab_id, prec.tab_relname, prec.tab_nspname;
		end loop;
		if not all_ok then
		   raise warning 'Mismatch found between sl_table and pg_class.  Slonik command REPAIR CONFIG may be useful to rectify this.';
		end if;
		return all_ok;
end
$$ language plpgsql;

comment on function @NAMESPACE@.slon_node_health_check() is 'called when slon starts up to validate that there are not problems with node configuration.  Returns t if all is OK, f if there is a problem.';

create or replace function @NAMESPACE@.log_truncate () returns trigger as
$$
	declare
		c_nspname text;
		c_relname text;
		c_log integer;
		c_node integer;
		c_tabid integer;
	begin
        c_tabid := tg_argv[0];
	    c_node := @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@');
		select tab_nspname, tab_relname into c_nspname, c_relname
				  from @NAMESPACE@.sl_table where tab_id = c_tabid;
		select last_value into c_log from @NAMESPACE@.sl_log_status;
		if c_log in (0, 2) then
			insert into @NAMESPACE@.sl_log_1 (
					log_origin, log_txid, log_tableid, 
					log_actionseq, log_tablenspname, 
					log_tablerelname, log_cmdtype, 
					log_cmdupdncols, log_cmdargs
				) values (
					c_node, pg_catalog.txid_current(), c_tabid,
					nextval('@NAMESPACE@.sl_action_seq'), c_nspname,
					c_relname, 'T', 0, '{}'::text[]);
		else   -- (1, 3) 
			insert into @NAMESPACE@.sl_log_2 (
					log_origin, log_txid, log_tableid, 
					log_actionseq, log_tablenspname, 
					log_tablerelname, log_cmdtype, 
					log_cmdupdncols, log_cmdargs
				) values (
					c_node, pg_catalog.txid_current(), c_tabid,
					nextval('@NAMESPACE@.sl_action_seq'), c_nspname,
					c_relname, 'T', 0, '{}'::text[]);
		end if;
		return NULL;
    end
$$ language plpgsql
    security definer;

comment on function @NAMESPACE@.log_truncate ()
is 'trigger function run when a replicated table receives a TRUNCATE request';

create or replace function @NAMESPACE@.deny_truncate () returns trigger as
$$
	begin
		raise exception 'truncation of replicated table forbidden on subscriber node';
    end
$$ language plpgsql;

comment on function @NAMESPACE@.deny_truncate ()
is 'trigger function run when a replicated table receives a TRUNCATE request';

create or replace function @NAMESPACE@.store_application_name (i_name text) returns text as $$
declare
		p_command text;
begin
		if exists (select 1 from pg_catalog.pg_settings where name = 'application_name') then
		   p_command := 'set application_name to '''|| i_name || ''';';
		   execute p_command;
		   return i_name;
		end if;
		return NULL::text;
end $$ language plpgsql;

comment on function @NAMESPACE@.store_application_name (i_name text) is
'Set application_name GUC, if possible.  Returns NULL if it fails to work.';

create or replace function @NAMESPACE@.is_node_reachable(origin_node_id integer,
	   receiver_node_id integer) returns boolean as $$
declare
		listen_row record;
		reachable boolean;
begin
	reachable:=false;
	select * into listen_row from @NAMESPACE@.sl_listen where
		   li_origin=origin_node_id and li_receiver=receiver_node_id;
	if found then
	   reachable:=true;
	end if;
  return reachable;
end $$ language plpgsql;

comment on function @NAMESPACE@.is_node_reachable(origin_node_id integer, receiver_node_id integer) 
is 'Is the receiver node reachable from the origin, via any of the listen paths?';

create or replace function @NAMESPACE@.component_state (i_actor text, i_pid integer, i_node integer, i_conn_pid integer, i_activity text, i_starttime timestamptz, i_event bigint, i_eventtype text) returns integer as $$
begin
	-- Trim out old state for this component
	if not exists (select 1 from @NAMESPACE@.sl_components where co_actor = i_actor) then
	   insert into @NAMESPACE@.sl_components 
             (co_actor, co_pid, co_node, co_connection_pid, co_activity, co_starttime, co_event, co_eventtype)
	   values 
              (i_actor, i_pid, i_node, i_conn_pid, i_activity, i_starttime, i_event, i_eventtype);
	else
	   update @NAMESPACE@.sl_components 
              set
                 co_connection_pid = i_conn_pid, co_activity = i_activity, co_starttime = i_starttime, co_event = i_event,
                 co_eventtype = i_eventtype
              where co_actor = i_actor 
	      	    and co_starttime < i_starttime;
	end if;
	return 1;
end $$
language plpgsql;

comment on function @NAMESPACE@.component_state (i_actor text, i_pid integer, i_node integer, i_conn_pid integer, i_activity text, i_starttime timestamptz, i_event bigint, i_eventtype text) is
'Store state of a Slony component.  Useful for monitoring';

create or replace function @NAMESPACE@.recreate_log_trigger(p_fq_table_name text,
       p_tab_id oid, p_tab_attkind text) returns integer as $$
begin
	execute 'drop trigger "_@CLUSTERNAME@_logtrigger" on ' ||
		p_fq_table_name	;
		-- ----
	execute 'create trigger "_@CLUSTERNAME@_logtrigger"' || 
			' after insert or update or delete on ' ||
			p_fq_table_name 
			|| ' for each row execute procedure @NAMESPACE@.logTrigger (' ||
                               pg_catalog.quote_literal('_@CLUSTERNAME@') || ',' || 
				pg_catalog.quote_literal(p_tab_id::text) || ',' || 
				pg_catalog.quote_literal(p_tab_attkind) || ');';
	return 0;
end
$$ language plpgsql;

comment on function  @NAMESPACE@.recreate_log_trigger(p_fq_table_name text,
       p_tab_id oid, p_tab_attkind text) is
'A function that drops and recreates the log trigger on the specified table.
It is intended to be used after the primary_key/unique index has changed.';

create or replace function @NAMESPACE@.repair_log_triggers(only_locked boolean)
returns integer as $$
declare
	retval integer;
	table_row record;
begin
	retval=0;
	for table_row in	
		select  tab_nspname,tab_relname,
				tab_idxname, tab_id, mode,
				@NAMESPACE@.determineAttKindUnique(tab_nspname||
					'.'||tab_relname,tab_idxname) as attkind
		from
				@NAMESPACE@.sl_table
				left join 
				pg_locks on (relation=tab_reloid and pid=pg_backend_pid()
				and mode='AccessExclusiveLock')				
				,pg_trigger
		where tab_reloid=tgrelid and 
		@NAMESPACE@.determineAttKindUnique(tab_nspname||'.'
						||tab_relname,tab_idxname)
			!=(@NAMESPACE@.decode_tgargs(tgargs))[2]
			and tgname =  '_@CLUSTERNAME@'
			|| '_logtrigger'
		LOOP
				if (only_locked=false) or table_row.mode='AccessExclusiveLock' then
					 perform @NAMESPACE@.recreate_log_trigger
					 		 (table_row.tab_nspname||'.'||table_row.tab_relname,
							 table_row.tab_id,table_row.attkind);
					retval=retval+1;
				else 
					 raise notice '%.% has an invalid configuration on the log trigger. This was not corrected because only_lock is true and the table is not locked.',
					 table_row.tab_nspname,table_row.tab_relname;
			
				end if;
		end loop;
	return retval;
end
$$
language plpgsql;
comment on function @NAMESPACE@.repair_log_triggers(only_locked boolean)
is '
repair the log triggers as required.  If only_locked is true then only 
tables that are already exclusively locked by the current transaction are 
repaired. Otherwise all replicated tables with outdated trigger arguments
are recreated.';


create or replace function @NAMESPACE@.unsubscribe_abandoned_sets(p_failed_node int4) returns bigint
as $$
declare
v_row record;
v_seq_id bigint;
v_local_node int4;
begin

	select @NAMESPACE@.getLocalNodeId('_@CLUSTERNAME@') into
			   v_local_node;

	if found then
		   --abandon all subscriptions from this origin.
		for v_row in select sub_set,sub_receiver from
			@NAMESPACE@.sl_subscribe, @NAMESPACE@.sl_set
			where sub_set=set_id and set_origin=p_failed_node
			and sub_receiver=v_local_node
		loop
				raise notice 'Slony-I: failover_abandon_set() is abandoning subscription to set % on node % because it is too far ahead', v_row.sub_set,
				v_local_node;
				--If this node is a provider for the set
				--then the receiver needs to be unsubscribed.
				--
			select @NAMESPACE@.unsubscribeSet(v_row.sub_set,
												v_local_node,true)
				   into v_seq_id;
		end loop;
	end if;

	return v_seq_id;
end
$$ language plpgsql;



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
