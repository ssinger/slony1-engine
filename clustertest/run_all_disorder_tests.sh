#!/bin/sh
JC=conf/java.conf
if [ -f ${JC} ] ; then
    . ${JC}
else
    echo "No Java configuration found in ${JC}"
    echo "  Likely, ${JC}.sample has not yet been copied to ${JC}"
    echo "  to allow for your local configuration"
    exit 1
fi

DBP=conf/disorder.properties
if [ -f ${DBP} ]; then
    i=1
else
    echo "No database configuration found in ${DBP}"
    echo "  Likely, ${DBP}.sample has not yet been copied to ${DBP}"
    echo "  to allow for your local configuration"
    exit 1
fi    

echo "Start test by dropping databases based on config in ${DBP}"
for db in db1 db2 db3 db4 db5; do
    PGTESTHOST=`grep "database.${db}.host=" ${DBP} | cut -d = -f 2`
    PGTESTPORT=`grep "database.${db}.port=" ${DBP} | cut -d = -f 2`
    PGTESTUSER=`grep "database.${db}.user.slony=" ${DBP} | cut -d = -f 2`
    PGTESTDATABASE=`grep "database.${db}.dbname=" ${DBP} | cut -d = -f 2`
    PGTESTPATH=`grep "database.${db}.pgsql.path=" ${DBP} | cut -d = -f 2`
    #PGTESTPASSWORD=`grep "database.${db}.password=" ${DBP} | cut -d = -f 2`
    echo "Dropping database: ${PGTESTPATH}/dropdb -h ${PGTESTHOST} -p ${PGTESTPORT} -U ${PGTESTUSER} ${PGTESTDATABASE}"
    ${PGTESTPATH}/psql -h ${PGTESTHOST} -p ${PGTESTPORT} -U ${PGTESTUSER} -d template1 -c "drop database if exists ${PGTESTDATABASE};"
done

java -jar ${CLUSTERTESTJAR} ${DBP} disorder/tests/disorder_tests.js
