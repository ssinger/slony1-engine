#!@@PERL@@
# 
# Author: Mark Stosberg
# Based on work by: Christopher Browne
# Parts Copyright 2008 Summerault, LLC
# Parts Copyright 2004-2009 Afilias Canada

use Getopt::Long;

$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: slonik_drop_sequence [--config file] sequence_id set

    sequence_id  The ID of the sequence to be dropped from replication
    set  The name or ID of the set to drop the sequence from

You can get the sequence_id with a query like this:  

 SELECT seq_id,seq_relname from _MY_CLUSTER.sl_sequence where seq_relname = 'MY_SEQUENCE';

No application-visible locking should take place.

";

if ($SHOW_USAGE) {
    print $USAGE;
    exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($SEQ_ID,$set) = @ARGV;
$SET_ID = get_set($set);
unless ($SEQ_ID && $SET_ID) {
    die $USAGE;
}

my $slonik = '';

$slonik .= genheader();

# DROP TABLE
$slonik .= "\n";
$slonik .= "# DROP SEQUENCE \n";
$slonik .= "  try {\n";
$slonik .= "    SET DROP SEQUENCE (id = $SEQ_ID, origin = $SET_ORIGIN);\n";
$slonik .= "  } on error {\n";
$slonik .= "    echo 'Could not drop sequence $SEQ_ID for $CLUSTER_NAME!';\n";
$slonik .= "    exit 1;\n";
$slonik .= "  }\n";

run_slonik_script($slonik, 'DROP SEQUENCE');

