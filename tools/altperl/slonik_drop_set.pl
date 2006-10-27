#!@@PERL@@
# $Id: slonik_drop_set.pl,v 1.1.4.1 2006-10-27 17:54:21 cbbrowne Exp $
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

my $slonik = '';

$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "        drop set (id = $set, origin = $SET_ORIGIN);\n";
$slonik .= "  } on error {\n";
$slonik .= "        exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'Dropped set $set';\n";

run_slonik_script($slonik, 'DROP SET');
