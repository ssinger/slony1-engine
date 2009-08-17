#!@@PERL@@
# $Id: slon_watchdog2.pl,v 1.9.4.2 2009-08-17 17:39:58 devrim Exp $
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

require '@@PGLIBDIR@@/slon-tools.pm';
require '@@SYSCONFDIR@@/slon_tools.conf';

$node =$ARGV[0];
$sleep =$ARGV[1];

if ( scalar(@ARGV) < 2 ) {
  die "Usage: ./slon_watchdog node sleep-time\n";
}

if ($node =~/^node(\d+)$/) {
  $nodenum = $1;
}

log_to_watchdog_log("Invoking watchdog for $CLUSTER_NAME node $nodenum");
while (1) {
  my $res = query_slony_status($nodenum);    # See where the node stands
  my $eventsOK;
  if ($res =~ /^\s*t\s*\|/) {
    $eventsOK = "YES";
  } else {
    $res = node_is_subscribing();
    if ($res =~ /SUBSCRIBE_SET/) {
      $eventsOK = "YES";
    } else {
      $eventsOK = "NO";
    }
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
    # See if the slon log ends with "FATAL  localListenThread: Another slon daemon is serving this node already"
    my $lastlog=`/bin/ls -t $LOGDIR/slony1/node$nodenum/$dbname*log | head -1`;
    my $lastline=`tail -1 $lastlog`;
    if ($lastline =~ /Another slon daemon is serving this node already/) {
      $kick = "YES";   # Yup, need to tell slonik to reset this node
    }
  }

  # If the node needs a swift kick in the "RESTART", then submit that to slonik
  if ($kick eq "YES") {
    log_to_watchdog_log("submit slonik to restart $CLUSTER_NAME node $nodenum");
    open(SLONIK, "|@@SLONBINDIR@@/slonik");
    print SLONIK genheader();
    print SLONIK "restart node $node\n";
    close SLONIK;
  }
  if ($restart eq "YES") {
    if ($pid) {
      log_to_watchdog_log("terminate slon daemon for $CLUSTER_NAME node $nodenum");
      # Kill slon until dead...
      kill 2, $pid;
      sleep 3;
      kill 15, $pid;
      sleep 3;
      kill 9, $pid;
    }
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
