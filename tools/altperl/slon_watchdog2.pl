#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$WATCHDOG_VERBOSE = 1;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
           "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_watchdog2 [--config file] node# sleep_seconds

    --config file  Location of the slon_tools.conf file

    sleep_seconds  Number of seconds for the watchdog process to sleep
                   between checks

";

if ($SHOW_USAGE or scalar(@ARGV) != 2) {
  die $USAGE;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

$node  = $ARGV[0];
$sleep = $ARGV[1];

if ($node =~ /^(?:node)?(\d+)$/) {
  $node = "node$1";
  $nodenum = $1;
} else {
  die $USAGE;
}

my $logfile = "$LOGDIR/slon-watchdog.log";

log_to_file( $logfile , "Invoking watchdog for $CLUSTER_NAME node $nodenum, sleep time = $sleep +/- " . int($sleep/2) . " seconds");

# When slon daemon is just started, may not have time to start syncronization
# and the watchdog will kill the process with no mercy.
# So sleep to give time to slony try to do their job.
sleep $sleep;

while (1) {
  my $res = query_slony_status($nodenum);    # See where the node stands
  my $eventsOK;
  if ($res =~ /^\s*t\s*\|/) {
    $eventsOK = "YES";
    if ( $WATCHDOG_VERBOSE ) {
      log_to_file( $logfile , "Query_slony_status returns true for $CLUSTER_NAME node $nodenum" );
    }
  } else {
    $res = node_is_subscribing($nodenum);
    if ($res =~ /SUBSCRIBE_SET/) {
      $eventsOK = "YES";
    } else {
      $eventsOK = "NO";
    }
    if ( $WATCHDOG_VERBOSE ) {
      log_to_file( $logfile , "Query_slony_status returns false for $CLUSTER_NAME node $nodenum, node_is_subscribing : $eventsOK" );
    }
  }
  my $pid = get_pid($node);  # See if the slon process is alive
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

#  The message searched isn't generated as bellow anymore...
#  and may be exist another better way to know if the restart of node is necessary
#  so this is a TODO, commenting the code since this is not working
#
#    # See if the slon log ends with "FATAL  localListenThread: Another slon daemon is serving this node already"
#    my $lastlog=`/bin/ls -t $LOGDIR/node$nodenum/$dbname*log | head -1`;
#    my $lastline=`tail -1 $lastlog`;
#    if ($lastline =~ /Another slon daemon is serving this node already/) {
#      $kick = "YES";   # Yup, need to tell slonik to reset this node
#    }

    # Kicking allways as slon_watchdog.pl do
    $kick = "YES";
  }

  # If the node needs a swift kick in the "RESTART", then submit that to slonik
  if ($kick eq "YES") {
    log_to_file($logfile,"submit slonik to restart $CLUSTER_NAME node $nodenum");
    if ($CONFIG_FILE ne "") {
      system "(@@TOOLSBIN@@/slonik_restart_node --config=${CONFIG_FILE} $node | @@SLONBINDIR@@/slonik) >> $logfile 2>> $logfile";
    } else {
      system "(@@TOOLSBIN@@/slonik_restart_node $node | @@SLONBINDIR@@/slonik) >> $logfile 2>> $logfile";
    }
  }
  if ($restart eq "YES") {
    if ($pid) {
      log_to_file($logfile,"terminate slon daemon for $CLUSTER_NAME node $nodenum, PID $pid");
      # Kill slon until dead...
      kill 2, $pid;
      sleep 3;
      kill 15, $pid;
      sleep 3;
      # if killed with 9 the pid file isn´t deleted and the service don´t restart
      # kill 9, $pid;
    }
    log_to_file($logfile,"restart slon for $CLUSTER_NAME node $nodenum");
    start_slon($nodenum);
  }
  sleep $sleep + (rand($sleep) - $sleep/2);
}

sub log_to_file {
  my ($logfile,$message) = @_;
  chomp $message;
  my $date = `date`;
  chomp $date;
  open (SLONLOG, ">>$logfile");
  print SLONLOG $date, "|", $message, "\n";
  close SLONLOG;
}
