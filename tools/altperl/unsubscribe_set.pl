#!@@PERL@@
# $Id: unsubscribe_set.pl,v 1.6 2005-02-10 06:22:41 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require '@@PGLIBDIR@@/slon-tools.pm';
require '@@SYSCONFDIR@@/slon_tools.conf';

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

open(SLONIK, ">/tmp/slonik-unsubscribe.$$");
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
run_slonik_script("/tmp/slonik-unsubscribe.$$");
