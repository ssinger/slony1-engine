#!@@PERL@@
# $Id: update_nodes.pl,v 1.5 2005-02-10 06:22:41 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require '@@PGLIBDIR@@/slon-tools.pm';
require '@@SYSCONFDIR@@/slon_tools.conf';

open(SLONIK, ">/tmp/update_nodes.$$");
print SLONIK genheader();

foreach my $node (@NODES) {
  print SLONIK "update functions (id = $node);\n";
};
close SLONIK;
run_slonik_script("/tmp/update_nodes.$$");
