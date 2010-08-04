#!/bin/sh
# 

# This script runs through what it can of the release checklist
# run via:  "sh tools/release_checklist.sh"
# It assumes the current directory is the top directory of the build

echo "Slony-I Release Checklist"

echo "pulling version information from config.h.in"
VERDOTTED=`egrep 'SLONY_I_VERSION_STRING.*"[0-9]+\.[0-9]+\.[0-9]+"' config.h.in | cut -d'"' -f 2`
MAJOR=`echo $VERDOTTED | cut -d '.' -f 1`
MINOR=`echo $VERDOTTED | cut -d '.' -f 2`
PATCHLEVEL=`echo $VERDOTTED | cut -d '.' -f 3`

echo "Slony-I version: ${VERDOTTED} - Major=${MAJOR} Minor=${MINOR} Patchlevel=${PATCHLEVEL} per config.h.in"

VERCOMMA="${MAJOR},${MINOR},${PATCHLEVEL}"
VERUNDERSCORE="${MAJOR}_${MINOR}_${PATCHLEVEL}"
if egrep "#define SLONY_I_VERSION_STRING_DEC ${VERCOMMA}\$" config.h.in >/dev/null 2>&1; then
   echo "SLONY_I_VERSION_STRING_DEC matches"
else
   echo "ERROR: SLONY_I_VERSION_STRING_DEC does not match ${VERCOMMA}"
   grep SLONY_I_VERSION_STRING_DEC config.h.in
fi

echo "Verifying configure..."
if egrep "^PACKAGE_VERSION='${VERDOTTED}'\$" configure >/dev/null 2>&1; then
   echo "configure PACKAGE_VERSION matches ${VERDOTTED}"
else
   echo "ERROR: PACKAGE_VERSION in configure does not match ${VERDOTTED}"
   egrep "PACKAGE_VERSION\=" configure
fi

if egrep "^PACKAGE_STRING='slony1 ${VERDOTTED}'\$" configure >/dev/null 2>&1; then
   echo "PACKAGE_STRING in configure matches slony1 ${VERDOTTED}"
else
   echo "ERROR: configure PACKAGE_STRING does not match ${VERDOTTED}"
   egrep "PACKAGE_STRING\=" configure
fi

FLIST=""
for file in `find config -name "*.m4" -newer configure | sort`; do
    FLIST="${FLIST} $file"
done
if [ x = x"$FLIST" ]; then
    echo "autoconf has probably been run lately..."
else
    echo "WARNING:: The following ./configure constituents are newer than ./configure - you probably should run autoconf!"
    echo "$FLIST" | fmt
fi

STOREDPROCVERS=`awk  -f tools/awk-for-stored-proc-vers.awk  src/backend/slony1_funcs.sql`

if [ x"$STOREDPROCVERS" = x"$VERDOTTED" ]; then
   OK=1
   echo "Stored proc version numbers match ${VERDOTTED}"
else
   echo "ERROR: Stored proc versions in src/backend/slony1_funcs.sql indicates version [${STOREDPROCVERS}] (versus [${VERDOTTED}])"
fi


for file in `find src/slon -name "conf-file.l" -newer src/slon/conf-file.c | sort`; do
    echo "WARNING: src/slon/conf-file.l newer than child file src/slon/conf-file.c"
done

for file in `find src/slonik -name "scan.l" -newer src/slonik/scan.c | sort`; do
    echo "WARNING: src/slonik/scan.l newer than child file src/slonik/scan.c"
done

for file in `find src/slonik -name "parser.y" -newer src/slonik/parser.c | sort`; do
    echo "WARNING: src/slonik/parser.y newer than child file src/slonik/parser.c"
done
