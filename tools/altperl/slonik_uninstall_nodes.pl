#!@@PERL@@
# $Id: slonik_uninstall_nodes.pl,v 1.1 2005-05-31 16:11:05 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: uninstall_nodes [--config file]

    Removes Slony configuration from all nodes in a cluster.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

$FILE="/tmp/slonik.$$";
open(SLONIK, ">$FILE");
print SLONIK genheader();
foreach my $node (@NODES) {
    next if $node == $MASTERNODE; # Do this one last
    print SLONIK "  uninstall node (id=$node);\n";
}
print SLONIK "  uninstall node (id=$MASTERNODE);\n";
close SLONIK;
run_slonik_script($FILE);
