#!@@PERL@@
# $Id: drop_set.pl,v 1.10 2005-02-23 20:30:51 smsimms Exp $
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
"Usage: drop_set [--config file] set#

    Drops a set.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set) = @ARGV;
if ($set =~ /^(?:set)?(\d+)$/) {
  $set = $1;
} else {
  print "Need set identifier\n";
  die $USAGE;
}

get_set($set) or die "Non-existent set specified.\n";

$FILE = "/tmp/dropset.$$";
open(SLONIK, ">", $FILE);
print SLONIK genheader();
print SLONIK "  try {\n";
print SLONIK "        drop set (id = $set, origin = $SET_ORIGIN);\n";
print SLONIK "  } on error {\n";
print SLONIK "        exit 1;\n";
print SLONIK "  }\n";
print SLONIK "  echo 'Dropped set $set';\n";
close SLONIK;
run_slonik_script($FILE);
