#!/usr/bin/perl
# $Id: drop_set.pl,v 1.2 2004-08-10 20:55:32 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
my ($set) = @ARGV;
if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Need set identifier\n";
  die "drop_set.pl setN\n";
}
$OUTFILE="/tmp/dropset.$$";
open(SLONIK, ">>$OUTFILE");

print SLONIK genheader();

print SLONIK qq{
        try {
                drop set (id = $set, origin=1);
        }
        on error {
                exit 1;
        }
        echo 'Dropped set $set';
};
close SLONIK;
run_slonik_script($OUTFILE);
