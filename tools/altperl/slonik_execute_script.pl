#!@@PERL@@
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;
use File::Temp qw(tempfile);

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$SCRIPT_ARG  = "";

# Allow this to be specified.  Otherwise it will be pulled from
# the get_set function.
$node = 0;

# temp file variable for script handling
my $filename = '';
my $fh       = undef;

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

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set, $file) = @ARGV;
die $USAGE unless $set;
$set = get_set($set) or die "Non-existent set specified.\n";
$node = $SET_ORIGIN unless $node;

# We can either have -c SCRIPT or a filename as an argument.  The
# latter takes precedence.
if ($file) {
    unless ($file =~ /^\// and -f $file) {
	print STDERR "SQL script path needs to be a full path, e.g. /tmp/my_script.sql\n\n";
	die $USAGE;
    }
    $filename = $file
}
elsif ($SCRIPT_ARG) {
    # Put the script into a file
    ($fh, $filename) = tempfile();
    print $fh $SCRIPT_ARG;
    close $fh;
}
else {
    print STDERR "You must include either a filename or a SQL statement on the command line to be run.\n\n";
    die $USAGE;
}

my $slonik = '';

$slonik .= genheader();
$slonik .= "  execute script (\n";
$slonik .= "    set id = $set,\n";
$slonik .= "    filename = '$filename',\n";
$slonik .= "    event node = $node\n";
$slonik .= "  );\n";

run_slonik_script($slonik, 'EXECUTE SCRIPT');
