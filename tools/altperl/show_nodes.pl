#!perl # -*- perl -*-
# $Id: show_nodes.pl,v 1.1.2.1 2004-09-30 17:37:28 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada
my @COST;
my  @PATH;

use Getopt::Long;

require 'slon-tools.pm';
require 'slon.env';
my $FILE="/tmp/init-cluster.$$";
open(SLONIK, ">$FILE");

print SLONIK genheader();

my ($dbname, $dbhost)=($DBNAME[1], $HOST[1]);
print SLONIK "
   init cluster (id = 1, comment = 'Node $dbname\@$dbhost');
";
close SLONIK;
run_slonik_script($FILE);

open(SLONIK, ">$FILE");
print SLONIK genheader();

foreach my $node (@NODES) {
  if ($node > 1) {		# skip the first one; it's already initialized!
    my ($dbname, $dbhost) = ($DBNAME[$node], $HOST[$node]);
    print SLONIK "   store node (id = $node, comment = 'Node $node - $dbname\@$dbhost');\n";
  }
}

print SLONIK "echo 'Set up replication nodes';
";
close SLONIK;
run_slonik_script($FILE);

open(SLONIK, ">$FILE");
print SLONIK genheader();

my @VIA ;
generate_listen_paths();
report_on_paths();
print SLONIK qq[
echo 'Next: configure paths for each node/origin';
];
foreach my $nodea (@NODES) {
  my $dsna = $DSN[$nodea];
  foreach my $nodeb (@NODES) {
    if ($nodea != $nodeb) {
      my $dsnb = $DSN[$nodeb];
      my $providerba = $VIA[$nodea][$nodeb];
      my $providerab = $VIA[$nodeb][$nodea];
      if (!$printed[$nodea][$nodeb]) {
	print SLONIK "      store path (server = $nodea, client = $nodeb, conninfo = '$dsna');\n";
	$printed[$nodea][$nodeb] = "done";
      }
      if (!$printed[$nodeb][$nodea]) {
	print SLONIK "      store path (server = $nodeb, client = $nodea, conninfo = '$dsnb');\n";
	$printed[$nodeb][$nodea] = "done";
      }
      print SLONIK "echo 'configured path between $nodea and $nodeb';\n";
    }
  }
}

close SLONIK;

run_slonik_script($FILE);

open(SLONIK, ">$FILE");
print SLONIK genheader();

foreach my $origin (@NODES) {
  my $dsna = $DSN[$origin];
  foreach my $receiver (@NODES) {
    if ($origin != $receiver) {
      my $provider = $VIA[$origin][$receiver];
      print SLONIK "      store listen (origin = $origin, receiver = $receiver, provider = $provider);\n";
    }
  }
}

print SLONIK qq[
        echo 'Replication nodes prepared';
        echo 'Please start the replication daemon on both systems';
];

close SLONIK;
run_slonik_script($FILE);

sub generate_listen_paths {
  my @COST;
  my @PATH;

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
  print "cost\n";
  print "    ";
  foreach my $node2 (@NODES) {
    printf "%4d|", $node2;
  }
  print "\n--------------------------------------------\n";
  foreach my $node1 (@NODES) {
    printf "%4d|", $node1;
    foreach my $node2 (@NODES) {
      if ($COST[$node2][$node1] == $infinity) {
	printf "inf  ";
      } else {
	printf "%4d ", $COST[$node2][$node1];
      }
      print "\n";
    }
  }
  print "\n\n";
  print "VIA\n";
  print "    ";
  foreach my $node2 (@NODES) {
    printf "%4d|", $node2;
  }
  print "\n--------------------------------------------\n";
  foreach my $node1 (@NODES) {
    printf "%4d", $node1;
    foreach my $node2 (@NODES) {
      printf "%4d ", $VIA[$node2][$node1];
    }
    print "\n";
  }

  print "PATHS\n";
  print "    ";
  foreach my $node2 (@NODES) {
    printf "%4d|", $node2;
  }
  print "\n--------------------------------------------\n";
  foreach my $node1 (@NODES) {
    printf "%4d", $node1;
    foreach my $node2 (@NODES) {
      printf "%4d ", $PATH[$node2][$node1];
    }
    print "\n";
  }
}
