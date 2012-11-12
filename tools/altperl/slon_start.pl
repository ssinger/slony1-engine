#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$START_WATCHDOG = 1;
$SLEEP_TIME     = 30;
$CONFIG_FILE    = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE     = 0;
$WATCHDOG_VERSION = 1;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
	   "watchdog!" => \$START_WATCHDOG,
	   "sleep=i"   => \$SLEEP_TIME,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_start [--config file] [--watchdog|--nowatchdog]
       [--sleep seconds] node#

    --config file    Location of the slon_tools.conf file

    --watchdog       Start a watchdog process after starting the slon
                     daemon (default)

    --nowatchdog     Do not start a watchdog process

    --sleep seconds  Number of seconds for the watchdog process to sleep
                     between checks (default 30)

";

if ($SHOW_USAGE or scalar(@ARGV) != 1) {
  die $USAGE;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

$node = $ARGV[0];

# Node can be passed either as "node1" or just "1"
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = "node$1";
  $nodenum = $1;
} else {
  die $USAGE;
}

$pid = get_pid($node);
if ($pid) {
  die "Slon is already running for the '$CLUSTER_NAME' cluster.\n";
}

my $dsn    = $DSN[$nodenum];
my $dbname = $DBNAME[$nodenum];
start_slon($nodenum);
$pid = get_pid($node);

unless ($pid) {
  print "Slon failed to start for cluster $CLUSTER_NAME, node $node\n";
} else {
  print "Slon successfully started for cluster $CLUSTER_NAME, node $node\n";
  print "PID [$pid]\n";
  if ($START_WATCHDOG) {
    print "Start the watchdog process as well...\n";
    if( $WATCHDOG_VERSION eq 2 ) {
      system "@@TOOLSBIN@@/slon_watchdog2 --config=$CONFIG_FILE $node $SLEEP_TIME &";
    } else {
      system "@@TOOLSBIN@@/slon_watchdog --config=$CONFIG_FILE $node $SLEEP_TIME &";
    }
  }
}
