#!perl # -*- perl -*-
# $Id: slon_kill.pl,v 1.4 2004-09-09 17:04:07 cbbrowne Exp $
# Kill all slon instances for the current setname
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

print "slon_kill.pl...   Killing all slon and slon_watchdog instances for setname $SETNAME\n";
print "1.  Kill slon watchdogs\n";
#kill the watchdog

open(PSOUT, ps_args() . " | egrep '[s]lon_watchdog' | sort -n | awk '{print \$2}'|");
$found="n";
while ($pid = <PSOUT>) {
  chomp $pid;
  if (!($pid)) {
    print "No slon_watchdog is running for set $SETNAME!\n";
  } else {
    $found="y";
    kill 9, $pid;
    print "slon_watchdog for set $SETNAME killed - PID [$pid]\n";
  }
}
close(PSOUT);
if ($found eq 'n') {
  print "No watchdogs found\n";
}
print "\n2. Kill slon processes\n";
#kill the slon daemon
$found="n";
open(PSOUT, ps_args() . " | egrep \"[s]lon .*$SETNAME\" | sort -n | awk '{print \$2}'|");
while ($pid = <PSOUT>) {
  chomp $pid;
  if (!($pid)) {
    print "No Slon is running for set $SETNAME!\n";
  } else {
    kill 9, $pid;
    print "Slon for set $SETNAME killed - PID [$pid]\n";
    $found="y";
  }
}
close(PSOUT);
if ($found eq 'n') {
  print "No slon processes found\n";
}
