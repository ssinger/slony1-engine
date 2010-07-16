#!/usr/bin/perl
# 
# Christopher Browne
# Copyright 2007-2009
# PostgreSQL Global Development Group

# This script, given DSN parameters to access a Slony-I cluster,
# submits a number of queries to test the state of the nodes in the
# cluster.  It generates, as output, a MediaWiki page summarizing
# the cluster.

# Documented in doc/adminguide/monitoring.sgml

use DBI;
use Getopt::Long;
#use strict;
my %PROBLEMS;

my $sleep_seconds = 4;

GetOptions
 ("H|help" => \$help,
  "d|database=s" => \$database,
  "h|host=s" => \$host,
  "u|user=s" => \$user,
  "c|cluster=s" => \$cluster,
  "p|password=s" => \$password,
  "P|port=i" => \$port,
  "m|mailprog=s" => \$mailprog,
  "f|finalquery=s" => \$finalquery,
  "r|recipient=s" => \$recipient,
  "g|categories=s" => \$categories);

if (defined($help)) {
  show_usage();
}
my $initialDSN = "dbi:Pg:";

$initialDSN .= "dbname=$database;" if (defined $database);
$initialDSN .= "host=$host;" if (defined $host);
$initialDSN .= "port=$port;" if (defined $port);
$initialDSN .= "user=$user;" if (defined $user);
$initialDSN .= "password=$password;" if (defined $password);

#print "Initial DSN: $initialDSN\n";
my $dbh = DBI->connect($initialDSN) or die "Unable to connect: $DBI::errstr\n";
my $clns = qq{"_${cluster}"};    # Slony-I cluster name
my $commaaggregator = qq{acl_admin.comma_list};

my $sverquery = qq{ select ${clns}.slonyversion();};
$tq = $dbh->prepare($sverquery);
$tq->execute();

while (my @row = $tq->fetchrow_array) {
  my ($sversion) = @row;
  print qq{== Slony-I cluster ${cluster} ===

Slony-I version $sversion;
};
}

# Query to find live DSNs
my $dsnsquery = qq{
select 
    no_id,
    (select min(pa_conninfo) from ${clns}.sl_path where pa_server = no_id) as path, 
    no_comment, exists (select * from ${clns}.sl_set where set_origin = no_id) as origin,
    (select  max(sub_provider::text) from ${clns}.sl_subscribe where sub_receiver = no_id) as providers
 from ${clns}.sl_node;
};
$tq = $dbh->prepare($dsnsquery);
$tq->execute();

&mknodeheader();
my %DSN, %COMMENT, %ORIGIN, %PROVIDERS, %HOST, %PVERSION, %PORT;
while (my @row = $tq->fetchrow_array) {
  my ($node, $dsn, $comment, $origin, $providers) = @row;
  $DSN{$node} = $dsn;
  $COMMENT{$node}=$comment;
  if ($origin) {
    $ORIGIN{$node} = "yes";
  } else {
    $ORIGIN{$node}= "no";
  }
  $PROVIDERS{$node}=$providers;
  if ($dsn=~/\s*host=(\S+)\s*/) {
    $HOST{$node}=$1;
  } elsif ($dsn =~ / host=([\S+]) /) {
    $HOST{$node}=$1;
  } elsif ($dsn =~ /host=([\S+])$/) {
    $HOST{$node}=$1;
  } elsif ($dsn =~ / host=([\S+])$/) {
    $HOST{$node}=$1;
  } elsif ($dsn =~ /host=([\S+]) /) {
    $HOST{$node}=$1;
  } else {
    #print "no HOST match!\n";
  }
#  print "Host: ", $HOST{$node}, "\n";
}

foreach my $node (keys %DSN) {
  my $dsn = $DSN{$node};
  test_node($node, $dsn);
  gennodeline($node, $PORT{$node}, $HOST{$node}, $PVERSION{$node}, $ORIGIN{$node}, $PROVIDERS{$node}, $dsn, $COMMENT{$node});
}
&mktblfooter();

print "=== Replicated Tables ===\n";
&mktblheader();
my $tquery=qq{ select set_id, tab_id, tab_nspname, tab_relname, tab_idxname, tab_comment from ${clns}.sl_table, ${clns}.sl_set where set_id = tab_set order by set_id, tab_id;};
$tq = $dbh->prepare($tquery);
$tq->execute();
while (my @row = $tq->fetchrow_array) {
  my ($set, $tbl, $nsp, $tname, $idx, $comment) = @row;
  print qq{|-
! $set
! $tbl
! $nsp
! $tname
! $idx
! $comment
};
}
&mktblfooter();

print "=== Replicated Sequences ===\n";
&mkseqheader();
my $squery = qq[ select set_id, seq_id, seq_nspname, seq_relname, seq_comment from ${clns}.sl_sequence, ${clns}.sl_set where set_id = seq_set; ];
$tq = $dbh->prepare($squery);
$tq->execute();
while (my @row = $tq->fetchrow_array) {
  my ($set, $seq, $nsp, $sname, $comment) = @row;
  print qq{|-
! ${set}
! ${seq}
! ${nsp}
! ${sname}
! ${comment}
};
}
&mktblfooter();

print "=== Subscriptions ===\n";
&mksubheader();
$squery = qq[  select sub_set, sub_provider, sub_receiver, sub_forward from ${clns}.sl_subscribe order by sub_set, sub_provider, sub_receiver; ];
$tq = $dbh->prepare($squery);
$tq->execute();
while (my @row = $tq->fetchrow_array) {
  my ($set, $provider, $receiver, $forward) = @row;
  if ($forward) {
    $forward = "yes";
  } else {
    $forward = "no";
  }
  print qq{|-
! ${set}
! ${provider}
! ${receiver}
! ${forward}
};
}
&mktblfooter();
&gencategories();

sub gennodeline {
  my ($node, $port, $host, $version, $origin, $providers, $dsn, $comment) = @_;
  print qq{|-
! ${node}
! $port
! [[host:$host|$host]]
! $version
! $origin
! $providers
! $dsn
! $comment
};
}

sub test_node {
  my ($node, $dsn) = @_;
  $dsn =~ s/ /\;/g;
  $dsn = "dbi:Pg:${dsn}";
#  print "TN - DSN[$dsn]\n";
  my $vquery = "select version();";
  my $pquery = "show port;";
  my $dbh2 = DBI->connect($dsn) or die "Unable to connect: $DBI::errstr\n";
  my $res = $dbh2->prepare($vquery);
  $res->execute();
  while (my @row = $res->fetchrow_array) {
    ($PVERSION{$node}) = @row;
  }

  $res = $dbh2->prepare($pquery);
  $res->execute();
  while (my @row = $res->fetchrow_array) {
    ($PORT{$node}) = @row;
  }
}

sub show_usage {
  my ($inerr) = @_;
  if ($inerr) {
    chomp $inerr;
    print $inerr, "\n";
  }
  die "$0  --host --database --user --cluster --port=integer --password --recipient --mailprog --categories\nnote also that libpq environment variables PGDATABASE, PGPORT, ... may also be passed in";
}

sub mknodeheader {
print qq[
{| border="1" cellpadding="1" cellspacing="0" style="font-size: 85%; border: gray solid 1px; border-collapse: collapse; text-align: center; width: 100%;"
|- style="background: #ececec" style="width:16em"
! Node
! Port
! Host
! PG Version
! Master Node?
! Provider nodes
! DSN
! Node Description
];
}

sub mktblfooter {
print "|}\n\n";
}

sub mktblheader {
print qq[
{| border="1" cellpadding="1" cellspacing="0" style="font-size: 85%; border: gray solid 1px; border-collapse: collapse; text-align: center; width: 100%;"
|- style="background: #ececec" style="width:16em"
! Set
! Tbl ID
! Namespace
! Table Name
! Replication Index
! Comment
];
}

sub mkseqheader {
print qq[
{| border="1" cellpadding="1" cellspacing="0" style="font-size: 85%; border: gray solid 1px; border-collapse: collapse; text-align: center; width: 100%;"
|- style="background: #ececec" style="width:16em"
! Set
! Seq ID
! Namespace
! Sequence Name
! Comment
];
}

sub mksubheader {
print qq[
{| border="1" cellpadding="1" cellspacing="0" style="font-size: 85%; border: gray solid 1px; border-collapse: collapse; text-align: center; width: 100%;"
|- style="background: #ececec" style="width:16em"
! Set
! Provider
! Receiver
! Forwarding?
];
}

# End off by splitting the --categories option into a comma-delimited
# list of categories and dropping them into place
sub gencategories {
  if ($categories ne "") {
    foreach my $cat (split(',', $categories)) {
      print "[Category:${cat}]\n";
    }
  }
}
