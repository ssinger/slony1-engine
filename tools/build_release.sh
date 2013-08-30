#!/bin/sh
############################################################################
#
# Builds a slony release in an automated way.
#
# usage: build_release.sh git_branch version pg_configdir
############################################################################

GIT_TAG=$1
REL_VERSION=$2
PGCONFIG_DIR=$3

if [ "$GIT_TAG" = "" -o "$REL_VERSION" = "" \
	-o "$PGCONFIG_DIR" = "" ]
then
  echo "usage: build_release.sh GIT_TAG VERSION PGCONFIG_DIR"
  exit -1
fi

DIR=`basename $PWD`
if [ "$DIR" != "slony1-engine" ]
then
    echo "must be in slony1-engine directory"
    exit -1
fi
git checkout $GIT_TAG
REL_TAG=REL_`echo $REL_VERSION|sed -e 's|\.|_|g'|tr '[:lower:]' '[:upper:]' `
git tag -a -m "Tagging $REL_VERSION" $REL_TAG
if [ -f "/tmp/slony1-engine-$REL_VERSION.tar" ]
then
  echo "/tmp/slony1-engine-$REL_VERSION.tar exists please delete first"
  exit -1
fi
git archive -o /tmp/slony1-engine-$REL_VERSION.tar $REL_TAG
if [ $? -ne 0 ]
then
  echo "git archive failed"
  exit -1
fi
cd ..

if [ -f "slony1-$REL_VERSION" ] 
then
  echo "slony1-REL_VERSION directory exists. please delete first"
  exit -1
fi

mkdir slony1-$REL_VERSION
cd slony1-$REL_VERSION
tar -xf /tmp/slony1-engine-$REL_VERSION.tar
autoconf
./configure --with-pgconfigdir=$PGCONFIG_DIR --with-docs
make
cd doc/adminguide
make html
make html
make man
# build PDF.  This requires dblatex
#
make slony.xml
make slony.pdf
cd ..
cd ..
sh tools/release_checklist.sh
ANS=""
while [ "$ANS" != "Y" -a "$ANS" != "N" ]
do
    echo "Does the release checklist look okay? (Y/N)"
    read ANS
done

if [ "$ANS" != "Y" ]
then
    exit -1;
fi

cd ..
tar -cjf slony1-$REL_VERSION-docs.tar.bz2 slony1-$REL_VERSION/doc/adminguide/*html slony1-$REL_VERSION/doc/adminguide/man[0-9] slony1-$REL_VERSION/doc/adminguide/*png slony1-$REL_VERSION/doc/adminguide/*css slony1-$REL_VERSION/doc/adminguide/slony.pdf
cd slony1-$REL_VERSION
make distclean
cd ..
tar -cjf slony1-$REL_VERSION.tar.bz2 slony1-$REL_VERSION

