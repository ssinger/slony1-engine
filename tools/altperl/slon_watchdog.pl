#!@@PERL@@ # -*- perl -*-
# $Id: slon_watchdog.pl,v 1.6 2005-02-02 17:22:29 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$SLON_ENV_FILE  = 'slon.env'; # Where to find the slon.env file
$SHOW_USAGE     = 0;          # Show usage, then quit

# Read command-line options
GetOptions("config=s"  => \$SLON_ENV_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_watchdog.pl [--config file] node# sleep_seconds

    --config file  Location of the slon.env file (default: Perl's \@INC)

    sleep_seconds  Number of seconds for the watchdog process to sleep
                   between checks

";

if ($SHOW_USAGE or scalar(@ARGV) != 2) {
  die $USAGE;
}

require 'slon-tools.pm';
require $SLON_ENV_FILE;

$node = $ARGV[0];
$sleep = $ARGV[1];

if ($node =~/^(?:node)?(\d+)$/) {
  $nodenum = $1;
} else {
  die $USAGE;
}

while (1) {
  $pid = get_pid($node);
  if (!($pid)) {
    my ($dsn, $dbname) = ($DSN[$nodenum], $DBNAME[$nodenum]);
    open (SLONLOG, ">>$LOGDIR/slon-$dbname-$node.err");
    print SLONLOG "WATCHDOG: No Slon is running for node $node!\n";
    print SLONLOG "WATCHDOG: You ought to check the postmaster and slon for evidence of a crash!\n";
    print SLONLOG "WATCHDOG: I'm going to restart slon for $node...\n";
    # First, restart the node using slonik
    system "./restart_node.sh $node";
    # Next, restart the slon process to service the node
    start_slon($nodenum);
    $pid = get_pid($node);
    print SLONLOG "WATCHDOG: Restarted slon for the $CLUSTER_NAME cluster, PID $pid\n";
  } else {
    open(LOG, ">>$LOGDIR/slon_watchdog.log");
    print LOG "\n";
    system "date >> $LOGDIR/slon_watchdog.log";
    print LOG "Found slon daemon running for the $CLUSTER_NAME cluster, PID $pid\n";
    print LOG "Looks Ok\n";
    print LOG "Sleeping for $sleep seconds\n";
  }
  close(PSOUT);
  sleep $sleep;
}
