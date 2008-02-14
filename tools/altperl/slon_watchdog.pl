#!@@PERL@@
# $Id: slon_watchdog.pl,v 1.14 2008-02-14 16:41:35 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_watchdog [--config file] node# sleep_seconds

    --config file  Location of the slon_tools.conf file

    sleep_seconds  Number of seconds for the watchdog process to sleep
                   between checks

";

if ($SHOW_USAGE or scalar(@ARGV) != 2) {
  die $USAGE;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

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
    system "@@TOOLSBIN@@/slonik_restart_node $node | @@SLONBINDIR@@/slonik";
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
    print LOG "Sleeping for $sleep +/- " . int($sleep/2) . " seconds\n";
  }
  close(PSOUT);
  sleep $sleep + (rand($sleep) - $sleep/2);
}
