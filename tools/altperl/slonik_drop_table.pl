#!@@PERL@@
# 
# Author: Mark Stosberg
# Based on work by: Christopher Browne
# Parts Copyright 2006 Summerault, LLC
# Parts Copyright 2004-2009 Afilias Canada

use Getopt::Long;

$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: slonik_drop_table [--config file] table_id set

    table_id  The ID of the table to be dropped from replication
    set  The name or ID of the set to drop the table from

 You can get the table_id with a query like this:  

 SELECT tab_id,tab_relname from _MY_CLUSTER.sl_table where tab_relname = 'MY_TABLE';

";

if ($SHOW_USAGE) {
    print $USAGE;
    exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($TABLE_ID,$set) = @ARGV;
$SET_ID = get_set($set);
unless ($TABLE_ID && $SET_ID) {
    die $USAGE;
}

my $slonik = '';

$slonik .= genheader();

# DROP TABLE
$slonik .= "\n";
$slonik .= "# DROP TABLE \n";
$slonik .= "  try {\n";
$slonik .= "    SET DROP TABLE (id = $TABLE_ID, origin = $SET_ORIGIN);\n";
$slonik .= "  } on error {\n";
$slonik .= "    echo 'Could not drop table $TABLE_ID for $CLUSTER_NAME!';\n";
$slonik .= "    exit 1;\n";
$slonik .= "  }\n";

run_slonik_script($slonik, 'DROP TABLE');
