#!@@PERL@@
# $Id: slonik_drop_node.pl,v 1.1.4.1 2006-10-27 17:54:21 cbbrowne Exp $
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
"Usage: drop_node [--config file] node#

    Drops a node.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($node) = @ARGV;
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  die $USAGE;
}

my $slonik = '';

$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "      drop node (id = $node, event node = $MASTERNODE);\n";
$slonik .= "  } on error {\n";
$slonik .= "      echo 'Failed to drop node $node from cluster';\n";
$slonik .= "      exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'dropped node $node cluster';\n";

run_slonik_script($slonik, 'DROP NODE');
