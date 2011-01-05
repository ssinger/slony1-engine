--- 

-- This SQL script is used to test the new listen path generation code
-- to make sure the resulting sl_listen allows all nodes to be 
-- reachable/audible

-- This test basically messes with sl_path, sl_listen, sl_node,
-- sl_subscribe by hand, assuming that the tables and normal functions exist
-- in the schema "_slony_regress1"

-- You can get this schema set up either by:

-- 1. Running a slonik script that does "init cluster()" for a cluster
--    called slony_regress1

-- 2. Running just about any of the test bed scripts, which default to
--    create the slony_regress1 cluster, and stop the script before
--    it ends and purges out the databases

-- Some helpers for the test below

create table nodes (id int4 not null primary key) ;
insert into nodes values (1);
insert into nodes values (2);
insert into nodes values (3);
insert into nodes values (4);
insert into nodes values (5);
insert into nodes values (6);
insert into nodes values (7);
insert into nodes values (8);
insert into nodes values (9);
insert into nodes values (10);


create or replace view _slony_regress1.listener_orphans as
   select n1.no_id as origin, n2.no_id as receiver
   from _slony_regress1.sl_node n1, _slony_regress1.sl_node n2
   where n1.no_id <> n2.no_id and
         not exists (select true from _slony_regress1.sl_listen
                     where li_origin = n1.no_id and li_receiver = n2.no_id);

create or replace function "_slony_regress1".are_all_nodes_audible () returns int4 as '
declare
   v_failed int4;
   v_source record;
   v_dest record;
begin
	v_failed := 0;
	for v_source in select no_id from "_slony_regress1".sl_node loop
	   for v_dest in select no_id from "_slony_regress1".sl_node where no_id <> v_source.no_id loop
		if "_slony_regress1".can_node_hear (v_source.no_id, v_dest.no_id) then
			raise notice ''Slony-I: Node % can hear %'', v_source.no_id, v_dest.no_id;
		else
			raise notice ''Slony-I: Node % cannot hear %'', v_source.no_id, v_dest.no_id;
			v_failed := v_failed + 1;
		end if;
	   end loop;
	end loop;
   	return v_failed;
end;' language plpgsql;


create or replace function "_slony_regress1".can_node_hear (v_receiver int4, v_origin int4) returns boolean as '
declare
   v_rec    record;
   v_clist int4[];
   v_csize int4;
   i int4;
   j int4;
   done boolean;
   add_item boolean;
begin
  v_clist :=  ARRAY[v_receiver];
  v_csize := 1;
  done := ''f'';
  while done = ''f'' loop
    for i in 1..v_csize loop
      for v_rec in select distinct li_provider from "_slony_regress1".sl_listen where
                         li_receiver = v_clist[i] loop
        if v_rec.li_provider = v_origin then
  	  return ''t'';
        end if;
        add_item := ''t'';
        for j in 1..v_csize loop
	  if v_clist[i] = v_rec.li_provider then
            add_item := ''f'';
            exit;  -- No need to keep searching
          end if;
        end loop;
        if add_item then
          v_csize := v_csize + 1;
          v_clist[v_csize] := v_rec.li_provider;
        end if;
      end loop;
    end loop;
  end loop;
  return ''f'';
end;' language plpgsql;   

--Test1
-- 21 <-> 20 <-> 1 <-> 10 <-> 11

truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node(no_id) values (1);
insert into _slony_regress1.sl_node(no_id) values (10);
insert into _slony_regress1.sl_node(no_id) values (11);
insert into _slony_regress1.sl_node(no_id) values (20);
insert into _slony_regress1.sl_node(no_id) values (21);

insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,10,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,20,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (10,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (10,11,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (11,10,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (20,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (20,21,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (21,20,'');

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

select 'Try again, specifying subscriptions, as well.';
insert into _slony_regress1.sl_set(set_id, set_origin) values (1, 1);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 1, 10);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 1, 20);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 10, 11);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 21, 21);

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();


--Test2
-- 2 <-- 1 --> 4
-- | ^ |
-- | / \ |
-- v / \ v
-- 3 5
truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node(no_id) values (1);
insert into _slony_regress1.sl_node(no_id) values (2);
insert into _slony_regress1.sl_node(no_id) values (3);
insert into _slony_regress1.sl_node(no_id) values (4);
insert into _slony_regress1.sl_node(no_id) values (5);

insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,3,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,5,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (2,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (3,2,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (4,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (5,4,'');

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

--Test3
--Fully meshed setup with 10 nodes
truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node (no_id) select * from nodes;

insert into _slony_regress1.sl_path (pa_client, pa_server, pa_conninfo)
select n1.id, n2.id, 'test dsn'
from nodes n1, nodes n2
where n1.id != n2.id ;

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

--Test4
--A transitiv graph with 10 nodes
--This should warn about unreachable nodes
truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node (no_id) select * from nodes;

insert into _slony_regress1.sl_path (pa_client, pa_server, pa_conninfo)
select n1.id, n2.id, 'test dsn'
from nodes n1, nodes n2
where n1.id < n2.id ;

select _slony_regress1.rebuildlistenentries();
select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

--Test5
--A (nearly) transitiv graph with 10 nodes, but with the missing
--connection (1 -> 10) added.
truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node (no_id) select * from nodes;

insert into _slony_regress1.sl_path (pa_client, pa_server, pa_conninfo)
select n1.id, n2.id, 'test dsn'
from nodes n1, nodes n2
where n1.id < n2.id ;

insert into _slony_regress1.sl_path (pa_client, pa_server, pa_conninfo)
values (10, 1, 'complete the graph...');

select _slony_regress1.rebuildlistenentries();
select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();


--Test6
-- 21 <-> 20 <-> 1 <-> 10 <-> 11

truncate _slony_regress1.sl_set, _slony_regress1.sl_setsync, _slony_regress1.sl_table, _slony_regress1.sl_sequence, _slony_regress1.sl_subscribe, _slony_regress1.sl_listen, _slony_regress1.sl_path, _slony_regress1.sl_node;

insert into _slony_regress1.sl_node(no_id) values (1);
insert into _slony_regress1.sl_node(no_id) values (10);
insert into _slony_regress1.sl_node(no_id) values (11);
insert into _slony_regress1.sl_node(no_id) values (20);
insert into _slony_regress1.sl_node(no_id) values (21);

insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,10,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (1,20,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (10,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (10,11,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (11,10,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (20,1,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (20,21,'');
insert into _slony_regress1.sl_path(pa_server, pa_client, pa_conninfo) values (21,20,'');

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

select 'Try again, specifying subscriptions, as well.';
insert into _slony_regress1.sl_set(set_id, set_origin) values (1, 10);
insert into _slony_regress1.sl_set(set_id, set_origin) values (2, 20);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 10, 11);
insert into _slony_regress1.sl_subscribe(sub_set, sub_provider, sub_receiver) values (1, 20, 21);

select _slony_regress1.rebuildlistenentries();

select * from _slony_regress1.sl_listen order by li_origin, li_receiver, li_provider;
select * from _slony_regress1.listener_orphans;
select "_slony_regress1".are_all_nodes_audible();

