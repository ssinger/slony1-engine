#!perl # -*- perl -*-
# $Id: restart_node.pl,v 1.3.2.1 2004-09-30 17:37:28 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($node) = @_;
if ($node =~ /^node(\d+)$/) {
  $nodenum = $node
} else {
  die "./restart_node nodeN\n";
} 
my $FILE="/tmp/restart.$$";

open(SLONIK, ">$FILE");
print SLONIK genheader();
print SLONIK "restart node $node;\n";
close SLONIK;
run_slonik_script($FILE);
