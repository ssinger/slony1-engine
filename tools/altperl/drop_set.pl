#!@@PERL@@
# $Id: drop_set.pl,v 1.7 2005-02-10 04:32:49 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

$SLON_ENV_FILE = 'slon.env'; # Where to find the slon.env file
$SHOW_USAGE    = 0;          # Show usage, then quit

# Read command-line options
GetOptions("config=s"  => \$SLON_ENV_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: drop_set.pl [--config file] set#

    Drops a set.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require 'slon-tools.pm';
require $SLON_ENV_FILE;

my ($set) = @ARGV;
if ($set =~ /^(?:set)?(\d+)$/) {
  $set = $1;
} else {
  print "Need set identifier\n";
  die $USAGE;
}

$FILE = "/tmp/dropset.$$";
open(SLONIK, ">", $FILE);
print SLONIK genheader();
print SLONIK "  try {\n";
print SLONIK "        drop set (id = $set, origin = $MASTERNODE);\n";
print SLONIK "  } on error {\n";
print SLONIK "        exit 1;\n";
print SLONIK "  }\n";
print SLONIK "  echo 'Dropped set $set';\n";
close SLONIK;
run_slonik_script($FILE);
