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

die $USAGE unless $set;
$set = get_set($set) or die "Non-existent set specified.\n";

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
