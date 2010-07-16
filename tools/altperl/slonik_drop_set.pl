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

my $USAGE = <<EOF
"Usage: drop_set [--config file] set#

    Drops a set.

Much as with DROP NODE, this leads to Slony-I dropping the Slony-I triggers on 
the tables and restoring "native" triggers. One difference is that this takes 
place on all nodes in the cluster, rather than on just one node. Another 
difference is that this does not clear out the Slony-I cluster's namespace, as 
there might be other sets being serviced. 
 
This operation is quite a bit more dangerous than DROP NODE, as there isn't the 
same sort of "failsafe." If you tell DROP SET to drop the wrong set, there 
isn't anything to prevent potentially career-limiting "unfortunate results." 
Handle with care... 

EOF
;

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set) = @ARGV;
die $USAGE unless $set;
$set = get_set($set) or die "Non-existent set specified.\n";

my $slonik = '';

$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "        drop set (id = $set, origin = $SET_ORIGIN);\n";
$slonik .= "  } on error {\n";
$slonik .= "        exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'Dropped set $set';\n";

run_slonik_script($slonik, 'DROP SET');
