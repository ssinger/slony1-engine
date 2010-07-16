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
"Usage: slonik_failover [--config file] dead_node backup_node

    Abandons dead_node, making backup_node the origin for all sets on
    dead_node.

    move_set should be used if dead_node is still available, so that
    transactions are not lost.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
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

my $slonik = '';
$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "      failover (id = $node1, backup node = $node2);\n";
$slonik .= "  } on error {\n";
$slonik .= "      echo 'Failure to fail node $node1 over to $node2';\n";
$slonik .= "      exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'Replication sets originating on $node1 failed over to $node2';\n";

run_slonik_script($slonik, 'FAILOVER');
