#!@@PERL@@
# $Id: execute_script.pl,v 1.1 2005-02-22 19:02:25 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: execute_script [--config file] set# node# full_path_to_sql_script_file

    Executes the contents of a SQL script file on the specified set and node.

    The script only needs to exist on the machine running the slon daemon.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $node, $file) = @ARGV;
if ($set =~ /^(?:set)?(\d+)$/) {
  $set = $1;
} else {
  print "Invalid set identifier\n\n";
  die $USAGE;
}

if ($node =~ /^(?:node)?(\d+)$/) {
  $node = $1;
} else {
  print "Invalid node identifier\n\n";
  die $USAGE;
}

unless ($file =~ /^\// and -f $file) {
  print "SQL script path needs to be a full path, e.g. /tmp/my_script.sql\n\n";
  die $USAGE;
}

my $FILE="/tmp/gensql.$$";
open(SLONIK, ">", $FILE);
print SLONIK genheader();
print SLONIK "  execute script (\n";
print SLONIK "    set id = $set,\n";
print SLONIK "    filename = '$file',\n";
print SLONIK "    event node = $node\n";
print SLONIK "  );\n";
close SLONIK;
run_slonik_script($FILE);
