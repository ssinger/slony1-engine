#!perl # -*- perl -*-
# $Id: subscribe_set.pl,v 1.3 2004-09-09 17:04:08 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
my ($set, $node) = @ARGV;
if ($node =~ /^node(\d+)$/) {
  $node = $1;
} else {
  print "Need to specify node!\n";
  die "subscribe_set setM nodeN\n";
}

if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Need to specify set!\n";
  die "subscribe_set setM nodeN\n";
}

$FILE="/tmp/slonik-subscribe.$$";
open(SLONIK, ">$FILE");
print SLONIK genheader();
print SLONIK "try {\n";

if ($DSN[$node]) {
  my $parent = 1;
  my $forward;
  if ($PARENT[$node]) {
    $parent = $PARENT[$node];
  }
  if ($NOFORWARD[$node] eq "yes") {
    $forward = "no";
  } else {
    $forward = "yes";
  }
  print SLONIK "   subscribe set (id = $set, provider = $parent, receiver = $node, forward = $forward);\n";
} else {
  die "Node $node not found\n";
}

print SLONIK "}\n";
print SLONIK qq{
        on error {
                exit 1;
        }
        echo 'Subscribed nodes to set 1';
};

close SLONIK;
run_slonik_script($FILE);
