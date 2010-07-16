#!/usr/bin/perl
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

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

require '@@PERLSHAREDIR@@/slon-tools.pm';
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

my $slonik = '';
$slonik .= genheader();
foreach my $node (@nodes) {
    $slonik .= "  restart node $node;\n";
}

run_slonik_script($slonik, 'RESTART NODE');
