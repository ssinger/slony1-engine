#!@@PERL@@
# $Id: init_cluster.pl,v 1.11 2005-02-22 17:11:18 smsimms Exp $
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

my @COST;
my @VIA;
my @PATH;
generate_listen_paths();

print SLONIK "  echo 'Next: configure paths for each node/origin';\n";
foreach my $nodea (@NODES) {
  my $dsna = $DSN[$nodea];
  foreach my $nodeb (@NODES) {
    if ($nodea != $nodeb) {
      my $dsnb = $DSN[$nodeb];
      my $providerba = $VIA[$nodea][$nodeb];
      my $providerab = $VIA[$nodeb][$nodea];
      if (!$printed[$nodea][$nodeb]) {
	print SLONIK "  store path (server = $nodea, client = $nodeb, conninfo = '$dsna');\n";
	$printed[$nodea][$nodeb] = "done";
      }
      if (!$printed[$nodeb][$nodea]) {
	print SLONIK "  store path (server = $nodeb, client = $nodea, conninfo = '$dsnb');\n";
	$printed[$nodeb][$nodea] = "done";
      }
      print SLONIK "  echo 'configured path between $nodea and $nodeb';\n";
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
report_on_paths();

sub generate_listen_paths {
  my $infinity = 10000000;	# Initial costs are all infinite
  foreach my $node1 (@NODES) {
    foreach my $node2 (@NODES) {
      $COST[$node1][$node2] = $infinity;
    }
  }

  # Initialize paths between parents and children, and based on them,
  # generate initial seeding of listener paths, @VIA

  foreach my $node1 (@NODES) {
    $COST[$node1][$node1] = 0;
    $VIA[$node1][$node1] = 0;
    foreach my $node2 (@NODES) {
      if ($node2 != $node1) {
	if ($PARENT[$node1] == $node2) {
	  $PATH[$node1][$node2] = 1;
	  $PATH[$node2][$node1] = 1;
	  # Set up a cost 1 path between them
	  # Parent to child
	  $COST[$node1][$node2] = 1;
	  $VIA[$node1][$node2] = $node1;

	  # Child to parent
	  $COST[$node2][$node1] = 1;
	  $VIA[$node2][$node1] = $node2;
	}
      }
    }
  }

  # Now, update the listener paths...
  # 4 level nested iteration:
  # 1 while not done, do
  #   2 for each node, node1
  #     3 for each node, node2, where node2 <> node1, where we don't
  #           yet have a listener path
  #       4 for each node node3 (<> node1 or node2),
  #          consider introducing the listener path:
  #                 node1 to node2 then node2 to node3
  # In concept, it's an O(n^4) algorithm; since the number of nodes, n,
  # is not likely to get particularly large, it's not worth tuning
  # further.
  $didwork = "yes";
  while ($didwork eq "yes") {
    $didwork = "no";
    foreach my $node1 (@NODES) {
      foreach my $node3 (@NODES) {
	if (($VIA[$node3][$node1] == 0) && ($node3 != $node1)) {
	  foreach my $node2 (@NODES) {
	    if ($PATH[$node1][$node2] && ($VIA[$node2][$node3] != 0) && ($node2 != $node3) && ($node2 != $node1)) {
	      # Consider introducing a path from n1 to n2 then n2 to n3
	      # as a cheaper alternative to going direct from n1 to n3
	      my $oldcost = $COST[$node3][$node1];
	      my $newcost = $COST[$node1][$node2] + $COST[$node2][$node3];
	      if ($newcost < $oldcost) {
		$didwork = "yes";
				# So we go via node 2
		$VIA[$node3][$node1] = $node2;
		$COST[$node3][$node1] = $newcost;
	      }
	    }
	  }
	}
      }
    }
  }
}

sub report_on_paths {
  print "# Configuration Summary:\n";
  print "#\n";
  print "# COST\n";
  print "#      ";
  foreach my $node2 (@NODES) {
    printf "| %3d ", $node2;
  }
  print "|\n# ";
  print ("-----+" x (scalar(@NODES) + 1));
  print "\n";
  foreach my $node1 (@NODES) {
    printf "#  %3d ", $node1;
    foreach my $node2 (@NODES) {
      if ($COST[$node2][$node1] == $infinity) {
	printf "| inf ";
      } else {
	printf "|%4d ", $COST[$node2][$node1];
      }
    }
    print "|\n";
  }
  print "# \n";
  print "# VIA\n";
  print "#      ";
  foreach my $node2 (@NODES) {
    printf "| %3d ", $node2;
  }
  print "|\n# ";
  print ("-----+" x (scalar(@NODES) + 1));
  print "\n";
  foreach my $node1 (@NODES) {
    printf "#  %3d ", $node1;
    foreach my $node2 (@NODES) {
      printf "|%4d ", $VIA[$node2][$node1];
    }
    print "|\n";
  }

  print "# \n";
  print "# PATHS\n";
  print "#      ";
  foreach my $node2 (@NODES) {
    printf "| %3d ", $node2;
  }
  print "|\n# ";
  print ("-----+" x (scalar(@NODES) + 1));
  print "\n";
  foreach my $node1 (@NODES) {
    printf "#  %3d ", $node1;
    foreach my $node2 (@NODES) {
      printf "|%4d ", $PATH[$node2][$node1];
    }
    print "|\n";
  }
  print "\n";
}
