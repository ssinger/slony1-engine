#!/usr/bin/perl
# $Id: slonik_restart_node.pl,v 1.1 2005-05-31 16:11:05 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$ALL_NODES   = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE,
	   "all"      => \$ALL_NODES);

my $USAGE =
"Usage: restart_node [--config file] [--all] [node# ...]

    Restart one or more nodes

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my @nodes;
if ($ALL_NODES) {
    @nodes = @NODES;
}
else {
    foreach my $node (@ARGV) {
	if ($node =~ /^(?:node)?(\d+)$/) {
	    push @nodes, ($1);
	}
	else {
	    die $USAGE;
	}
    }
}

die $USAGE unless scalar @nodes;

my $FILE="/tmp/restart.$$";
open(SLONIK, ">", $FILE);
print SLONIK genheader();
foreach my $node (@nodes) {
    print SLONIK "  restart node $node;\n";
}
close SLONIK;
run_slonik_script($FILE);
