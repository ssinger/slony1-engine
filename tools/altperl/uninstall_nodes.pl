#!@@PERL@@ # -*- perl -*-
# $Id: uninstall_nodes.pl,v 1.5 2005-02-02 17:22:29 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

$FILE="/tmp/slonik.$$";
open(SLONIK, ">$FILE");
print SLONIK genheader();
print SLONIK qq{
	uninstall node (id=$MASTERNODE);
};
close SLONIK;
run_slonik_script($FILE);

foreach my $node (@NODES) {
  foreach my $command ("drop schema _$CLUSTER_NAME cascade;") {
    print $command, "\n";
    print `echo "$command" | psql -h $HOST[$node] -U $USER[$node] -d $DBNAME[$node] -p $PORT[$node]`;
  }
  foreach my $t (@SERIALTABLES) {
    my $command = "alter table $t drop column \\\"_Slony-I_" . $CLUSTER_NAME . "_rowID\\\";";
    print $command, "\n";
    print `echo "$command" | psql -h $HOST[$node] -U $USER[$node] -d $DBNAME[$node] -p $PORT[$node]`;
  }
}
