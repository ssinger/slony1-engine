#!/usr/bin/perl
# $Id: slon_watchdog.pl,v 1.1 2004-07-25 04:02:51 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

$node =$ARGV[0];
$sleep =$ARGV[1];

if ( scalar(@ARGV) < 2 ) {
  die "Usage: ./slon_watchdog node sleep-time\n";
}

slon_watchdog();

sub slon_watchdog {
  get_pid();
  if (!($pid)) {
    if ($node eq "node1") {
      open (SLONLOG, ">>$LOGDIR/slon-$DBNAME1.out");
      print SLONLOG "WATCHDOG: No Slon is running for set $SETNAME!\n";
      print SLONLOG "WATCHDOG: You ought to check the postmaster and slon for evidence of a crash!\n";
      print SLONLOG "WATCHDOG: I'm going to restart slon for $node...\n";
      #first restart the node
      system "./restart_node.sh";
      system "$SLON_BIN_PATH/slon $SETNAME -s 1000 -d2 'dbname=$DBNAME1 port=$DBPORT1' 2>$LOGDIR/slon-$DBNAME1.err >$LOGDIR/slon-$DBNAME1.out &";
      get_pid();
      print SLONLOG "WATCHDOG: Restarted slon for set $SETNAME, PID $pid\n";
    } elsif ($node eq "node2") {
      open (SLONLOG, ">>$LOGDIR/slon-$DBNAME2.out");
      print SLONLOG "WATCHDOG: No Slon is running for set $SETNAME!\n";
      print SLONLOG "WATCHDOG: You ought to check the postmaster and slon for evidence of a crash!\n";
      print SLONLOG "WATCHDOG: I'm going to restart slon for $node...\n";
      #first restart the node
      system "./restart_node.sh";
      system "$SLON_BIN_PATH/slon $SETNAME -s 1000 -d2 'dbname=$DBNAME2 port=$DBPORT2' 2>$LOGDIR/slon-$DBNAME2.err >$LOGDIR/slon-$DBNAME2.out &";
      get_pid();
      print SLONLOG "Restarted slon for set $SETNAME, PID $pid\n";
    }
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

sub get_pid {
  open(PSOUT, "ps -auxww | grep -v grep | grep \"slon $SETNAME\" | sort -n | awk '{print \$2}'|");
  $pid = <PSOUT>;
  chop $pid;
  close(PSOUT);
}
