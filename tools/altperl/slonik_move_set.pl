#!@@PERL@@
# $Id: slonik_move_set.pl,v 1.1 2005-05-31 16:11:05 cbbrowne Exp $
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
"Usage: move_set [--config file] set# from_node# to_node#

    Change a set's origin.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $node1, $node2) = @ARGV;
if ($set =~ /^(?:set)?(\d+)$/) {
  # Node name is in proper form
  $set = $1;
} else {
  print "Valid set names are set1, set2, ...\n\n";
  die $USAGE;
}

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

open(SLONIK, ">", "/tmp/slonik.$$");
print SLONIK genheader();
print SLONIK "  echo 'Locking down set $set on node $node1';\n";
print SLONIK "  lock set (id = $set, origin = $node1);\n";
print SLONIK "  echo 'Locked down - moving it';\n";
print SLONIK "  move set (id = $set, old origin = $node1, new origin = $node2);\n";
print SLONIK "  echo 'Replication set $set moved from node $node1 to $node2.  Remember to';\n";
print SLONIK "  echo 'update your configuration file, if necessary, to note the new location';\n";
print SLONIK "  echo 'for the set.';\n";
close SLONIK;
run_slonik_script("/tmp/slonik.$$");
