#!perl # -*- perl -*-
# $Id: update_nodes.pl,v 1.2 2004-09-09 17:04:08 cbbrowne Exp $
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
run_slonik_script("/tmp/update_nodes.$$");
