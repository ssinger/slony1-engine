#!/usr/bin/perl
# $Id: slon_start.pl,v 1.7 2005-01-25 23:13:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$START_WATCHDOG = 1;          # Whether or not the watchdog process should be started
$SLEEP_TIME     = 30;         # Number of seconds for watchdog to sleep
$SLON_ENV_FILE  = 'slon.env'; # Where to find the slon.env file
$SHOW_USAGE     = 0;          # Show usage, then quit

# Read command-line options
GetOptions("config=s"  => \$SLON_ENV_FILE,
	   "watchdog!" => \$START_WATCHDOG,
	   "sleep=i"   => \$SLEEP_TIME,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_start.pl [--config file] [--watchdog|--nowatchdog]
       [--sleep seconds] node#

    --config file    Location of the slon.env file (default: Perl's \@INC)

    --watchdog       Start a watchdog process after starting the slon
                     daemon (default)

    --nowatchdog     Do not start a watchdog process

    --sleep seconds  Number of seconds for the watchdog process to sleep
                     between checks (default 30)

";

if ($SHOW_USAGE or scalar(@ARGV) != 1) {
  die $USAGE;
}

require 'slon-tools.pm';
require $SLON_ENV_FILE;

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
  die "Slon is already running for the '$SETNAME' set\n";
}

my $dsn    = $DSN[$nodenum];
my $dbname = $DBNAME[$nodenum];
start_slon($nodenum);
$pid = get_pid($node);

unless ($pid) {
  print "Slon failed to start for cluster $SETNAME, node $node\n";
} else {
  print "Slon successfully started for cluster $SETNAME, node $node\n";
  print "PID [$pid]\n";
  if ($START_WATCHDOG) {
    print "Start the watchdog process as well...\n";
    system "perl slon_watchdog.pl --config=$SLON_ENV_FILE $node $SLEEP_TIME &";
  }
}
