#!/usr/bin/perl
# $Id: update_node.pl,v 1.2 2004-08-03 18:15:06 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

open(SLONIK, ">/tmp/update_nodes.$$");
print SLONIK genheader();

foreach my $node (@NODES) {
  print SLONIK "update functions (id = $node);\n";
};
close SLONIK;
print `slonik /tmp/update_nodes.$$`;
