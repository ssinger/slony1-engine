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

DBP=conf/slonyregress.properties
if [ -f ${DBP} ]; then
    i=1
else
    echo "No database configuration found in ${DBP}"
    echo "  Likely, ${DBP}.sample has not yet been copied to ${DBP}"
    echo "  to allow for your local configuration"
    exit 1
fi    

java -jar ${CLUSTERTESTJAR} ${DBP} ./regression/test1/test1.js ./regression/testdatestyles/testdatestyles.js ./regression/testddl/testddl.js ./regression/testdeadlockddl/testdeadlockddl.js ./regression/testinherit/testinherit.js ./regression/testlargetuples/testlargetuples.js ./regression/testmergeset/testmergeset.js ./regression/testmultipaths/testmultipaths.js ./regression/testmultiplemoves/testmultiplemoves.js ./regression/testomitcopy/testomitcopy.js ./regression/testschemanames/testschemanames.js ./regression/testseqnames/testseqnames.js ./regression/testtabnames/testtabnames.js ./regression/testutf8/testutf8.js ./regression/testtruncate/testtruncate.js
