#!perl # -*- perl -*-
# $Id: drop_node.pl,v 1.4.2.1 2004-09-30 17:37:28 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($node) = @ARGV;
if ($node =~ /^node(\d+)$/) {
  $node = $1;
} else {
  print "Need to specify node!\n";
  die "drop_node nodeN\n";
}

my $OUTPUTFILE="/tmp/slonik-drop.$$";
open(SLONIK, ">$OUTPUTFILE");
print SLONIK genheader();
print SLONIK qq{
try {
      drop node (id = $node);
} on error {
      echo 'Failed to drop node $node from cluster';
      exit 1;
}
echo 'dropped node $node cluster';
};
close SLONIK;
run_slonik_script($OUTPUTFILE);
