#!@@PERL@@
# 
# Author: Gurjeet Singh

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
         "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: add_node [--config file] node# event_node#

    adds a node to the cluster.

event_node is the node number of the currnet origin node.
";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($addnode, $current_origin) = @ARGV;
if ($addnode =~ /^(?:node)?(\d+)$/) {
  $addnode = $1;
} else {
  die $USAGE;
}

if ($current_origin =~ /^(?:node)?(\d+)$/) {
  $current_origin = $1;
} else {
  die $USAGE;
}

my ($dbname, $dbhost) = ($DBNAME[$addnode], $HOST[$addnode]);

my $slonik = '';

$slonik .=  "\n# ADD NODE\n";
$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "     store node (id = $addnode, event node = $current_origin, comment = 'Node $addnode - $dbname\@$dbhost');\n";
$slonik .= "  } on error {\n";
$slonik .= "      echo 'Failed to add node $node to cluster';\n";
$slonik .= "      exit 1;\n";
$slonik .= "  }\n";

# STORE PATH
$slonik .=  "\n# STORE PATHS\n";
foreach my $node (@NODES) {
  my $adddsn = $DSN[$addnode];
  if ($node != $addnode) {       # skip the master node; it's already initialized!
    my ($dbname, $dbhost, $nodedsn) = ($DBNAME[$node], $HOST[$node], $DSN[$node]);
    $slonik .= "  store path (server = $addnode, client = $node, conninfo = '$adddsn');\n";
    $slonik .= "  store path (server = $node, client = $addnode, conninfo = '$nodedsn');\n";
  }
}

$slonik .= "  echo 'added node $addnode to cluster';\n";
$slonik .= "  echo 'Please start a slon replication daemon for node $addnode';\n";

run_slonik_script($slonik, 'ADD NODE');


