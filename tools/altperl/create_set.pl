#!/usr/bin/perl
# $Id: create_set.pl,v 1.4 2004-08-10 20:55:32 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';
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
		    node id=1,
		    full qualified name='$table'
		);
";
}
close OUTFILE;
run_slonik_script($OUTPUTFILE);


open (OUTFILE, ">$OUTPUTFILE");
print OUTFILE genheader();

print OUTFILE "
	try {
		create set (id = $set, origin = 1, comment = 'Set for slony tables');
	}
	on error {
		echo 'Could not create subscription set!';
		exit -1;
	}
";

close OUTFILE;
run_slonik_script($OUTPUTFILE);

open (OUTFILE, ">$OUTPUTFILE");
print OUTFILE genheader();
print OUTFILE "
	echo 'Subscription set created';
	echo 'Adding tables to the subscription set';

";

$TABLE_ID=1;
foreach my $table (@SERIALTABLES) {
  $table = ensure_namespace($table);
  print OUTFILE "
		set add table (set id = $set, origin = 1, id = $TABLE_ID, full qualified name = '$table', comment = 'Table $table', key=serial);
                echo 'Add unkeyed table $table';
"; 
  $TABLE_ID++;
}

foreach my $table (@KEYEDTABLES) {
  $table = ensure_namespace($table);
  print OUTFILE "
		set add table (set id = $set, origin = 1, id = $TABLE_ID, full qualified name = '$table', comment = 'Table $table');
                echo 'Add keyed table $table';
";
  $TABLE_ID++;
}

close OUTFILE;
run_slonik_script($OUTPUTFILE);

open (OUTFILE, ">$OUTPUTFILE");
print OUTFILE genheader();
# Finish subscription set...
print OUTFILE "
                echo 'Adding sequences to the subscription set';
";

$SEQID=1;
foreach my $seq (@SEQUENCES) {
  $seq = ensure_namespace($seq);
  print OUTFILE "
                set add sequence (set id = $set, origin = 1, id = $SEQID, full qualified name = '$seq', comment = 'Sequence $seq');
                echo 'Add sequence $seq';
";
  $SEQID++;
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
