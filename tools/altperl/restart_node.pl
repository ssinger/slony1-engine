#!/usr/bin/perl
# $Id: restart_node.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

foreach my $node (@NODES) {
    my $dsn = $DSN[$node];
    open(SLONIK, "|slonik");
    print SLONIK qq{
	cluster name = $SETNAME ;
	node $node admin conninfo = '$dsn';
	restart node $node;
    };
    close SLONIK;
}
