#!@@PERL@@ #-*- perl -*-
# $Id: show_configuration.pl,v 1.2 2005-01-26 19:42:24 darcyb Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

# This script simply displays an overview of node configuration
# for a given SLONY node set

require 'slon-tools.pm';
require 'slon.env';

print "Slony Configuration\n-------------------------------------\n";
if ($ENV{"SLONYNODES"}) {
  print "With node configuration from ", $ENV{"SLONYNODES"}, "\n";
}
if ($ENV{"SLONYSET"}) {
  print "With set configuration from ", $ENV{"SLONYSET"}, "\n";
}

print qq{
Slony-I Cluster: $SETNAME
Logs stored under $LOGDIR
Slony Binaries in: $SLON_BIN_PATH
};
if ($APACHE_ROTATOR) {
  print "Rotating logs using Apache Rotator: $APACHE_ROTATOR\n";
}
print qq{
Node information
--------------------------------
};
foreach $node (0..100) {
  if ($DSN[$node]) {
    printf("Node: %2d Host: %15s User: %8s Port: %4d Forwarding? %4s Parent: %2d Database: %10s\n         DSN: %s\n",
	   $node, $HOST[$node], $USER[$node], $PORT[$node], $NOFORWARD[$node],
	   $PARENT[$node], $DBNAME[$node], $DSN[$node]);
  }
}
