#!@@PERL@@ # -*- perl -*-
# $Id: failover.pl,v 1.5 2005-01-26 19:42:23 darcyb Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($node1, $set1, $node2) = @ARGV;
if ($node1 =~ /^node(\d+)$/) {
  $node1 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./failover.pl nodeN setOLD nodeNEW\n";
}
if ($set1 =~ /^set(\d+)$/) {
  $set1 = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./failover.pl nodeN setOLD nodeNEW\n";
}
if ($node2 =~ /^node(\d+)$/) {
  $node2 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./failover.pl nodeN setOLD nodeNEW\n";
}

open(SLONIK, ">/tmp/slonik.$$");
print SLONIK genheader();
my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK qq[
try {
      failover (id = $node1, backup node = $node2);
} on error {
      echo 'Failure to fail node $node1 over to $node2';
      exit 1;
}
      echo 'Replication sets originating on $node1 failed over to $node2';
];

close SLONIK;
run_slonik_script("/tmp/slonik.$$");
