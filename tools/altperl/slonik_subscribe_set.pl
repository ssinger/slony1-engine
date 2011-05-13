#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: subscribe_set [--config file] set# node#

    Begins replicating a set to the specified node.

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
  print "Need to specify node!\n";
  die $USAGE;
}

die $USAGE unless $set;
$set = get_set($set) or die "Non-existent set specified.\n";

my $slonik = '';

$slonik .= genheader();
$slonik .= " \n";

if ($DSN[$node]) {
  my $provider = $SET_ORIGIN;
  my $forward;
  if ($PARENT[$node]) {
    $provider = $PARENT[$node];
  }
  if ($NOFORWARD[$node] eq "yes") {
    $forward = "no";
  } else {
    $forward = "yes";
  }
  $slonik .= "    subscribe set (id = $set, provider = $provider, receiver = $node, forward = $forward);\n";
} else {
  die "Node $node not found\n";
}

$slonik .= "  echo 'Subscribed nodes to set $set';\n";
run_slonik_script($slonik);
