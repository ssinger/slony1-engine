#!@@PERL@@
# $Id: slony_show_configuration.pl,v 1.1.4.2 2009-08-17 17:39:58 devrim Exp $
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

# This script simply displays an overview of node configuration
# for a given SLONY node set

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: show_configuration [--config file]

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

print "Slony Configuration\n-------------------------------------\n";
if ($ENV{"SLONYNODES"}) {
  print "With node configuration from ", $ENV{"SLONYNODES"}, "\n";
}
if ($ENV{"SLONYSET"}) {
  print "With set configuration from ", $ENV{"SLONYSET"}, "\n";
}

print qq{
Slony-I Cluster: $CLUSTER_NAME
Logs stored under $LOGDIR
Slony Binaries in: @@SLONBINDIR@@
};
if ($APACHE_ROTATOR) {
  print "Rotating logs using Apache Rotator: $APACHE_ROTATOR\n";
}
print qq{
Node information
--------------------------------
};
foreach $node (@NODES) {
  printf("Node: %2d Host: %15s User: %8s Port: %4d Forwarding? %4s Parent: %2d Database: %10s\n         DSN: %s\n",
	 $node, $HOST[$node], $USER[$node], $PORT[$node], $NOFORWARD[$node],
	 $PARENT[$node], $DBNAME[$node], $DSN[$node]);
}
