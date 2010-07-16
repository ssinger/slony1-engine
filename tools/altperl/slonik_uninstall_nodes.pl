#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

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

Restores all tables to the unlocked state, with all original user       
triggers, constraints and rules, eventually added Slony-I specific      
serial key columns dropped and the Slony-I schema dropped. The node     
becomes a standalone database. The data is left untouched.              
 
The difference between UNINSTALL NODE and DROP NODE is that all         
UNINSTALL NODE does is to remove the Slony-I configuration; it doesn't  
drop the node's configuration from replication.                         

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my $slonik = '';
$slonik .= genheader();
foreach my $node (@NODES) {
    next if $node == $MASTERNODE; # Do this one last
    $slonik .= "  uninstall node (id=$node);\n";
}
$slonik .= "  uninstall node (id=$MASTERNODE);\n";

run_slonik_script($slonik, 'UNINSTALL NODE');
