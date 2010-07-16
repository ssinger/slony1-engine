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
"Usage: move_set [--config file] set# from_node# to_node#

    Change a set's origin.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $node1, $node2) = @ARGV;

die $USAGE unless $set;
$set = get_set($set) or die "Non-existent set specified.\n";

if ($node1 =~ /^(?:node)?(\d+)$/) {
  $node1 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die $USAGE;
}

if ($node2 =~ /^(?:node)?(\d+)$/) {
  $node2 = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die $USAGE;
}

my $slonik = '';

$slonik .= genheader();
$slonik .= "  echo 'Locking down set $set on node $node1';\n";
$slonik .= "  lock set (id = $set, origin = $node1);\n";
$slonik .= "  sync (id = $node1);\n";
$slonik .= "  wait for event (origin = $node1, confirmed = $node2, wait on = $node2);\n";
$slonik .= "  echo 'Locked down - moving it';\n";
$slonik .= "  move set (id = $set, old origin = $node1, new origin = $node2);\n";
$slonik .= "  echo 'Replication set $set moved from node $node1 to $node2.  Remember to';\n";
$slonik .= "  echo 'update your configuration file, if necessary, to note the new location';\n";
$slonik .= "  echo 'for the set.';\n";

run_slonik_script($slonik, 'MOVE SET');
