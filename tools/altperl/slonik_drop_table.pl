#!@@PERL@@
# $Id: slonik_drop_table.pl,v 1.1 2006-05-30 14:52:35 cbbrowne Exp $
# Author: Mark Stosberg
# Based on work by: Christopher Browne
# Parts Copyright 2006 Summerault, LLC
# Parts Copyright 2004 Afilias Canada

use Getopt::Long;

$CONFIG_FILE = '/usr/local/etc/slon_tools.conf';
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

require '/usr/local/pgsql/lib//slon-tools.pm';
require $CONFIG_FILE;

my ($TABLE_ID,$set) = @ARGV;
$SET_ID = get_set($set);
unless ($TABLE_ID && $SET_ID) {
    die $USAGE;
}

$FILE="/tmp/drop_table.$$";
open (SLONIK, ">", $FILE);
print SLONIK genheader();

# DROP TABLE
print SLONIK "\n";
print SLONIK "# DROP TABLE \n";
print SLONIK "  try {\n";
print SLONIK "    SET DROP TABLE (id = $TABLE_ID, origin = $SET_ORIGIN);\n";
print SLONIK "  } on error {\n";
print SLONIK "    echo 'Could not drop table $TABLE_ID for $CLUSTER_NAME!';\n";
print SLONIK "    exit -1;\n";
print SLONIK "  }\n";


close SLONIK;
run_slonik_script($FILE);
