#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: drop_node [--config file] node# event_node#

    Drops a node.

node# is the id of the node in config file you wish to remove from cluster.

event_node# is the node number of the current origin node; the one capable of
sending notification to other nodes.
This will lead to Slony-I dropping the triggers (generally that deny the
ability to update data), restoring the \"native\" triggers, dropping the
schema used by Slony-I, and the slon process for that node terminating
itself.
 
As a result, the database should be available for whatever use your     
application makes of the database.                                      
 
This is a pretty major operation, with considerable potential to cause  
substantial destruction; make sure you drop the right node!             
 
The operation will fail if there are any nodes subscribing to the node  
that you attempt to drop, so there is a bit of a failsafe to protect    
you from errors.                                                        
";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($node, $event_node) = @ARGV;
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  die $USAGE;
}

if ($event_node =~ /^(?:node)?(\d+)$/) {
  $event_node = $1;
} else {
  print "Need to specify event node!\n";
  die $USAGE;
}

my $slonik = '';

$slonik .= genheader();
$slonik .= "  drop node (id = $node, event node = $event_node);\n";
$slonik .= "  echo 'dropped node $node cluster';\n";

run_slonik_script($slonik, 'DROP NODE');
