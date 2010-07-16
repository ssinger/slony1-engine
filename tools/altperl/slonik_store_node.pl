#!@@PERL@@
# 
# Author: Steve Simms
# Copyright 2005-2009 PostgreSQL Global Development Group

use Getopt::Long;

# Defaults
my $CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
my $SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: store_node [--config file] node#

    Generates the slonik commands necessary to add a node to a
    cluster.  Also displays a report showing the relationships between
    the various nodes.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

$node = $ARGV[0];

# Node can be passed either as "node1" or just "1"
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  die $USAGE;
}

my $slonik = '';

$slonik .= genheader();

# STORE NODE
$slonik .= "\n# STORE NODE\n";
my ($dbname, $dbhost) = ($DBNAME[$node], $HOST[$node]);
$slonik .= "  store node (id = $node, event node = $MASTERNODE, comment = 'Node $node - $dbname\@$dbhost');\n";
$slonik .= "  echo 'Set up replication nodes';\n";

# STORE PATH
$slonik .= "\n# STORE PATH\n";

$slonik .= "  echo 'Next: configure paths for each node/origin';\n";
foreach my $nodea (@NODES) {
    my $dsna = $DSN[$nodea];
    foreach my $nodeb (@NODES) {
	if ($nodea != $nodeb) {
	    next unless ($node == $nodea or $node == $nodeb);
	    my $dsnb = $DSN[$nodeb];
	    if (!$printed[$nodea][$nodeb]) {
		$slonik .= "  store path (server = $nodea, client = $nodeb, conninfo = '$dsna');\n";
		$printed[$nodea][$nodeb] = "done";
	    }
	    if (!$printed[$nodeb][$nodea] and $providerba == $nodea) {
		$slonik .= "  store path (server = $nodeb, client = $nodea, conninfo = '$dsnb');\n";
		$printed[$nodeb][$nodea] = "done";
	    }
	}
    }
}

$slonik .= "  echo 'Replication nodes prepared';\n";
$slonik .= "  echo 'Please start a slon replication daemon for each node';\n";

run_slonik_script($slonik, 'STORE NODE');
