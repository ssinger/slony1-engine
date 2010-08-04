#!/bin/bash
# 
# This script downloads email archives from gBorg
# Parameters:
#  1.  If need be, change ARCHHOME to be some more suitable location for mailing list archives
#  2.  Optional single argument: INIT
#
#      If you run "bash pull-gborg-mail.sh INIT", it will go through
#      all the years of the Slony-I project at gBorg, and download
#      each month's file.
#
#      If you pass no value, it will pull the current month's email
#      archives, overwriting the existing copies.

ARCHHOME=$HOME/Slony-I/MailingListArchives

ARG=$1

if [ x$ARG = "xINIT" ] ; then
   for year in 2004 2005 2006; do
     for month in January February March April May June July August September October November December; do
       for arch in commit general; do
	  DIR=${ARCHHOME}/${arch}
	  mkdir -p $DIR
	  wget -O $DIR/${year}-${month}.txt http://gborg.postgresql.org/pipermail/slony1-${arch}/${year}-${month}.txt
       done
     done
   done
else
    for year in `date +"%Y"`; do
	for month in `date +"%B"`; do
	    for arch in commit general; do
		DIR=${ARCHHOME}/${arch}
		mkdir -p $DIR
		wget -O $DIR/${year}-${month}.txt http://gborg.postgresql.org/pipermail/slony1-${arch}/${year}-${month}.txt
	    done
	done
    done
fi
