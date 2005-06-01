#!@@PERL@@
# $Id: slonik_drop_node.pl,v 1.1 2005-05-31 16:11:05 cbbrowne Exp $
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

my $FILE="/tmp/slonik-drop.$$";
open(SLONIK, ">", $FILE);
print SLONIK genheader();
print SLONIK "  try {\n";
print SLONIK "      drop node (id = $node, event node = $MASTERNODE);\n";
print SLONIK "  } on error {\n";
print SLONIK "      echo 'Failed to drop node $node from cluster';\n";
print SLONIK "      exit 1;\n";
print SLONIK "  }\n";
print SLONIK "  echo 'dropped node $node cluster';\n";
close SLONIK;
run_slonik_script($FILE);
