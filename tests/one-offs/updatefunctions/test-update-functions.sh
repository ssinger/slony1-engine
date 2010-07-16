#!/bin/sh
# 

# Where to stow Slony-I checkouts?
SRCHOME=${SRCHOME:-"/tmp/slony-sources"}
mkdir -p ${SRCHOME}

# Pick a PostgreSQL build to do this against...
PGHOME=${PGHOME:-"/opt/OXRS/dbs/pgsql81"}
PGBINDIR=${PGHOME}/bin

PGPORT=${PGPORT:-"5832"}
PGHOST=${PGHOST:-"localhost"}
PGUSER=${PGUSER:-"cbbrowne"}
PGDATABASE=${PGDATABASE:-"functest"}
SLONYCLUSTER=${SLONYCLUSTER:-"functest"}

OURCVSROOT=":pserver:anonymous@main.slony.info:/slony1"

ELDERVERSIONS="
	REL_1_0_0
	REL_1_0_1
	REL_1_0_2
	REL_1_0_4
	REL_1_0_5
	REL_1_0_6
"

VERSIONS="
	REL_1_1_0
	REL_1_1_1
	REL_1_1_2
	REL_1_1_5
	REL_1_1_6
	REL_1_1_7
	REL_1_1_8
	REL_1_1_9
	REL_1_2_0
	REL_1_2_1
	REL_1_2_2
	REL_1_2_5
	REL_1_2_6
	REL_1_2_7
	REL_1_2_8
	REL_1_2_9
"

LATESTVERSION=`echo ${VERSIONS} | fmt -w 5 | sort | tail -1`

# Make sure all versions of Slony-I are present and built
for REL in `echo $VERSIONS | sort`; do
  if [ -e ${SRCHOME}/slony1-engine-${REL} ] ; then
     CHECKOUT=0
  else
    echo "Check out repository for version ${REL}"
    cd ${SRCHOME}
    CVSROOT="${OURCVSROOT}"  cvs export -r${REL} slony1-engine
    mv slony1-engine slony1-engine-${REL}
  fi
  echo "run autoconf version ${REL}"
  cd ${SRCHOME}/slony1-engine-${REL}
  autoconf
  for i in `echo ${REL} | egrep REL_1_2`; do
    echo "Build 1.2 release ${REL}"
    ./configure  --with-pgconfigdir=${PGBINDIR}
  done
  for i in `echo ${REL} | egrep REL_1_1`; do
    echo "Build 1.1 release ${REL}"
    ./configure  --with-pgconfigdir=${PGBINDIR} --with-pgbindir=${PGBINDIR} --with-pgincludedir=${PGHOME}/include   --with-pgincludeserverdir=${PGHOME}/include/server --with-pglibdir=${PGHOME}/lib   --with-pgpkglibdir=${PGHOME}/lib   --with-pgsharedir=${PGHOME}/share
  done
  make
done

echo "
cluster name = ${SLONYCLUSTER};
node 1 admin conninfo='host=${PGHOST} dbname=${PGDATABASE} user=${PGUSER} port=${PGPORT}';
init cluster (id=1, comment='test cluster for checking schema');
" > $SRCHOME/init-cluster.slonik

echo "
cluster name = ${SLONYCLUSTER};
node 1 admin conninfo='host=${PGHOST} dbname=${PGDATABASE} user=${PGUSER} port=${PGPORT}';
update functions (id=1);
" > $SRCHOME/update-functions.slonik

for rel in `echo $VERSIONS | sort`; do
  echo "Prepare databases for ${rel}"
  ${PGBINDIR}/dropdb -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE}
  ${PGBINDIR}/createdb -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE}
  ${PGBINDIR}/createlang -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} plpgsql ${PGDATABASE}
  echo "Install Slony-I version ${rel}"
  cd ${SRCHOME}/slony1-engine-${rel}/src
  make install
  echo "Initialize Slony-I cluster based on release ${rel}"
  ${PGBINDIR}/slonik ${SRCHOME}/init-cluster.slonik
  echo "Install Slony-I version ${LATESTVERSION}"
  cd ${SRCHOME}/slony1-engine-${LATESTVERSION}/src
  make install
  echo "Update cluster to ${LATESTVERSION}"
  ${PGBINDIR}/slonik ${SRCHOME}/update-functions.slonik
  mkdir -p $SRCHOME/relschemas
  echo "Dump ${rel}-to-${LATESTVERSION} schema"
  ${PGBINDIR}/pg_dump  -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE} > $SRCHOME/relschemas/${rel}-to-${LATESTVERSION}.sql
done
