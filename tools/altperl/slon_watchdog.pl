#!perl # -*- perl -*-
# $Id: slon_watchdog.pl,v 1.3 2004-09-09 17:04:08 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

$node =$ARGV[0];
$sleep =$ARGV[1];

if ( scalar(@ARGV) < 2 ) {
  die "Usage: ./slon_watchdog node sleep-time\n";
}

if ($node =~/^node(\d+)$/) {
  $nodenum = $1;
}

slon_watchdog($node, $nodenum);

sub slon_watchdog {
  my ($node, $nodenum) = @_;
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
    print SLONLOG "WATCHDOG: Restarted slon for set $SETNAME, PID $pid\n";
  } else {
    open(LOG, ">>$LOGDIR/slon_watchdog.log");
    print LOG "\n";
    system "date >> $LOGDIR/slon_watchdog.log";
    print LOG "Found slon daemon running for set $SETNAME, PID $pid\n";
    print LOG "Looks Ok\n";
    print LOG "Sleeping for $sleep seconds\n";
  }
  close(PSOUT);
  sleep $sleep;
  slon_watchdog();
}
