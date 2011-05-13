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
"Usage: merge_sets [--config file] node# targetset# srcset#

    Merges the contents of the second set into the first one.
    When completed, the second set no longer exists.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($node, $set1, $set2) = @ARGV;
if ($node =~ /^(?:node)?(\d+)$/) {
  # Set name is in proper form
  $node = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die $USAGE;
}

die $USAGE unless $set1;
$set1 = get_set($set1) or die "Non-existent set specified.\n";
die $USAGE unless $set2;
$set2 = get_set($set2) or die "Non-existent set specified.\n";

my ($dbname, $dbhost) = ($DBNAME[$MASTERNODE], $HOST[$MASTERNODE]);

my $slonik = '';

$slonik .= genheader();
$slonik .= "    merge set (id = $set1, add id = $set2, origin = $node);\n";
$slonik .= "  echo 'Replication set $set2 merged into $set1 on origin $node. Set $set2 no longer exists.';\n";

run_slonik_script($slonik, 'MERGE SET');
