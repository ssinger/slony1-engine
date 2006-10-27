#!@@PERL@@
# $Id: slonik_init_cluster.pl,v 1.1.4.2 2006-10-27 17:54:21 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
my $CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
my $SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: init_cluster [--config file]

    Generates the slonik commands necessary to create a cluster and
    prepare the nodes for use.  Also displays a report showing the
    relationships between the various nodes.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my $FILE="/tmp/init-cluster.$$";

# INIT CLUSTER
open(SLONIK, ">", $FILE);
print SLONIK "\n# INIT CLUSTER\n";
print SLONIK genheader();
my ($dbname, $dbhost) = ($DBNAME[$MASTERNODE], $HOST[$MASTERNODE]);
print SLONIK "  init cluster (id = $MASTERNODE, comment = 'Node $MASTERNODE - $dbname\@$dbhost');\n";

# STORE NODE
print SLONIK "\n# STORE NODE\n";
foreach my $node (@NODES) {
  if ($node != $MASTERNODE) {		# skip the master node; it's already initialized!
    my ($dbname, $dbhost) = ($DBNAME[$node], $HOST[$node]);
    print SLONIK "  store node (id = $node, event node = $MASTERNODE, comment = 'Node $node - $dbname\@$dbhost');\n";
  }
}
print SLONIK "  echo 'Set up replication nodes';\n";

# STORE PATH
print SLONIK "\n# STORE PATH\n";

print SLONIK "  echo 'Next: configure paths for each node/origin';\n";
foreach my $nodea (@NODES) {
  my $dsna = $DSN[$nodea];
  foreach my $nodeb (@NODES) {
    if ($nodea != $nodeb) {
      my $dsnb = $DSN[$nodeb];
      my $providerba = $VIA[$nodea][$nodeb];
      my $providerab = $VIA[$nodeb][$nodea];
      if (!$printed[$nodea][$nodeb] and $providerab == $nodea) {
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

# STORE LISTEN
print SLONIK "\n# STORE LISTEN\n";
foreach my $origin (@NODES) {
  my $dsna = $DSN[$origin];
  foreach my $receiver (@NODES) {
    if ($origin != $receiver) {
      my $provider = $VIA[$origin][$receiver];
      print SLONIK "  store listen (origin = $origin, receiver = $receiver, provider = $provider);\n";
    }
  }
}
print SLONIK "  echo 'Replication nodes prepared';\n";
print SLONIK "  echo 'Please start a slon replication daemon for each node';\n";
close SLONIK;
run_slonik_script($FILE);
