#!perl   # -*- perl -*-
# $Id: test_slony_state.pl,v 1.1 2005-01-18 22:57:34 cbbrowne Exp $
# Christopher Browne
# Copyright 2004
# Afilias Canada

# This script, given DSN parameters to access a Slony-I cluster,
# submits a number of queries to test the state of the nodes in the
# cluster.

use Pg;
use Getopt::Long;
#use strict;

my $sleep_seconds = 4;

my $goodopts = GetOptions("help", "database=s", "host=s", "user=s", "cluster=s",
			  "password=s", "port=s");
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

#DBI: my $initialDSN = "dbi:Pg:dbname=$database;host=$host;port=$port";
my $initialDSN = "dbname=$database host=$host port=$port";
$initialDSN = $initialDSN . " password=$password" if defined($opt_password);

print "DSN: $initialDSN\n===========================\n";

# DBI: my $dbh = DBI->connect($initialDSN, $user, $password,
# 		       {RaiseError => 0, PrintError => 0, AutoCommit => 1});
# die "connect: $DBI::errstr" if ( !defined($dbh) || $DBI::err );
my $dbh = Pg::connectdb($initialDSN);

print "Rummage for DSNs\n=============================\n";
# Query to find live DSNs
my $dsnsquery =
"
   select p.pa_server, p.pa_conninfo
   from _$cluster.sl_path p
   where exists (select * from _$cluster.sl_subscribe s where
                          (s.sub_provider = p.pa_server or s.sub_receiver = p.pa_server) and
                          sub_active = 't')
   group by pa_server, pa_conninfo;
";

print "Query:\n$dsnsquery\n";
$tq = $dbh->exec($dsnsquery);
my %DSN;
while (my @row = $tq->fetchrow) {
  my ($node, $dsn) = @row;
  $DSN{$node} = $dsn;
}

foreach my $node (keys %DSN) {
  my $dsn = $DSN{$node};
  test_node($node, $dsn);
}

sub test_node {
  my ($node, $dsn) = @_;

  print "\nTests for node $node - DSN = $dsn\n========================================\n";

  my $listener_query = "select relpages, reltuples from pg_catalog.pg_class where relname = 'pg_listener';";
  my $res = $dbh->exec($listener_query);
  my ($relpages, $reltuples);
  while (my @row = $res->fetchrow) {
    ($relpages, $reltuples) = @row;
  }
  print qq{pg_listener info:
Pages: $relpages
Tuples: $reltuples
};

  print "\nSize Tests\n================================================\n";
  my $sizequeries = qq{select relname, relpages, reltuples from pg_catalog.pg_class where relname in ('sl_log_1', 'sl_log_2', 'sl_seqlog') order by relname;};
  $res = $dbh->exec($sizequeries);
  while (my @row = $res->fetchrow) {
    my ($relname, $relpages, $reltuples) = @row;
    printf "%15s  %8d %9f\n", $relname, $relpages, $reltuples;
  }

  print "\nListen Path Analysis\n===================================================\n";
  my $inadequate_paths = qq{
select li_origin, count(*) from _$cluster.sl_listen
group by li_origin
having count(*) < (select count(*) - 1 from _$cluster.sl_node );
};
  $res = $dbh->exec($inadequate_paths);
  while (my @row = $res->fetchrow) {
    my ($origin, $count) = @row;
    printf "Problem node: %4d  Listen path count for node: %d\n", $origin, $count;
    $listenproblems++;
  }
  my $missing_paths = qq{
   select * from (select n1.no_id as origin, n2.no_id as receiver
     from _$cluster.sl_node n1, _$cluster.sl_node n2 where n1.no_id != n2.no_id) as foo
   where not exists (select 1 from _$cluster.sl_listen
                     where li_origin = origin and li_receiver = receiver);
};
  $res = $dbh->exec($missing_paths);
  while (my @row = $res->fetchrow) {
    my ($origin, $receiver) = @row;
    printf "(origin,receiver) where there is exists a direct path missing in sl_listen: (%d,%d)\n", 
      $origin, $receiver;
    $listenproblems++;
  }

  # Each subscriber node must have a direct listen path
  my $no_direct_path = qq{
    select sub_set, sub_provider, sub_receiver from _$cluster.sl_subscribe where not exists
        (select 1 from _$cluster.sl_listen 
         where li_origin = sub_provider and li_receiver = sub_receiver and li_provider = sub_provider);
};
  $res = $dbh->exec($no_direct_path);
  while (my @row = $res->fetchrow) {
    my ($set, $provider, $receiver) = @row;
    printf "No direct path found for set %5d from provider %5d to receiver %5d\n", $set, $provider, $receiver;
    $listenproblems++;
  }

  if ($listenproblems > 0) {
    print "sl_listen problems found: $listenproblems\n";
  } else {
    print "No problems found with sl_listen\n";
  }

  print "\n--------------------------------------------------------------------------------\n";
  print "Summary of event info\n";
  printf "%7s %9s %9s %12s %12s\n", "Origin", "Min SYNC", "Max SYNC", "Min SYNC Age", "Max SYNC Age";
  print "================================================================================\n";

  my $event_summary = qq{
  select ev_origin, min(ev_seqno), max(ev_seqno),
         date_trunc('minutes', min(now() - ev_timestamp)),
         date_trunc('minutes', max(now() - ev_timestamp))
     from _$cluster.sl_event group by ev_origin;
  };
  $res = $dbh->exec($event_summary);
  while (my @row = $res->fetchrow) {
    my ($origin, $minsync, $maxsync, $minage, $maxage) = @row;
    printf "%7s %9d %9d %12s %12s\n", $origin, $minsync, $maxsync, $minage, $maxage;
  }
  print "\n";

  print "\n---------------------------------------------------------------------------------\n";
  print "Summary of sl_confirm aging\n";
  printf "%9s  %9s  %9s  %9s  %12s  %12s\n", "Origin", "Receiver", "Min SYNC", "Max SYNC", "Age of latest SYNC", "Age of eldest SYNC";
  print "=================================================================================\n";
  my $confirm_summary = qq{

    select con_origin, con_received, min(con_seqno) as minseq,
           max(con_seqno) as maxseq, date_trunc('minutes', min(now()-con_timestamp)) as age1,
           date_trunc('minutes', max(now()-con_timestamp)) as age2
    from _$cluster.sl_confirm
    group by con_origin, con_received
    order by con_origin, con_received;
  };

  $res = $dbh->exec($confirm_summary);
  while (my @row = $res->fetchrow) {
    my ($origin, $receiver, $minsync, $maxsync, $minage, $maxage) = @row;
    printf "%9s  %9s  %9s  %9s  %12s  %12s\n", $origin, $receiver, $minsync, $maxsync, $minage, $maxage;
  }
  print "\n";

  print "\n------------------------------------------------------------------------------\n";
  print "\nListing of old open connections\n";
  printf "%15s %15s %15s %12s %20s\n", "Database", "PID", "User", "Query Age", "Query";
  print "================================================================================\n";

  my $old_conn_query = qq{
     select datname, procpid, usename, date_trunc('minutes', now() - query_start), substr(current_query,0,20)
     from pg_stat_activity
     where  (now() - query_start) > '1:30'::interval and
            current_query <> '<IDLE>'
     order by query_start;
  };

  $res = $dbh->exec($old_conn_query);
  while (my @row = $res->fetchrow) {
    my ($db, $pid, $user, $age, $query) = @row;
    printf "%15s %15d %15s %12s %20s\n", $db, $pid, $user, $age, $query;
  }
  print "\n";
}

sub show_usage {
  my ($inerr) = @_;
  if ($inerr) {
    chomp $inerr;
    print $inerr, "\n";
  }
  die "$0  --host --database --user --cluster --port=integer --password";
}
