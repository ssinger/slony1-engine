#!@@PERL@@
# $Id: update_nodes.pl,v 1.6 2005-02-22 16:51:10 smsimms Exp $
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
"Usage: update_nodes.pl [--config file]

    Updates the functions on all nodes.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

open(SLONIK, ">", "/tmp/update_nodes.$$");
print SLONIK genheader();
foreach my $node (@NODES) {
  print SLONIK "  update functions (id = $node);\n";
};
close SLONIK;
run_slonik_script("/tmp/update_nodes.$$");
