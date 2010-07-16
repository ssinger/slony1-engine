#!/usr/bin/perl
# 
# Christopher Browne
# Copyright 2004-2009
# Afilias Canada

# This script, given DSN parameters to access a Slony-I cluster,
# submits insert, update, and delete requests and sees how they
# propagate through the system.

use Pg;
use Getopt::Long;
#use strict;

my $sleep_seconds = 4;

my $goodopts = GetOptions("help", "database=s", "host=s", "user=s", "cluster=s",
			  "password=s", "port=s", "set=s", "finalquery=s");
if (defined($opt_help)) {
  show_usage();
}
my ($database,$user, $port, $cluster, $host, $password, $set, $finalquery);

$database = $opt_database if (defined($opt_database));
$port = 5432;
$port = $opt_port if (defined($opt_port));
$user = $opt_user if (defined($opt_user));
$password = $opt_password if (defined($opt_password));
$host = $opt_host if (defined($opt_host));
$cluster = $opt_cluster if (defined($opt_cluster));
$set = 1;
$set = $opt_set if (defined($opt_set));
$finalquery = $opt_finalquery if (defined($opt_finalquery));

require 'log.pm';
initialize_flog($cluster);

#DBI: my $initialDSN = "dbi:Pg:dbname=$database;host=$host;port=$port";
my $initialDSN = "dbname=$database host=$host port=$port";
$initialDSN = $initialDSN . " password=$password" if defined($opt_password);

print "DSN: $initialDSN\n";

# DBI: my $dbh = DBI->connect($initialDSN, $user, $password,
# 		       {RaiseError => 0, PrintError => 0, AutoCommit => 1});
# die "connect: $DBI::errstr" if ( !defined($dbh) || $DBI::err );
my $dbh = Pg::connectdb($initialDSN);

# Query to find the "master" node
my $masterquery = qq{
  select sub_provider 
   from "_$cluster".sl_subscribe s1 
   where not exists (select * from "_$cluster".sl_subscribe s2 
                                where s2.sub_receiver = s1.sub_provider and 
                                           s1.sub_set = $set and s2.sub_set = $set and
                                           s1.sub_active = 't' and s2.sub_active = 't')
   and s1.sub_set = $set
   group by sub_provider;
};

my $tq = $dbh->exec($masterquery);

#print "Rummage for master - $masterquery\n";
my $masternode;
while (my @row = $tq->fetchrow) {
  ($masternode) = @row;
  print "Found master: $masternode\n";
}

print "Rummage for DSNs\n";
# Query to find live DSNs
my $dsnsquery =
qq{
   select p.pa_server, p.pa_conninfo
   from "_$cluster".sl_path p
   where exists (select * from "_$cluster".sl_subscribe s where 
                          s.sub_set = $set and 
                          (s.sub_provider = p.pa_server or s.sub_receiver = p.pa_server) and
                          sub_active = 't')
   group by pa_server, pa_conninfo;
};

print "Query:\n$dsnsquery\n";
$tq = $dbh->exec($dsnsquery);
my %DSN;
while (my @row = $tq->fetchrow) {
  my ($node, $dsn) = @row;
  if (($node == $masternode) || check_node_for_subscription($node, $dsn)) {
    $DSN{$node} = $dsn;
    print "DSN[$node] = $dsn\n";
  } else {
    print "Skip node $node / DSN:$DSN - not yet subscribed\n";
  }
}
#$tq->finish();

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
my $time_date = sprintf ("%d-%.2d-%.2d %.2d:%.2d:%.2d", $year+1900, $mon+1, $mday, $hour, $min, $sec);

my $insert_text = "INSERT Replication Test";
my $insert_query = "insert into slony_test (description, mod_date) values ('$insert_text', '$time_date');";
submit_to_master($masternode, $insert_query, 1);

sleep $sleep_seconds;

my $select_query = "select description, mod_date from slony_test where mod_date = '$time_date' and description = '$insert_text'";
foreach my $node (keys %DSN) {
   if ($node != $masternode) {
      my $result=submit_to_slave($node, $select_query, 1);
   }
}

my $update_text = "UPDATE Replication Test";

my $update_qry = "update slony_test set description = '$update_text' where description = '$insert_text' and mod_date = '$time_date'";
submit_to_master($masternode, $update_qry, 1);

sleep $sleep_seconds;

$select_query = "select description, mod_date from slony_test where mod_date = '$time_date' and description = '$update_text'";

foreach my $node (keys %DSN) {
   if ($node != $masternode) {
      my $result=submit_to_slave($node, $select_query, 1);
      find_slave_backwardness($node);
   }
}

# We delete old data...
my $delete_qry = "delete from slony_test where mod_date < now() - '7 days'::interval;";
submit_to_master($masternode, $delete_qry, -1);
finalize_flog($cluster);

sub submit_to_master {
  my ($node, $query, $expected_count) = @_;
  my $dsn = $DSN{$node};
  if ($dsn =~ /password=/) {
    # got a password
  } else {
    $dsn .= " password=$password";
  }
  print "Connecting to master node $node - DSN:[$dsn]\n";
  my $master = Pg::connectdb($dsn);
  my $status_conn = $master->status;
  if ($status_conn ne PGRES_CONNECTION_OK) {
    alog($source, "", "Master connection Query failed - " . $master->errorMessage, $status_conn);
    report_failed_conn($conn_info);
    return -1;
  }
  my $result = $master->exec($query);
  if ($expected_count < 0) {
    print "Final query to master\n";
    if (defined($opt_finalquery)) {
      my $lastlogquery = $opt_finalquery;
      my $result = $master->exec($lastlogquery);
      my @row = $result ->fetchrow;
      my ($name, $creation) = @row;
      log_fact($cluster, "Replication for Cluster: [$cluster] Node: [$node] Behind by: [00:00:00] Last Log:[$name] Created: [$creation]");
    } else {
      log_fact($cluster, "Replication for Cluster: [$cluster] Node: [$node] Behind by: [00:00:00]");
    }
  } elsif ($result->ntuples != $expected_count) {
    alog($source, "", "Master $query failed - unexpected tuple count", $result->cmdTuples);
    return -2;
  } else {
    alog($source, "", "Master $query succeeded", 0);
    return 0;
  }
}

sub find_slave_backwardness {
  my ($node) = @_;
  my $dsn = $DSN{$node};
  if ($dsn =~ /password=/) {
    # got a password
  } else {
    $dsn .= " password=$password";
  }
  print "Connecting to slave node $node - DSN:[$dsn]\n";
  my $slave = Pg::connectdb($dsn);
  my $status_conn = $slave->status;
  if ($status_conn ne PGRES_CONNECTION_OK) {
    alog($source, $dest, "Connection Query failed!", -1);
    return "Connection Failed";
  }
  my $behindnessquery = "select coalesce( date_trunc('seconds', now() - max(mod_date)), '999999h'::interval) from slony_test;";
  my $result = $slave->exec($behindnessquery);
  if ($result->resultStatus != 2) {
    alog($source, $dest, "Slave query $query failed", $result->resultStatus);
    report_failed_conn($conn_info);
    log_fact($cluster, "Replication Test Failed: Cluster: [$cluster] Node: [$node]");
    return "Query Failed";
  } else {
    my $age;
    while (my @row = $result ->fetchrow) {
      ($age) = @row;
    }
    if (defined($opt_finalquery)) {
      my $lastlogquery = $opt_finalquery;
      my $result = $slave->exec($lastlogquery);
      while (my @row = $result ->fetchrow) {
	my ($name, $creation) = @row;
	log_fact($cluster, "Replication for Cluster: [$cluster] Node: [$node] Behind by: [$age] Last Log:[$name] Created: [$creation]");
	return;
      }
    } else {
      log_fact($cluster, "Replication for Cluster: [$cluster] Node: [$node] Behind by: [$age]");
      return;
    }
  }
}

sub submit_to_slave {
  my ($node, $query, $expect_count)=@_;
  my $dsn = $DSN{$node};
  if ($dsn =~ /password=/) {
    # got a password
  } else {
    $dsn .= " password=$password";
  }
  print "Connecting to slave node $node - DSN:[$dsn]\n";
  my $slave = Pg::connectdb($dsn);
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

sub check_node_for_subscription {
  my ($node, $dsn) = @_;
  if ($dsn =~ /password=/) {
    # got a password
  } else {
    $dsn .= " password=$password";
  }
  my $slave = Pg::connectdb($dsn);
  my $status_conn = $slave->status;
  if ($status_conn eq  PGRES_CONNECTION_BAD) {
    print "Status:  PGRES_CONNECTION_BAD\n";
    return;
  }
  my $livequery = qq{ select * from "_$cluster".sl_subscribe s1 where sub_set = $set and sub_receiver = $node and sub_active;};
  print "Query: $livequery\n";
  my $result = $slave->exec($livequery);
  while (my @row = $result->fetchrow) {
    print "Found live set!\n";
    return 1;
  }
  print "No live set found\n";
}

sub report_failed_conn {
    my ($ci) = @_;
    $ci =~ s/password=.*$//g;
    print "Failure - connection to $ci\n";
}

sub show_usage {
  my ($inerr) = @_;
  if ($inerr) {
    chomp $inerr;
    print $inerr, "\n";
  }
  die "$0  --host --database --user --cluster --port=integer --password --set=integer --finalquery=SQLQUERY";
}
