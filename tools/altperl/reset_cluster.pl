#!/usr/bin/perl
# $Id: reset_cluster.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

open(SLONIK, ">/tmp/slonik.$$");

print SLONIK genheader();

my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK "
        try {
";

foreach my $node (@NODES) {
    if ($node > 1) {
	my ($dbname, $dbhost) = ($DBNAME[$node], $HOST[$node]);
	print SLONIK "     store node (id = $node, comment = 'Node $dbname@$dbhost');\n";
    }
}

foreach my $nodea (@NODES) {
    my $dsna = $DSN[$nodea];
    foreach my $nodeb (@NODES) {
	if ($nodea != $nodeb) {
	    my $dsnb = $DSN[$nodeb];
	    print SLONIK "      store path (server = $nodea, client = $nodeb, conninfo = '$dsna');\n";
	    print SLONIK "      store path (server = $nodeb, client = $nodea, conninfo = '$dsnb');\n";
	    print SLONIK "      store listen (origin = $nodea, receiver = $nodeb);\n";
	    print SLONIK "      store listen (origin = $nodeb, receiver = $nodea);\n";
	}
    }
}
}
print SLONIK qq[
        }
        on error {
                exit 1;
        }
        echo 'Replication nodes prepared';
        echo 'Please start the replication daemon on both systems';

];

close SLONIK;
`slonik < /tmp/slonik.$$`;
unlink("/tmp/slonik.$$");
