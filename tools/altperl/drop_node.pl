#!@@PERL@@
# $Id: drop_node.pl,v 1.7 2005-02-10 04:32:49 smsimms Exp $
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
      drop node (id = $node, event node = $MASTERNODE);
} on error {
      echo 'Failed to drop node $node from cluster';
      exit 1;
}
echo 'dropped node $node cluster';
};
close SLONIK;
run_slonik_script($OUTPUTFILE);
