#!@@PERL@@
#
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

# This script simply displays an overview of node configuration
# for a given SLONY node set

use Getopt::Long;
use Switch;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
           "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: slony_show_configuration [--config file] [node# nodeproperty]

  nodeproperty valid values:
        host : return host
        port : return connection port
        user : return connection user
        dbname : return connecton database name
        noforward : return noforward configuration
        parent : return parent node
        dsn : return dsn connection string
        cluster : return cluster name
        node-config-file : return node config file name
        node-config-file-quotemeta : retun quoted node config file name
        config-file : return slon-tools config file name
        config-file-quotemeta : retun quoted slon-tools config file name

";

if ($SHOW_USAGE) {
  die $USAGE;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

if ( scalar(@ARGV) == 0 ) {

  print_configurations();

} elsif ( scalar(@ARGV) == 2 ) {
    $nodenum = $ARGV[0];
    $nodeproperty = $ARGV[1];

    if ($nodenum =~ /^(?:node)?(\d+)$/) {
      $nodenum = $1;
    }
    print_property_value();

} else {
  die $USAGE;
}

sub print_configurations {
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
  for $node (@NODES) {
    printf("Node: %2d Host: %15s User: %8s Port: %4d Forwarding? %4s Parent: %2d Database: %10s\n         DSN: %s\n",
         $node, $HOST[$node], $USER[$node], $PORT[$node], $NOFORWARD[$node],
         $PARENT[$node], $DBNAME[$node], $DSN[$node]);
  }
}

sub print_property_value {
  switch($nodeproperty) {
    # connection configs
    case "host"      { print $HOST[$nodenum], "\n"; }
    case "port"      { print $PORT[$nodenum], "\n"; }
    case "user"      { print $USER[$nodenum], "\n"; }
    case "dbname"    { print $DBNAME[$nodenum], "\n"; }
    case "dsn"       { print $DSN[$nodenum], "\n"; }
    # replication configs
    case "parent"    { print $PARENT[$nodenum], "\n"; }
    case "noforward" { print $NOFORWARD[$nodenum], "\n"; }
    # Misc
    case "cluster"   { print $CLUSTER_NAME, "\n"; }
    # config files
    case "node-config-file"           { print $CONFIG[$nodenum], "\n"; }
    case "node-config-file-quotemeta" { print quotemeta( $CONFIG[$nodenum] ), "\n"; }
    case "config-file"           { print $CONFIG_FILE, "\n"; }
    case "config-file-quotemeta" { print quotemeta( $CONFIG_FILE ), "\n"; }
    else                         { print $USAGE; }
  }
}

