#!/usr/bin/perl
# $Id: failover.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($node1, $node2) = @ARGV;
if ($node1 =~ /^node(\d+)$/) {
  $node1 = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./failover.pl nodeN setOLD setNEW\n";
}
if ($node2 =~ /^node(\d+)$/) {
  $node2 = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./failover.pl nodeN setOLD setNEW\n";
}

open(SLONIK, ">/tmp/slonik.$$");
print SLONIK genheader();
my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK qq[
        try {
                failover (id = $node1, backup node = $node2);
        }
        on error {
                echo 'Failure to fail node $node1 over to $node2';
                exit 1;
        }
        echo 'Replication sets originating on $node1 failed over to $node2';
];

close SLONIK;
`slonik < /tmp/slonik.$$`;
unlink("/tmp/slonik.$$");
