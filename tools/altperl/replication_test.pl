#!perl   # -*- perl -*-
# $Id: replication_test.pl,v 1.1.2.1 2004-09-30 17:37:28 cbbrowne Exp $
# Christopher Browne
# Copyright 2004
# Afilias Canada

# This script, given DSN parameters to access a Slony-I cluster,
# submits insert, update, and delete requests and sees how they
# propagate through the system.

# It requires setting up the following table and propagating it via
# (surprise!) Slony-I

#create table slony_test (
#   description text,
#   mod_date timestamp with time zone
#);

use Pg;
use Getopt::Long;
use strict;

$LOGDIR="/opt/OXRS/logs/general";
`mkdir -p $LOGDIR`;
$HOST = `hostname`;
$IDENT = "-";
$USER = `whoami`;
$SIZE = "-";
chomp($HOST, $IDENT, $USER, $SIZE);
my $sleep_seconds = 80;

$goodopts = GetOptions("help", "database=s", "host=s", "user=s", "cluster=s", "password=s", "port=s", "set=s");
if (defined($opt_help)) {
  show_usage();
}
my ($database,$user, $port, $cluster, $host, $password, $set);

$database = $opt_database if (defined($opt_database));
$port = 5432;
$port = $opt_port if (defined($opt_port));
$user = $opt_user if (defined($opt_user));
$password = $opt_password if (defined($opt_password));
$host = $opt_host if (defined($opt_host));
$cluster = $opt_cluster if (defined($opt_cluster));
$set = $opt_set if (defined($opt_set));

#DBI: my $initialDSN = "dbi:Pg:dbname=$database;host=$host;port=$port";
my $initialDSN = "dbname=$database host=$host port=$port";
$initialDSN = $initialDSN . " password=$password" if defined($opt_password);

# DBI: my $dbh = DBI->connect($initialDSN, $user, $password,
# 		       {RaiseError => 0, PrintError => 0, AutoCommit => 1});
# die "connect: $DBI::errstr" if ( !defined($dbh) || $DBI::err );
my $dbh = Pg::connectdb($initialDSN);

# Query to find the "master" node
my $masterquery = qq{
  select sub_provider 
   from _oxrslive.sl_subscribe s1 
   where not exists (select * from _oxrslive.sl_subscribe s2 
                                where s2.sub_receiver = s1.sub_provider and 
                                           s1.sub_set = $set and s2.sub_set = $set and
                                           s1.sub_active = 't' and s2.sub_active = 't')
   group by sub_provider;
};

# my $tq = $dbh->prepare($masterquery);
# die "prepare(masterquery): $DBI::errstr" if (!defined($tq) || $DBI::err);
# die "execute(masterquery): $DBI::errstr" if (!$tq->execute());
my $tq = $dbh->exec($masterquery);

my $masternode;
#while (my $row = $tq->fetchrow_arrayref()) {
while (my @row = $tq->fetchrow) {
  ($masternode) = @row;
}
#$tq->finish();

# Query to find live DSNs
my $dsnsquery =
qq {
   select p.pa_server, p.pa_conninfo
   from _$cluster.sl_path p
   where exists (select * from _$cluster.subscribe s where 
                          s.sub_set = $set and 
                          (s.sub_provider = p.pa_server or s.sub_receiver = p.pa_server)
                          sub_active = 't')
   group by pa_server, pa_conninfo;
};

# $tq = $dbh->prepare($dsnsquery);
# die "prepare(dsnsquery): $DBI::errstr" if (!defined($tq) || $DBI::err);
# die "execute(dsnsquery): $DBI::errstr" if (!$tq->execute());
$tq = $dbh->exec($dsnsquery);
my @DSN;
#while (my $row = $tq->fetchrow_arrayref()) {
while (my @row = $tq->fetchrow) {
  ($node, $dsn) = @row;
  $DSN{$node} = $dsn;
}
#$tq->finish();

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
my $time_date = sprintf ("%d-%.2d-%.2d %.2d:%.2d:%.2d", $year+1900, $mon+1, $mday, $hour, $min, $sec);

my $insert_text = "INSERT Replication Test";
my $insert_query = "insert into slony_test (description, mod_date) values ('$insert_text', '$time_date');";
foreach my $source (sort keys %SOURCES) {
  submit_to_master($masternode, $insert_query, 1);
}

sleep $sleep_seconds;

sub submit_to_master {
  my ($node, $query, $expected_count) = @_;
  my $dsn = $DSN{$node};
  my $master = Pg::connectdb($conn_info);
  my $status_conn = $master->status;
  if ($status_conn ne PGRES_CONNECTION_OK) {
    alog($source, "", "Master connection Query failed - " . $master->errorMessage, $status_conn);
    report_failed_conn($conn_info);
    return -1;
  }
  my $result = $master->exec($query);
  if ($result->ntuples != $expect_count) {
    alog($source, "", "Master $query failed - unexpected tuple count", $result->cmdTuples);
    return -2;
  } else {
    alog($source, "", "Master $query succeeded", 0);
    return 0;
  }
}

sub submit_to_slave {
  my ($source, $dest, $conn_info, $query, $expect_count)=@_;
  my $slave = Pg::connectdb($conn_info);
  my $status_conn = $slave->status;
  if ($status_conn ne PGRES_CONNECTION_OK) {
    alog($source, $dest, "Connection Query failed!", -1);
    return "Connection Failed";
  }
  my $result = $slave->exec($query);
  if ($result->resultStatus != 2) {
    alog($source, $dest, "Slave query $query failed", $result->resultStatus);
    report_failed_conn($conn_info);
    return "Query Failed";
  } elsif ($result->ntuples != $expect_count) {
    alog($source, $dest, "Slave query failed - $query - slave is behind master", -3);
    # This indicates that the slave is behind - issue message!
    return "Slave Behind";
  } else {
    alog($source, $dest, "Query $query succeeded", 0);
    return "OK";
  }
}

sub report_failed_conn {
    my ($ci) = @_;
    $ci =~ s/password=.*$//g;
    print "Failure - connection to $ci\n";
}

sub alog {
  my ($source, $dest, $message, $rc) = @_;
  chomp ($source, $dest, $message, $rc);
  #print"Master DB: $source  Slave DB: $dest  Message: [$message] RC=$rc\n"; 
  apache_log("replication.log", "Master DB: $source  Slave DB: $dest  Message: [$message]", $rc);
}

sub apache_log {
  my ($logfile, $request, $status) = @_;
  my $date = `date`;
  chomp($request, $status, $date);
  my $LOGENTRY ="$HOST $IDENT $USER [$date] \"$request\" $status $SIZE";
  open(OUTPUT, ">>$LOGDIR/$logfile");
  print OUTPUT $LOGENTRY, "\n";
  close OUTPUT;
}

sub show_usage {
  my ($inerr) = @_;
  if ($inerr) {
    chomp $inerr;
    print $inerr, "\n";
  }
  die "$0  --host --database --user --cluster --port --password";
}
