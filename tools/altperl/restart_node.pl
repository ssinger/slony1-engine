#!@@PERL@@
# $Id: restart_node.pl,v 1.5 2005-02-10 04:32:49 smsimms Exp $
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
