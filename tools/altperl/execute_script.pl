#!@@PERL@@
# $Id: execute_script.pl,v 1.2 2005-03-16 21:15:10 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$SCRIPT_ARG  = "";

# Allow this to be specified.  Otherwise it will be pulled from
# the get_set function.
$node = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE,
	   "c=s"      => \$SCRIPT_ARG,
           "n|node=s" => \$node);

my $USAGE =
"Usage:
    execute_script [options] set# full_path_to_sql_script_file
    execute_script [options] -c SCRIPT set#

    Executes the contents of a SQL script file on the specified set.
    The script only needs to exist on the machine running the slon
    daemon.

    set#        The set to which this script applies.

    -c SCRIPT   Pass the SQL to be executed via the command line instead
                of as a file.

    -n NUM
    --node=NUM  Override the set origin specified in the configuration
                file.

";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $file) = @ARGV;
if ($set =~ /^(?:set)?(\d+)$/) {
  $set = $1;
} else {
  print "Invalid set identifier\n\n";
  die $USAGE;
}

get_set($set) or die "Non-existent set specified.\n";
$node = $SET_ORIGIN unless $node;

# We can either have -c SCRIPT or a filename as an argument.  The
# latter takes precedence.
if ($file) {
    unless ($file =~ /^\// and -f $file) {
	print STDERR "SQL script path needs to be a full path, e.g. /tmp/my_script.sql\n\n";
	die $USAGE;
    }
}
elsif ($SCRIPT_ARG) {
    # Put the script into a file
    $file = "/tmp/execute_script.sql.$$";
    my $fh;
    open $fh, ">", $file;
    print $fh $SCRIPT_ARG;
    close $fh;
}
else {
    print STDERR "You must include either a filename or a SQL statement on the command line to be run.\n\n";
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
