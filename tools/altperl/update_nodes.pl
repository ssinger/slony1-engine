#!@@PERL@@
# $Id: update_nodes.pl,v 1.4 2005-02-10 04:32:51 smsimms Exp $
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
