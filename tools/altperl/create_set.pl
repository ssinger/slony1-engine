#!@@PERL@@
# $Id: create_set.pl,v 1.12 2005-02-10 06:22:41 smsimms Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require '@@PGLIBDIR@@/slon-tools.pm';
require '@@SYSCONFDIR@@/slon_tools.conf';
my ($set) = @ARGV;
if ($set =~ /^set(\d+)$/) {
  $set = $1;
} else {
  print "Need set identifier\n";
  die "create_set.pl setN\n";
}

$OUTPUTFILE="/tmp/add_tables.$$";

open (OUTFILE, ">$OUTPUTFILE");
print OUTFILE genheader();

foreach my $table (@SERIALTABLES) {
  $table = ensure_namespace($table);
  print OUTFILE "
		echo '  Adding unique key to table $table...';
		table add key (
		    node id=$MASTERNODE,
		    full qualified name='$table'
		);
";
}
print OUTFILE "
try {
      create set (id = $set, origin = $MASTERNODE, comment = 'Set $set for $CLUSTER_NAME');
} on error {
      echo 'Could not create subscription set $set for $CLUSTER_NAME!';
      exit -1;
}
";
print OUTFILE "
	echo 'Subscription set $set created';
	echo 'Adding tables to the subscription set';

";

if ($TABLE_ID < 1) {
  $TABLE_ID = 1;
}
foreach my $table (@SERIALTABLES) {
  $table = ensure_namespace($table);
  print OUTFILE "
		set add table (set id = $set, origin = $MASTERNODE, id = $TABLE_ID, full qualified name = '$table', comment = 'Table $table without primary key', key=serial);
                echo 'Add unkeyed table $table';
"; 
  $TABLE_ID++;
}

foreach my $table (@PKEYEDTABLES) {
  $table = ensure_namespace($table);
  print OUTFILE "
		set add table (set id = $set, origin = $MASTERNODE, id = $TABLE_ID, full qualified name = '$table', comment = 'Table $table with primary key');
                echo 'Add primary keyed table $table';
";
  $TABLE_ID++;
}

foreach my $table (keys %KEYEDTABLES) {
  my $key = $KEYEDTABLES{$table};
  $table = ensure_namespace($table);
  print OUTFILE "
		set add table (set id = $set, origin = $MASTERNODE, id = $TABLE_ID, full qualified name = '$table', key='$key', comment = 'Table $table with candidate primary key $key');
                echo 'Add candidate primary keyed table $table';
";
  $TABLE_ID++;
}

print OUTFILE "
                echo 'Adding sequences to the subscription set';
";

if ($SEQUENCE_ID < 1) {
  $SEQUENCE_ID = 1;
}
foreach my $seq (@SEQUENCES) {
  $seq = ensure_namespace($seq);
  print OUTFILE "
                set add sequence (set id = $set, origin = $MASTERNODE, id = $SEQUENCE_ID, full qualified name = '$seq', comment = 'Sequence $seq');
                echo 'Add sequence $seq';
";
  $SEQUENCE_ID++;
}
print OUTFILE "
        echo 'All tables added';
";

run_slonik_script($OUTPUTFILE);

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

