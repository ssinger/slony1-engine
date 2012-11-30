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

if ($node =~ /^(?:node)?(\d+)$/) {
  $node = "node$1";
  $nodenum = $1;
} else {
  die $USAGE;
}

my $logfile = "$LOGDIR/slon-watchdog.log";
my $logfile_err = "$LOGDIR/slon-$CLUSTER_NAME-$node.err";

log_to_file( $logfile , "Invoking watchdog for $CLUSTER_NAME node $nodenum , sleep time = $sleep +/- " . int($sleep/2) . " seconds");

while (1) {
  $pid = get_pid($node);
  if (!($pid)) {
    my ($dsn, $dbname) = ($DSN[$nodenum], $DBNAME[$nodenum]);
    my $message = qq{WATCHDOG: No Slon is running for $CLUSTER_NAME node $nodenum!
You ought to check the postmaster and slon for evidence of a crash!
I'm going to restart slon for $node...
};

    # log to error log as original routine
    log_to_file( $logfile_err , $message );

    # log to watchdog log
    log_to_file( $logfile , "No Slon running for $CLUSTER_NAME node $nodenum, restarting slon");

    # First, restart the node using slonik
    if ($CONFIG_FILE ne "") {
      system "(@@TOOLSBIN@@/slonik_restart_node --config=${CONFIG_FILE} $node | @@SLONBINDIR@@/slonik) >> $logfile_err 2>> $logfile_err";
    } else {
      system "(@@TOOLSBIN@@/slonik_restart_node $node | @@SLONBINDIR@@/slonik) >> $logfile_err 2>> $logfile_err";
    }
    # Next, restart the slon process to service the node
    start_slon($nodenum);
    $pid = get_pid($node);
    log_to_file( $logfile_err , "WATCHDOG: Restarted slon for the $CLUSTER_NAME cluster $node, new PID $pid" );
  } else {
    if ( $WATCHDOG_VERBOSE ) {
      log_to_file( $logfile , "Found slon daemon for $CLUSTER_NAME $node, PID $pid, Sleeping for $sleep +/- " . int($sleep/2) . " seconds" );
    }
  }
  sleep $sleep + (rand($sleep) - $sleep/2);
}

sub log_to_file {
  my ($logfile,$message) = @_;
  chomp $message;
  my $date = `date`;
  chomp $date;

  open (SLONLOG, ">>$logfile");
  print SLONLOG $date, " | ", $message, "\n";
  close SLONLOG;
}

