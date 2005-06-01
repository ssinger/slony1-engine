#!@@PERL@@
# $Id: slonik_failover.pl,v 1.1 2005-05-31 16:11:05 cbbrowne Exp $
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
"Usage: failover [--config file] dead_node backup_node

    Abandons dead_node, making backup_node the origin for all sets on
    dead_node.

    move_set should be used if dead_node is still available, so that
    transactions are not lost.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($node1, $node2) = @ARGV;
if ($node1 =~ /^(?:node)?(\d+)$/) {
  $node1 = $1;
} else {
  die $USAGE;
}

if ($node2 =~ /^(?:node)?(\d+)$/) {
  $node2 = $1;
} else {
  die $USAGE;
}

open(SLONIK, ">", "/tmp/slonik.$$");
print SLONIK genheader();
print SLONIK "  try {\n";
print SLONIK "      failover (id = $node1, backup node = $node2);\n";
print SLONIK "  } on error {\n";
print SLONIK "      echo 'Failure to fail node $node1 over to $node2';\n";
print SLONIK "      exit 1;\n";
print SLONIK "  }\n";
print SLONIK "  echo 'Replication sets originating on $node1 failed over to $node2';\n";
close SLONIK;
run_slonik_script("/tmp/slonik.$$");
