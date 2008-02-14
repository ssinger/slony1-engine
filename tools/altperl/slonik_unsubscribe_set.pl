#!@@PERL@@
# $Id: slonik_unsubscribe_set.pl,v 1.3 2008-02-14 16:41:35 cbbrowne Exp $
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
"Usage: unsubscribe_set [--config file] set# node#

    Stops replicating a set on the specified node.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $node) = @ARGV;
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  print "Need to specify node!\n\n";
  die $USAGE;
}

if ($set =~ /^(?:set)?(\d+)$/) {
  $set = $1;
} else {
  print "Need to specify set!\n\n";
  die $USAGE;
}

my $slonik = '';
$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "      unsubscribe set (id = $set, receiver = $node);\n";
$slonik .= "  }\n";
$slonik .= "  on error {\n";
$slonik .= "      echo 'Failed to unsubscribe node $node from set $set';\n";
$slonik .= "      exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'unsubscribed node $node from set $set';\n";

run_slonik_script($slonik, 'UNSUBSCRIBE SET');
