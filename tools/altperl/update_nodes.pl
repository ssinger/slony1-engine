#!@@PERL@@ # -*- perl -*-
# $Id: update_nodes.pl,v 1.3 2005-01-26 19:42:26 darcyb Exp $
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
