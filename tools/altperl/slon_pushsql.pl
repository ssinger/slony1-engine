#!/usr/bin/perl
# $Id: slon_pushsql.pl,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
my ($set, $node, $file) = @ARGV;
if ($set =~ /^set(\d+)$/) {
    $set = $1;
} else {
    print "Invalid set identifier";
    die "Usage: ./slon_pushsql.pl set[N] node[N] sql_script_file\n";
}
if ($node =~ /^node(\d+)$/) {
    $node = $1;
} else {
    print "Invalid node identifier";
    die "Usage: ./slon_pushsql.pl set[N] node[N] sql_script_file\n";
}

open(SLONIK, "|slonik");
print SLONIK genheader();

print SLONIK qq{
  execute script (
    set id=$set,
    filename='$file',
    event node = $node
  );
};
