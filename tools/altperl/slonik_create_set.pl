#!@@PERL@@

# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

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

my $slonik = ''; 

if ($SHOW_USAGE) {
    print $USAGE;
    exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my ($set) = @ARGV;

die $USAGE unless $set;
$SET_ID = get_set($set);
unless ($SET_ID) {
    my $possible_sets = join "\n\t", keys %$SLONY_SETS;
    print "No set was found with the name provided.  Possible valid names include:\n\t"
          . $possible_sets . "\n\n"
          . "New sets may be defined in your slon_tools.conf file\n\n";
    die $USAGE;
}

$slonik .= genheader();

# Tables without primary keys
$slonik .= "\n";
$slonik .= "# TABLE ADD KEY\n";

# CREATE SET
$slonik .= "\n";
$slonik .= "# CREATE SET\n";
$slonik .= "    create set (id = $SET_ID, origin = $SET_ORIGIN, comment = 'Set $SET_ID ($SET_NAME) for $CLUSTER_NAME');\n";

# SET ADD TABLE
$slonik .= "\n";
$slonik .= "# SET ADD TABLE\n";
$slonik .= "  echo 'Subscription set $SET_ID ($SET_NAME) created';\n";
$slonik .= "  echo 'Adding tables to the subscription set';\n";

$TABLE_ID = 1 if $TABLE_ID < 1;

foreach my $table (@PKEYEDTABLES) {
    $table = ensure_namespace($table);
    $table = lc($table) if $FOLD_CASE;
    $slonik .= "  set add table (set id = $SET_ID, origin = $SET_ORIGIN, id = $TABLE_ID,\n";
    $slonik .= "                 full qualified name = '$table',\n";
    $slonik .= "                 comment = 'Table $table with primary key');\n";
    $slonik .= "  echo 'Add primary keyed table $table';\n";
    $TABLE_ID++;
}

foreach my $table (keys %KEYEDTABLES) {
    my $key = $KEYEDTABLES{$table};
    $table = ensure_namespace($table);
    $table = lc($table) if $FOLD_CASE;
    $slonik .= "  set add table (set id = $SET_ID, origin = $SET_ORIGIN, id = $TABLE_ID,\n";
    $slonik .= "                 full qualified name = '$table', key='$key',\n";
    $slonik .= "                 comment = 'Table $table with candidate primary key $key');\n";
    $slonik .= "  echo 'Add candidate primary keyed table $table';\n";
    $TABLE_ID++;
}

# SET ADD SEQUENCE
$slonik .= "\n";
$slonik .= "# SET ADD SEQUENCE\n";
$slonik .= "  echo 'Adding sequences to the subscription set';\n";

$SEQUENCE_ID = 1 if $SEQUENCE_ID < 1;
foreach my $seq (@SEQUENCES) {
    $seq = ensure_namespace($seq);
    $seq = lc($seq) if $FOLD_CASE;
    $slonik .= "  set add sequence (set id = $SET_ID, origin = $SET_ORIGIN, id = $SEQUENCE_ID,\n";
    $slonik .= "                    full qualified name = '$seq',\n";
    $slonik .= "                    comment = 'Sequence $seq');\n";
    $slonik .= "  echo 'Add sequence $seq';\n";
    $SEQUENCE_ID++;
}
$slonik .= "  echo 'All tables added';\n";

run_slonik_script($slonik, 'CREATE SET');

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
