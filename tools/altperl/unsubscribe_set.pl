#!/usr/bin/perl
# $Id: unsubscribe_set.pl,v 1.1 2004-07-25 04:02:51 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

my ($set, $node) = @ARGV;
if ($node =~ /^node(\d+)$/) {
  $node = $1;
} else {
  print "Need to specify node!\n";
  die "unsubscribe_set setM nodeN\n";
}

if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Need to specify set!\n";
  die "unsubscribe_set setM nodeN\n";
}

open(SLONIK, ">/tmp/slonik-subscribe.$$");
print SLONIK genheader();
print SLONIK qq{
        try {
                unsubscribe set (id = $set, receiver = $node);
        }
        on error {
                echo 'Failed to unsubscribe node $node from set $set';
                exit 1;
        }
        echo 'unsubscribed node $node from set $set';
};
close SLONIK;
print `slonik < /tmp/slonik-unsubscribe.$$`;
unlink("/tmp/slonik-unsubscribe.$$");
