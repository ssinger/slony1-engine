#!/usr/bin/perl
# $Id: slon_start.pl,v 1.3 2004-08-04 16:38:34 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

#start the slon daemon
require 'slon-tools.pm';
require 'slon.env';

$node =$ARGV[0];

if ( scalar(@ARGV) < 1 ) {
  die "Usage: ./slon_start [node]\n";
}

if ($node =~ /^node\d+$/) {
  # Node name is in proper form
} else {
  print "Valid node names are node1, node2, ...\n\n";
  die "Usage: ./slon_start [node]\n";
}

get_pid();

if ($pid) {
  die "Slon is already running for set $SETNAME!\n";
}

$node =~ /node(\d*)$/;
$nodenum = $1;
my $dsn = $DSN[$nodenum];
my $dbname=$DBNAME[$nodenum];
system "$SLON_BIN_PATH/slon -d2 -s 1000 $SETNAME '$dsn' 2>$LOGDIR/slon-$dbname-$node.err >$LOGDIR/slon-$dbname-$node.out &";

get_pid();

if (!($pid)){
  print "Slon failed to start for set $SETNAME!\n";
} else {
  print "Slon successfully started for set $SETNAME\n";
  print "PID [$pid]\n";
}
#start the watchdog process
system " perl slon_watchdog.pl $node 30 &";

sub get_pid {
  $node =~ /node(\d*)$/;
  my $nodenum = $1;
  my ($dbname, $dbport, $dbhost) = ($DBNAME[$nodenum], $PORT[$nodenum], $HOST[$nodenum]);
#  print "Searching for PID for $dbname on port $dbport\n";
  open(PSOUT, "ps -auxww | egrep \"[s]lon $SETNAME\" | egrep \"host=$dbhost dbname=$dbname.*port=$dbport\" | sort -n | awk '{print \$2}'|");
  $pid = <PSOUT>;
  chop $pid;
  close(PSOUT);
}
