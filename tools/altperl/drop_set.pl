#!/usr/bin/perl
# $Id: drop_set.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
my ($set) = @ARGV;
if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Need set identifier\n";
  croak "drop_set.pl setN\n";
}

open(SLONIK, "|slonik");

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
