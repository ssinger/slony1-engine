#!@@PERL@@
# $Id: slon_kill.pl,v 1.11 2005-02-22 17:11:18 smsimms Exp $
# Kill all slon instances for the current setname
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s"  => \$CONFIG_FILE,
	   "help"      => \$SHOW_USAGE);

my $USAGE =
"Usage: slon_kill [--config file]

   Kills all running slon and slon_watchdog instances for the set
   specified in the config file.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

print "slon_kill.pl...   Killing all slon and slon_watchdog instances for the cluster $CLUSTER_NAME\n";
print "1.  Kill slon watchdogs\n";
# kill the watchdog

open(PSOUT, ps_args() . " | egrep '[s]lon_watchdog' | sort -n | awk '{print \$2}'|");
shut_off_processes();
close(PSOUT);
if ($found eq 'n') {
  print "No watchdogs found\n";
}
print "\n2. Kill slon processes\n";

# kill the slon daemon
$found="n";
open(PSOUT, ps_args() . " | egrep \"[s]lon .*$CLUSTER_NAME\" | sort -n | awk '{print \$2}'|");
shut_off_processes();
close(PSOUT);
if ($found eq 'n') {
  print "No slon processes found\n";
}

sub shut_off_processes {
  $found="n";
  while ($pid = <PSOUT>) {
    chomp $pid;
    if (!($pid)) {
      print "No slon_watchdog is running for the cluster $CLUSTER_NAME!\n";
    } else {
      $found="y";
      kill 9, $pid;
      print "slon_watchdog for cluster $CLUSTER_NAME killed - PID [$pid]\n";
    }
  }
}
