#!@@PERL@@
# $Id: restart_nodes.pl,v 1.6 2005-02-10 06:22:41 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require '@@PGLIBDIR@@/slon-tools.pm';
require '@@SYSCONFDIR@@/slon_tools.conf';

my $FILE="/tmp/restart.$$";
foreach my $node (@NODES) {
  my $dsn = $DSN[$node];
  open(SLONIK, ">$FILE");
  print SLONIK qq{
	cluster name = $CLUSTER_NAME ;
	node $node admin conninfo = '$dsn';
	restart node $node;
    };
  close SLONIK;
  run_slonik_script($FILE);
}
