#!@@PERL@@
# 
# Contributed by:
# Joe Kalash
# kalash@savicom.net

# This script, given parameters concerning the database nodes,
# generates output for "slon_tools.conf" consisting of:
# - A set of add_node() calls to configure the cluster
# - The arrays @PKEYEDTABLES, and @SEQUENCES

use DBI;
use Getopt::Long;
use strict;

my $dataBase;
my $host;
my $dataBaseUser;
my $dataBasePassword;
my $dataBasePort;
my @nodes;
my $schema = 'public';
my $usage = "$0 -node host:database:user[:password:port] [-node ...] [-schema myschema]
First node is assumed to be the master.
Default schema is \"public\"\n";

&usage if(!GetOptions('node=s@'=>\@nodes, 'schema=s' => \$schema));

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
my $connectString = "dbi:Pg:dbname=$dataBase;host=$host;port=$dataBasePort";
my $dbh = DBI->connect($connectString,$dataBaseUser,$dataBasePassword,
		       {RaiseError => 0, PrintError => 0, AutoCommit => 1});
die "connect: $DBI::errstr" if ( !defined($dbh) || $DBI::err );
# Read in all the user 'normal' tables in $schema (public by default).
my $tableQuery = $dbh->prepare("
SELECT pg_namespace.nspname || '.' || pg_class.relname,pg_class.relkind,pg_class.relhaspkey 
FROM pg_namespace,pg_class
WHERE pg_class.reltype > 0
AND pg_class.relnamespace = pg_catalog.pg_namespace.oid
AND (pg_class.relkind = 'r' OR pg_class.relkind = 'S')
AND pg_namespace.nspname = '$schema' AND pg_namespace.oid = pg_class.relnamespace");

die "prepare(tableQuery): $DBI::errstr" if ( !defined($tableQuery) || $DBI::err );
die "execute(tableQuery): $DBI::errstr" if ( !$tableQuery->execute() );
die "No objects to replicate found in schema \"$schema\"\n" if ($tableQuery->rows <= 0);

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

if ( scalar(@tablesWithIndexes) >= 1 ) {
  print '@PKEYEDTABLES=(' . "\n";
  foreach my $table (sort @tablesWithIndexes) {
    print "\t\"$table\",\n";
  }
  print ");\n";
}
if ( scalar(@tablesWithoutIndexes) >= 1 ) {
  my $tables = ''; 
  foreach my $table (sort @tablesWithoutIndexes) {
	  if($tables ne '') {
		  $tables.=',';
	  }
	  $tables.="\"$table\"";
  }
  die "The following tables had no unique index:" . $tables."\n";
}
if ( scalar(@sequences) >= 1 ) {
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
