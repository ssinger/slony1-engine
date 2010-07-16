#!/bin/sh
# 
# Run Slony-I Replication Tests

PASS="some secret"
TESTQUERY="select name, created_on from some_table order by id desc limit 1;"
PERL=/usr/bin/perl
SCRIPT=test_slony_replication.pl
HOST=localhost
USER=postgres
$PERL $SCRIPT -database=mydb -host=$HOST -user=$USER -cluster=test -port=5432 -password="$PASS" --finalquery="$TESTQUERY"
