#!@@PERL@@
# $Id: unsubscribe_set.pl,v 1.7 2005-02-22 16:51:10 smsimms Exp $
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

require '@@PGLIBDIR@@/slon-tools.pm';
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

open(SLONIK, ">", "/tmp/slonik-unsubscribe.$$");
print SLONIK genheader();
print SLONIK "  try {\n";
print SLONIK "      unsubscribe set (id = $set, receiver = $node);\n";
print SLONIK "  }\n";
print SLONIK "  on error {\n";
print SLONIK "      echo 'Failed to unsubscribe node $node from set $set';\n";
print SLONIK "      exit 1;\n";
print SLONIK "  }\n";
print SLONIK "  echo 'unsubscribed node $node from set $set';\n";
close SLONIK;
run_slonik_script("/tmp/slonik-unsubscribe.$$");
