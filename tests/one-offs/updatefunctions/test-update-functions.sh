#!/bin/sh
# $Id: test-update-functions.sh,v 1.1 2007-01-15 21:38:24 cbbrowne Exp $

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

OURCVSROOT=":pserver:anonymous@gborg.postgresql.org:/usr/local/cvsroot/slony1"

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
	REL_1_2_0
	REL_1_2_1
	REL_1_2_2
	REL_1_2_5
	REL_1_2_6
"

LATESTVERSION=`echo ${VERSIONS} | sort | tail -1`

# Make sure all versions of Slony-I are present and built
for REL in `echo $VERSIONS | sort`; do
    if [ -e ${SRCHOME}/slony1-engine-${REL} ] ; then
      CHECKOUT=0
   else
      cd ${SRCHOME}
echo"     CVSROOT=\"${OURCVSROOT}\"  cvs export -r${REL} slony1-engine"
     CVSROOT="${OURCVSROOT}"  cvs export -r${REL} slony1-engine
     mv slony1-engine slony1-engine-${REL}
     cd ${SRCHOME}/slony1-engine-${REL}
     if [ echo ${REL} | egrep 'REL_1_2' ] ; then
       ./configure  --with-pgconfigdir=${PGBINDIR}
     fi
     if [ echo ${REL} | egrep 'REL_1_1' ] ; then
       ./configure  --with-pgconfigdir=${PGBINDIR} --with-pgbindir=${PGBINDIR} --with-pgincludedir=${PGHOME}/include   --with-pgincludeserverdir=${PGHOME}/include/server --with-pglibdir=${PGHOME}/lib   --with-pgpkglibdir=${PGHOME}/lib   --with-pgsharedir=${PGHOME}/share
     fi
     make
  fi
done

echo <<EOF > $SRCHOME/init-cluster.slonik
cluster name = ${SLONYCLUSTER};
node 1 admin conninfo='host=${PGHOST} dbname=${PGDATABASE} user=${PGUSER} port=${PGPORT}';
init cluster (id=1, 'test cluster for checking schema');
EOF

echo <<EOF > $SRCHOME/update-functions.slonik
cluster name = ${SLONYCLUSTER};
node 1 admin conninfo='host=${PGHOST} dbname=${PGDATABASE} user=${PGUSER} port=${PGPORT}';
update functions (id=1);
EOF

for rel in `echo $VERSIONS | sort`; do
  ${PGBINDIR}/dropdb -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE}
  ${PGBINDIR}/createdb -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE}
  ${PGBINDIR}/createlang -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} plpgsql ${PGDATABASE}
  cd ${SRCHOME}/slony1-engine-${rel}
  make install
  ${PGBINDIR}/slonik ${SRCHOME}/init-cluster.slonik
  cd ${SRCHOME}/slony1-engine-${LATESTVERSION}
  make install
  ${PGBINDIR}/slonik ${SRCHOME}/update-functions.slonik
  mkdir -p $SRCHOME/relschemas
  ${PGBINDIR}/pg_dump  -h ${PGHOST} -p ${PGPORT} -U ${PGUSER} ${PGDATABASE} > $SRCHOME/relschemas/${rel}-to-${LATESTVERSION}.sql
done
