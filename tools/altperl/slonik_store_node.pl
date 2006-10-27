#!@@PERL@@
# $Id: slonik_store_node.pl,v 1.3 2006-10-27 17:52:10 cbbrowne Exp $
# Author: Steve Simms
# Copyright 2005 PostgreSQL Global Development Group

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

require '@@PGLIBDIR@@/slon-tools.pm';
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

my @COST;
my @VIA;
my @PATH;
generate_listen_paths();

$slonik .= "  echo 'Next: configure paths for each node/origin';\n";
foreach my $nodea (@NODES) {
    my $dsna = $DSN[$nodea];
    foreach my $nodeb (@NODES) {
	if ($nodea != $nodeb) {
	    next unless ($node == $nodea or $node == $nodeb);
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

$slonik .= "  echo 'Replication nodes prepared';\n";
$slonik .= "  echo 'Please start a slon replication daemon for each node';\n";

run_slonik_script($slonik, 'STORE NODE');
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
	if ((not ($PARENT[$node1] or $PARENT[$node2])) or
	    ($PARENT[$node1] and $PARENT[$node1] == $node2) or
	    ($PARENT[$node2] and $PARENT[$node2] == $node1)) {
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

		$VIA[$node1][$node3] = $node2;
		$COST[$node1][$node3] = $newcost;
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
