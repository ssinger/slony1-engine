#!@@PERL@@
# 
# Kill all slon instances for the current cluster
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$WATCHDOG_ONLY = 0;

# Read command-line options
GetOptions("config=s"   => \$CONFIG_FILE,
	   "help"       => \$SHOW_USAGE,
	   "w|watchdog" => \$WATCHDOG_ONLY);

my $USAGE =
"Usage: slon_kill [--config file] [-w|--watchdog]

    --config file  Location of the slon_tools.conf file

    -w
    --watchdog     Only kill the watchdog process(es)

    Kills all running slon and slon_watchdog on this machine for every
    node in the cluster.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

print "slon_kill.pl...   Killing all slon and slon_watchdog instances for the cluster $CLUSTER_NAME\n";
print "1.  Kill slon watchdogs\n";

# kill the watchdog
my $watchdog_suffix = '_watchdog';
open(PSOUT, ps_args() . " | egrep '[s]lon_watchdog' | sort -n | awk '{print \$2}'|");
shut_off_processes($watchdog_suffix);
$watchdog_suffix = '';
close(PSOUT);
if ($found eq 'n') {
    print "No watchdogs found\n";
}

unless ($WATCHDOG_ONLY) {
    print "\n2. Kill slon processes\n";
    
    # kill the slon daemon
    $found="n";
    open(PSOUT, ps_args() . " | egrep \"[s]lon .*$CLUSTER_NAME\" | sort -n | awk '{print \$2}'|");
    shut_off_processes($watchdog_suffix);
    close(PSOUT);
    if ($found eq 'n') {
	print "No slon processes found\n";
    }
}

sub shut_off_processes($) {
    my $watchdog_suffix=$_;

    $found="n";
    while ($pid = <PSOUT>) {
	chomp $pid;
	if (!($pid)) {
	    print "No slon $watchdog_suffix is running for the cluster $CLUSTER_NAME!\n";
	} else {
	    $found="y";
	    kill 9, $pid;
	    print "slon $watchdog_suffix for cluster $CLUSTER_NAME killed - PID [$pid]\n";
	}
    }
}
