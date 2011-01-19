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

DBP=conf/databases.properties
if [ -f ${DBP} ]; then
    i=1
else
    echo "No database configuration found in ${DBP}"
    echo "  Likely, ${DBP}.sample has not yet been copied to ${DBP}"
    echo "  to allow for your local configuration"
    exit 1
fi    

java -jar ${CLUSTERTESTHOME}/build/jar/clustertest-coordinator.jar ${DBP} disorder/tests/disorder_tests.js
