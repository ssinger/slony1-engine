#!@@PERL@@
# $Id: create_set.pl,v 1.14 2005-02-22 17:11:18 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Getopt::Long;

$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: create_set [--config file] set

    set  The name or ID of the set to be created

";

if ($SHOW_USAGE) {
    print $USAGE;
    exit 0;
}

require '@@PGLIBDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set) = @ARGV;
$SET_ID = get_set($set);
unless ($SET_ID) {
    die $USAGE;
}

$FILE="/tmp/add_tables.$$";
open (SLONIK, ">", $FILE);
print SLONIK genheader();

# Tables without primary keys
print SLONIK "\n";
print SLONIK "# TABLE ADD KEY\n";
foreach my $table (@SERIALTABLES) {
    $table = ensure_namespace($table);
    print SLONIK "  echo '  Adding unique key to table $table...';\n";
    print SLONIK "  table add key (\n";
    print SLONIK "    node id=$MASTERNODE,\n";
    print SLONIK "    full qualified name='$table'\n";
    print SLONIK "  );\n";
}

# CREATE SET
print SLONIK "\n";
print SLONIK "# CREATE SET\n";
print SLONIK "  try {\n";
print SLONIK "    create set (id = $SET_ID, origin = $MASTERNODE, comment = 'Set $SET_ID for $CLUSTER_NAME');\n";
print SLONIK "  } on error {\n";
print SLONIK "    echo 'Could not create subscription set $SET_ID for $CLUSTER_NAME!';\n";
print SLONIK "    exit -1;\n";
print SLONIK "  }\n";

# SET ADD TABLE
print SLONIK "\n";
print SLONIK "# SET ADD TABLE\n";
print SLONIK "  echo 'Subscription set $SET_ID created';\n";
print SLONIK "  echo 'Adding tables to the subscription set';\n";

$TABLE_ID = 1 if $TABLE_ID < 1;

foreach my $table (@SERIALTABLES) {
    $table = ensure_namespace($table);
    print SLONIK "  set add table (set id = $SET_ID, origin = $MASTERNODE, id = $TABLE_ID,\n";
    print SLONIK "                 full qualified name = '$table', key=serial,\n";
    print SLONIK "                 comment = 'Table $table without primary key');\n";
    print SLONIK "  echo 'Add unkeyed table $table';\n";
    $TABLE_ID++;
}

foreach my $table (@PKEYEDTABLES) {
    $table = ensure_namespace($table);
    print SLONIK "  set add table (set id = $SET_ID, origin = $MASTERNODE, id = $TABLE_ID,\n";
    print SLONIK "                 full qualified name = '$table',\n";
    print SLONIK "                 comment = 'Table $table with primary key');\n";
    print SLONIK "  echo 'Add primary keyed table $table';\n";
    $TABLE_ID++;
}

foreach my $table (keys %KEYEDTABLES) {
    my $key = $KEYEDTABLES{$table};
    $table = ensure_namespace($table);
    print SLONIK "  set add table (set id = $SET_ID, origin = $MASTERNODE, id = $TABLE_ID,\n";
    print SLONIK "                 full qualified name = '$table', key='$key'\n";
    print SLONIK "                 comment = 'Table $table with candidate primary key $key');\n";
    print SLONIK "  echo 'Add candidate primary keyed table $table';\n";
    $TABLE_ID++;
}

# SET ADD SEQUENCE
print SLONIK "\n";
print SLONIK "# SET ADD SEQUENCE\n";
print SLONIK "  echo 'Adding sequences to the subscription set';\n";

$SEQUENCE_ID = 1 if $SEQUENCE_ID < 1;
foreach my $seq (@SEQUENCES) {
    $seq = ensure_namespace($seq);
    print SLONIK "  set add sequence (set id = $SET_ID, origin = $MASTERNODE, id = $SEQUENCE_ID,\n";
    print SLONIK "                    full qualified name = '$seq',\n";
    print SLONIK "                    comment = 'Sequence $seq');\n";
    print SLONIK "  echo 'Add sequence $seq';\n";
    $SEQUENCE_ID++;
}
print SLONIK "  echo 'All tables added';\n";

close SLONIK;
run_slonik_script($FILE);

### If object hasn't a namespace specified, assume it's in "public", and make it so...
sub ensure_namespace {
    my ($object) = @_;
    if ($object =~ /^(.*\..*)$/) {
	# Table has a namespace specified
    } else {
	$object = "public.$object";
    }
    return $object;
}
