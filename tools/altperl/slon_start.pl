#!/usr/bin/perl
# $Id: slon_start.pl,v 1.4 2004-08-12 22:14:32 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

#start the slon daemon
require 'slon-tools.pm';
require 'slon.env';
$SLEEPTIME=30;   # number of seconds for watchdog to sleep

$node =$ARGV[0];

if ( scalar(@ARGV) < 1 ) {
  die "Usage: ./slon_start [node]\n";
}

if ($node =~ /^node(\d+)$/) {
  # Node name is in proper form
  $nodenum = $1;
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./slon_start [node]\n";
}

$pid = get_pid($node);

if ($pid) {
  die "Slon is already running for set $SETNAME!\n";
}

my $dsn = $DSN[$nodenum];
my $dbname=$DBNAME[$nodenum];
start_slon($nodenum);

$pid = get_pid($node);

if (!($pid)) {
  print "Slon failed to start for cluster $SETNAME, node $node\n";
} else {
  print "Slon successfully started for cluster $SETNAME, node $node\n";
  print "PID [$pid]\n";
  print "Start the watchdog process as well...\n";
  system "perl slon_watchdog.pl $node $SLEEPTIME &";
}
