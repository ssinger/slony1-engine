#!perl # -*- perl -*-
# $Id: slon_watchdog2.pl,v 1.1 2004-09-23 16:03:32 cbbrowne Exp $
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

log_to_watchdog_log("Invoking watchdog for $SETNAME node $nodenum");
while (1) {
  my $res = query_slony_status($nodenum);    # See where the node stands
  my $eventsOK;
  if ($res =~ /^\s*f\s*\|/) {
    $eventsOK = "YES";
  } else {
    $eventsOK = "NO";
  }
  my $pid = get_pid($node);                  # See if the slon process is alive
  my ($restart, $kick);
  $kick = "NO";   # Initially, assume we don't need to submit a "restart node" command
  if ($pid) {  # PID is alive...
    if ($eventsOK eq "YES") {
      # All is well - do nothing!
      $restart = "NO";
    } else {
      $restart = "YES";
    }
  } else {
    $restart = "YES";
  }

  if ($restart eq "YES") {
    if ($pid) {  # process is still alive, but evidently deranged, so it's getting terminated
      log_to_watchdog_log("terminate slon daemon for $SETNAME node $nodenum");
      # Kill slon until dead...
      kill 2, $pid;
      sleep 3;
      kill 15, $pid;
      sleep 3;
      kill 9, $pid;
      sleep 3;
    }
  }
  if ($restart eq "YES") {
    # Now, let's see if there's a lingering pg_listener entry
    my $dead_connections = query_slon_connections($nodenum);
    log_to_watchdog_log("spurious pg_listener entries for $SETNAME node $node - Count=[$dead_connections]");
    if ($dead_connections > 0) {
      $kick = "YES";
    }
  }
  # If the node needs a swift kick in the "RESTART", then submit that to slonik
  if ($kick eq "YES") {
    log_to_watchdog_log("submit slonik to restart $SETNAME node $nodenum");
    open(SLONIK, "|$SLON_BIN_PATH/slonik");
    print SLONIK genheader();
    print SLONIK "restart node $node\n";
    close SLONIK;
  }
  if ($restart eq "YES") {
    log_to_watchdog_log("restart slon for $nodenum");
    start_slon($nodenum);
  }
  sleep $sleep;
}


sub log_to_watchdog_log {
  my ($message) = @_;
  chomp $message;
  my $date = `date`;
  chomp $date;
  open (SLONLOG, ">>$LOGDIR/slony-watchdog.log");
  print SLONLOG $date, "|", $message, "\n";
  close SLONLOG;
}
