#!/usr/bin/perl
# $Id: merge_sets.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($node, $set1, $set2) = @ARGV;
if ($node =~ /^node(\d+)$/) {
  # Set name is in proper form
  $node = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./merge_sets.pl nodeN setOLD setNEW\n";
}

if ($set1 =~ /^node(\d+)$/) {
  $set1 = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./merge_sets.pl nodeN setOLD setNEW\n";
}
if ($set2 =~ /^node(\d+)$/) {
  $set2 = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./merge_sets.pl nodeN setOLD setNEW\n";
}

open(SLONIK, ">/tmp/slonik.$$");
print SLONIK genheader();
my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK qq[
        try {
                merge set (id = $set1, add id = $set2, origin = $node);
        }
        on error {
                echo 'Failure to merge sets $set1 and $set2 with origin $node';
                exit 1;
        }
        echo 'Replication set $set2 merged in with $set1 on origin $node';
];

close SLONIK;
`slonik < /tmp/slonik.$$`;
unlink("/tmp/slonik.$$");
