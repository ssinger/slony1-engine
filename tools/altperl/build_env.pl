#!perl    # -*- perl -*-
# $Id: build_env.pl,v 1.4 2004-09-09 17:04:07 cbbrowne Exp $
# Contributed by:
# Joe Kalash
# kalash@savicom.net

# This script, given parameters concerning the database nodes,
# generates output for "slon.env" consisting of:
# - A set of add_node() calls to configure the cluster
# - The arrays @KEYEDTABLES, @SERIALTABLES, and @SEQUENCES

use DBI;
use Getopt::Long;
use strict;

my $dataBase;
my $host;
my $dataBaseUser;
my $dataBasePassword;
my $dataBasePort;
my @nodes;
my $usage = "$0 -node host:database:user[:password:port] [-node ...]
First node is assumed to be the master.\n";

&usage if(!GetOptions('node=s@'=>\@nodes));

die "At least one node is required" if ( scalar(@nodes) < 1 );

my $nodeNumber = 1;
my $parentString;
foreach my $node (@nodes)
{
  my($tmpHost,$tmpDataBase,$tmpDataBaseUser,$tmpDataBasePassword,$tmpPort) =
    split(/:/,$node);
  die "Host is required" if ( !$tmpHost );
  die "database is required" if ( !$tmpDataBase );
  die "user is required" if ( !$tmpDataBaseUser );
  $tmpPort = 5432 if ( !$tmpPort );
  $host = $tmpHost if ( !$host );
  $dataBase = $tmpDataBase if ( !$dataBase );
  if ( !$dataBaseUser ) {
    $dataBaseUser = $tmpDataBaseUser;
    $dataBasePassword = $tmpDataBasePassword if ( $tmpDataBasePassword );
    $dataBasePort = $tmpPort if ( $tmpPort );
  }
  print "&add_node(host => '$tmpHost', dbname => '$tmpDataBase', port =>$tmpPort,
        user=>'$tmpDataBaseUser', password=>'$tmpDataBasePassword', node=>$nodeNumber $parentString);\n";
  $parentString = ', parent=>1';
  $nodeNumber++;

}
my $connectString = "dbi:Pg:dbname=$dataBase;host=$host";
my $dbh = DBI->connect($connectString,$dataBaseUser,$dataBasePassword,
		       {RaiseError => 0, PrintError => 0, AutoCommit => 1});
die "connect: $DBI::errstr" if ( !defined($dbh) || $DBI::err );
# Read in all the user 'normal' tables in public.
my $tableQuery = $dbh->prepare("
SELECT pg_namespace.nspname || '.' || pg_class.relname,pg_class.relkind,pg_class.relhaspkey 
FROM pg_namespace,pg_class
WHERE pg_class.reltype > 0
AND pg_class.relnamespace = pg_catalog.pg_namespace.oid
AND (pg_class.relkind = 'r' OR pg_class.relkind = 'S')
AND pg_namespace.nspname = 'public' AND pg_namespace.oid = pg_class.relnamespace");

die "prepare(tableQuery): $DBI::errstr" if ( !defined($tableQuery) || $DBI::err );
die "execute(tableQuery): $DBI::errstr" if ( !$tableQuery->execute() );

my @tablesWithIndexes;
my @tablesWithoutIndexes;
my @sequences;
while ( my $row = $tableQuery->fetchrow_arrayref() ) {
  my $relname = @$row[0];
  my $relkind = @$row[1];
  my $relhaspkey = @$row[2];
  push(@sequences,$relname) if ( $relkind eq 'S' );
  push(@tablesWithIndexes,$relname) if ( $relkind eq 'r' && $relhaspkey == 1 );
  push(@tablesWithoutIndexes,$relname) if ( $relkind eq 'r' && $relhaspkey == 0 );
}
$tableQuery->finish();
$dbh->disconnect();

if ( scalar(@tablesWithIndexes) > 1 ) {
  print '@KEYEDTABLES=(' . "\n";
  foreach my $table (sort @tablesWithIndexes) {
    print "\t\"$table\",\n";
  }
  print ");\n";
}
if ( scalar(@tablesWithoutIndexes) > 1 ) {
  print '@SERIALTABLES=(' . "\n";
  foreach my $table (sort @tablesWithoutIndexes) {
    print "\t\"$table\",\n";
  }
  print ");\n";
}
if ( scalar(@sequences) > 1 ) {
  print '@SEQUENCES=(' . "\n";
  foreach my $table (sort @sequences) {
    print "\t\"$table\",\n";
  }
  print ");\n";
}
exit 0;

sub usage {
  print "$usage";
  exit 0;
}
