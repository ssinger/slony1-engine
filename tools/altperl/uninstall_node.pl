#!/usr/bin/perl
# $Id: uninstall_node.pl,v 1.1 2004-07-25 04:02:51 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
#use Pg;
open(SLONIK, "|slonik");
print SLONIK genheader();
print SLONIK qq{
	uninstall node (id=1);
};
close SLONIK;

foreach my $node (@NODES) {
    foreach my $command ("drop schema _$SETNAME cascade;") {
	print $command, "\n";
	print `echo "$command" | psql -h $HOST[$node] -U $USER[$node] -d $DBNAME[$node] -p $PORT[$node]`;
    }
    foreach my $t (@SERIALTABLES) {
	my $command = "alter table $t drop column \\\"_Slony-I_" . $SETNAME . "_rowID\\\";";
	print $command, "\n";
	print `echo "$command" | psql -h $HOST[$node] -U $USER[$node] -d $DBNAME[$node] -p $PORT[$node]`;
    }
}
