#!@@PERL@@
# Author: Devrim GÜNDÜZ <devrim@gunduz.org>
# Copyright 2009 Slony-I Global Development Group

use Getopt::Long;

# Defaults
$CONFIG_FILE    = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE     = 0;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_status [--config file] node#

    --config file    Location of the slon_tools.conf file

";

if ($SHOW_USAGE or scalar(@ARGV) != 1) {
  die $USAGE;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

$node = $ARGV[0];

# Node can be passed either as "node1" or just "1"
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = "node$1";
  $nodenum = $1;
} else {
  die $USAGE;
}

if ( ! $DBNAME[$nodenum] ) {
  die "There is no such node.\n";
}

$pid = get_pid($node);

if ($pid) {
  die "Slon is running for the '$CLUSTER_NAME' cluster on $node.\n";
} else {
  die "slon is not running for $node_name cluster.\n";
}


