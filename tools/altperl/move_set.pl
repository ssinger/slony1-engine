#!@@PERL@@
# $Id: move_set.pl,v 1.6 2005-02-10 04:32:49 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($set, $node1, $node2) = @ARGV;
if ($set =~ /^set(\d+)$/) {
  # Node name is in proper form
  $set = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die "Usage: ./move_set.pl setN nodeOLD nodeNEW\n";
}

if ($node1 =~ /^node(\d+)$/) {
  $node1 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./move_set.pl setN nodeOLD nodeNEW\n";
}
if ($node2 =~ /^node(\d+)$/) {
  $node2 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./move_set.pl setN nodeOLD nodeNEW\n";
}

open(SLONIK, ">/tmp/slonik.$$");
print SLONIK genheader();
my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK qq[
        try {
                echo 'Locking down set $set on node $node1';
                lock set (id = $set, origin = $node1);
                echo 'Locked down - moving it';
                move set (id = $set, old origin = $node1, new origin = $node2);
        }
        on error {
                echo 'Failure to move set $set from $node1 to $node2';
                unlock set (id = $set, origin = $node1);
                exit 1;
        }
        echo 'Replication set $set moved from node $node1 to $node2';
];

close SLONIK;
run_slonik_script("/tmp/slonik.$$");
