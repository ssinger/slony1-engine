#!/usr/bin/perl
# $Id: drop_node.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
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

open(SLONIK, ">/tmp/slonik-drop.$$");
print SLONIK genheader();
print SLONIK qq{
        try {
                drop node (id = $node);
        }
        on error {
                echo 'Failed to drop node $node from cluster';
                exit 1;
        }
        echo 'dropped node $node cluster';
};
close SLONIK;
print `slonik < /tmp/slonik-drop.$$`;
unlink("/tmp/slonik-drop.$$");
