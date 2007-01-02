#!@@PERL@@
# $Id: slonik_drop_node.pl,v 1.3 2007-01-02 17:12:33 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: drop_node [--config file] node#

    Drops a node.

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

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($node) = @ARGV;
if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  die $USAGE;
}

my $slonik = '';

$slonik .= genheader();
$slonik .= "  try {\n";
$slonik .= "      drop node (id = $node, event node = $MASTERNODE);\n";
$slonik .= "  } on error {\n";
$slonik .= "      echo 'Failed to drop node $node from cluster';\n";
$slonik .= "      exit 1;\n";
$slonik .= "  }\n";
$slonik .= "  echo 'dropped node $node cluster';\n";

run_slonik_script($slonik, 'DROP NODE');
