#!perl # -*- perl -*-
# $Id: slon_pushsql.pl,v 1.4 2004-09-15 21:56:37 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
my ($set, $node, $file) = @ARGV;
if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Invalid set identifier";
  die "Usage: ./slon_pushsql.pl set[N] node[N] full_path_to_sql_script_file\n";
}
if ($node =~ /^node(\d+)$/) {
  $node = $1;
} else {
  print "Invalid node identifier";
  die "Usage: ./slon_pushsql.pl set[N] node[N] full_path_to_sql_script_file\n";
}

if ($file !=~ /^\//) {
  print "SQL script path needs to be a full path, i.e. /tmp/my_script.sql\n";
  die "Usage: ./slon_pushsql.pl set[N] node[N] full_path_to_sql_script_file\n";
}

my $FILE="/tmp/gensql.$$";
open(SLONIK, ">$FILE");
print SLONIK genheader();

print SLONIK qq{
  execute script (
    set id=$set,
    filename='$file',
    event node = $node
  );
};
close SLONIK;
run_slonik_script($FILE);
