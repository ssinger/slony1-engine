#!/bin/bash
# 

# Here is a test which sets up 3 clusters:
#   TLD1  - tld1 + billing    - origin node receiving data, and "billing" DB doing aggregation
#   TLD2  - tld2 + billing    - origin node receiving data, and "billing" DB doing aggregation
#   DW    - billing + dw      - cluster that processes aggregated data

#   SLONYTOOLS needs to be set

SLONYTOOLS=${HOME}/Slony-I/CMD/slony1-HEAD

for db in tld1 tld2 billing dw; do
  dropdb $db
  createdb $db
  createlang plpgsql $db
done

TESTHOME=/tmp/inhcltst
rm -rf ${TESTHOME}
mkdir -p ${TESTHOME}

for TLD in tld1 tld2; do
  NTABLES=""
  for t in domains hosts contacts txns; do
    NTABLES="${NTABLES} ${TLD}.${t}"
  done
  for t in host contact; do
    NTABLES="${NTABLES} ${TLD}.domain_${t}"
  done
  NSEQUENCES=""
  for s in domains hosts contacts; do
    NSEQUENCES="${NSEQUENCES} ${TLD}.${s}_id_seq"
  done

SLONIKCONFDIR="${TESTHOME}/${TLD}"  DB2=billing  DB1="${TLD}"  NUMNODES=2 CLUSTER=${TLD} HOST1=localhost HOST2=localhost TABLES="${NTABLES}" SEQUENCES="${NSEQUENCES}" ${SLONYTOOLS}/tools/configure-replication.sh
done


NTABLES=""
for t in txns; do
  NTABLES="${NTABLES} billing.${t}"
done
NSEQUENCES=""
for s in txns; do
  NSEQUENCES="${NSEQUENCES} billing.${s}_id_seq"
done
SLONIKCONFDIR=${TESTHOME}/dw CLUSTER=dw NUMNODES=2 DB1=billing DB2=dw HOST1=localhost HOST2=localhost TABLES="${NTABLES}" SEQUENCES="${NSEQUENCES}" ${SLONYTOOLS}/tools/configure-replication.sh

for DB in tld1 billing; do
 for schema in tld1 billing; do 
   psql -d ${DB} -c "create schema ${schema};"
 done
done

for DB in tld2 billing; do
 for schema in tld2 billing; do 
   psql -d ${DB} -c "create schema ${schema};"
 done
done

psql -d dw -c "create schema billing;"

for TLD in tld1 tld2; do
 for DB in ${TLD} billing; do
   psql -d ${DB} -c "create table ${TLD}.txns (id serial primary key, domain_id integer not null, created_on timestamptz default now(), txn_type text);"
 done
done

for DB in billing dw; do
   psql -d ${DB} -c "create table billing.txns (id serial, tld text, domain_id integer not null, created_on timestamptz default now(), txn_type text, primary key(id, tld));"
done


for TLD in tld1 tld2; do
   for DB in ${TLD} billing; do
       for table in domains hosts contacts; do
	   psql -d ${DB} -c "create table ${TLD}.${table} (id serial primary key, name text not null unique, created_on timestamptz default now());"
       done
       for sub in host contact; do
	   psql -d ${DB} -c "create table ${TLD}.domain_${sub} (domain_id integer not null, ${sub}_id integer not null, primary key (domain_id, ${sub}_id));"
       done
   done
done

MKRAND="
create or replace function public.randomtext (integer) returns text as \$\$
declare
  i_id alias for \$1;
  c_res text;
  c_fc integer;
  c_str text;
  c_base integer;
  c_bgen integer;
  c_rand integer;
  c_i integer;
  c_j integer;
begin
  c_res := 'PF';
  c_str := '1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ';
  c_base := length(c_str);
  c_bgen := c_base * c_base * c_base * c_base;
  for c_i in 1..3 loop
    c_rand := floor(random() * c_bgen + i_id + random())::integer % c_bgen;
    for c_j in 1..4 loop
      c_fc := c_rand % c_base;
      c_res := c_res || substr(c_str, c_fc, 1);
      c_rand := c_rand / c_base;
    end loop;
  end loop;
  return lower(c_res);
end;\$\$ language 'plpgsql';
"  

for db in tld1 tld2; do
   psql -d ${db} -c "${MKRAND}"
done


for tld in tld1 tld2; do
MKDOMAIN="
create or replace function ${tld}.mkdomain (text) returns integer as \$\$
declare
   i_tld alias for \$1;
   c_domain_name text;
   c_domain_id integer;
   c_contact_name text;
   c_contact_id integer;
   c_host_name text;
   c_host_id integer;

begin
   c_domain_name := public.randomtext(nextval('${tld}.domains_id_seq')::integer) || '.' || i_tld;
   insert into ${tld}.domains (name) values (c_domain_name);
   select currval('${tld}.domains_id_seq') into c_domain_id;
   c_contact_name := public.randomtext(nextval('${tld}.contacts_id_seq')::integer);
   insert into ${tld}.contacts (name) values (c_contact_name);
   select currval('${tld}.contacts_id_seq') into c_contact_id;
   insert into ${tld}.domain_contact (domain_id, contact_id) values (c_domain_id, c_contact_id);
   insert into ${tld}.hosts (name) values ('ns1.' || public.randomtext(nextval('${tld}.hosts_id_seq')::integer) || '.net');
   select currval('${tld}.hosts_id_seq') into c_host_id;
   insert into ${tld}.domain_host (domain_id, host_id) values (c_domain_id, c_host_id);
   insert into ${tld}.domain_host (domain_id, host_id) select c_domain_id, id from ${tld}.hosts order by random() limit 1;
   insert into ${tld}.txns (domain_id, txn_type) values (c_domain_id, 'domain:create');
   return c_domain_id;
end; \$\$ language plpgsql;
"
   psql -d ${tld} -c "${MKDOMAIN}"
done

psql -d billing -c "
create table billing.work_queue (tld text, txn_id integer);
create or replace function tld1.add_to_queue () returns trigger as \$\$
begin
   insert into billing.work_queue (tld, txn_id) values ('tld1', NEW.id);
   return new;
end; \$\$ language plpgsql;

create or replace function tld2.add_to_queue () returns trigger as \$\$
begin
   insert into billing.work_queue (tld, txn_id) values ('tld2', NEW.id);
   return new;
end; \$\$ language plpgsql;

create or replace function billing.work_on_queue(integer) returns integer as \$\$
declare
   c_prec record;
   c_count integer;
begin
   c_count := 0;
   for c_prec in select * from billing.work_queue limit \$1 loop
      if c_prec.tld = 'tld1' then
         insert into billing.txns (id, tld, domain_id, created_on, txn_type) select id, 'tld1', domain_id, created_on, txn_type from tld1.txns where id = c_prec.txn_id;
         delete from billing.work_queue where tld = c_prec.tld and txn_id = c_prec.txn_id;
         c_count := c_count + 1;
      end if;

      if c_prec.tld = 'tld2' then
         insert into billing.txns (id, tld, domain_id, created_on, txn_type) select id, 'tld2', domain_id, created_on, txn_type from tld2.txns where id = c_prec.txn_id;
         delete from billing.work_queue where tld = c_prec.tld and txn_id = c_prec.txn_id;
         c_count := c_count + 1;
      end if;
   end loop;
   return c_count;
end; \$\$ language plpgsql;

create or replace function billing.work_on_queue() returns integer as \$\$
begin
    return billing.work_on_queue(97);
end; \$\$ language plpgsql;

create trigger tld1_queue after insert on tld1.txns for each row execute procedure tld1.add_to_queue ();
create trigger tld2_queue after insert on tld2.txns for each row execute procedure tld2.add_to_queue ();
"


for i in create_nodes create_set store_paths subscribe_set_2; do
  for j in `find ${TESTHOME} -name "${i}.slonik"`; do
    echo "Executing slonik script: ${j}"
    slonik ${j}
  done
done

# Add in "store trigger" to activate the above triggers
for tld in tld1 tld2; do
  TFILE=${TESTHOME}/${tld}/store-billing-trigger.slonik
  echo "include <${TESTHOME}/${tld}/preamble.slonik>;" > ${TFILE}
  echo "store trigger (table id = 4, trigger name = 'tld1_queue', event node = 1);" >> ${TFILE}
  slonik ${TFILE}
done

for cluster in tld1 tld2; do
  for nodetld2 in 1:${cluster} 2:billing; do
    node=`echo $nodetld2 | cut -d ":" -f 1`
    dbname=`echo $nodetld2 | cut -d ":" -f 2`
    echo "Briefly fire up slon for ${nodetld2}"
    (sleep 3; killall slon)&
    slon -q 5 ${cluster} "dbname=${dbname}" > /dev/null 2> /dev/null
  done    
  DESTDIR=${TESTHOME}/${cluster}
  SLONYCLUSTER=${cluster} MKDESTINATION=${DESTDIR} LOGHOME=${DESTDIR}/logs PGDATABASE=${cluster} ${SLONYTOOLS}/tools/mkslonconf.sh
done

for cluster in dw; do
  for nodetld2 in 1:billing 2:dw; do
    node=`echo $nodetld2 | cut -d ":" -f 1`
    dbname=`echo $nodetld2 | cut -d ":" -f 2`
    echo "Briefly fire up slon for ${nodetld2}"
    (sleep 3; killall slon)&
    slon -q 5 ${cluster} "dbname=${dbname}" > /dev/null 2> /dev/null
  done    
  DESTDIR=${TESTHOME}/${cluster}
  SLONYCLUSTER=${cluster} MKDESTINATION=${DESTDIR} LOGHOME=${DESTDIR}/logs PGDATABASE=${cluster} ${SLONYTOOLS}/tools/mkslonconf.sh
done

for cluster in tld1 tld2 dw; do
  DESTDIR=${TESTHOME}/${cluster}
  for i in 1 2; do
    echo "Start up node ${i} for real..."
    CONF="${DESTDIR}/${cluster}/conf/node${i}.conf"
    LOG="${DESTDIR}/logs/node${i}.log"
    slon -f ${CONF} > ${LOG} 2>&1 &
  done
done

# Add in "store trigger" to activate the billing triggers
echo "Generate STORE TRIGGER scripts to activate billing triggers"
for tld in tld1 tld2; do
  TFILE=${TESTHOME}/${tld}/store-billing-trigger.slonik
  echo "include <${TESTHOME}/${tld}/preamble.slonik>;" > ${TFILE}
  echo "store trigger (table id = 4, trigger name = '${tld}_queue', event node = 1);" >> ${TFILE}
  slonik ${TFILE}
  echo "Activated trigger ${tld}_queue"
done

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do
   for tld in tld1 tld2; do
      BQ="select ${tld}.mkdomain('${tld}');"
      Q="begin;"
      for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25; do
       for q in 1 2 3 4; do
         Q="${Q} ${BQ} "
       done
      done
      Q="${Q} end;"
      psql -d ${tld} -c "${Q}" &
      sleep 1
   done
   psql -d billing -c "select billing.work_on_queue(167);" &
done

for i in 1 2 3 4 5 6 7 8 9 10 11 12; do
  psql -d billing -c "select billing.work_on_queue(277);"
  psql -d billing -c "vacuum billing.work_queue;"
done