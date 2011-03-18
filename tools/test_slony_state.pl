#!/usr/bin/perl
# 
# Christopher Browne
# Copyright 2005-2009
# PostgreSQL Global Development Group

# This script, given DSN parameters to access a Slony-I cluster,
# submits a number of queries to test the state of the nodes in the
# cluster.

# Note: This uses the legacy "Pg" module; it is likely that the other
# version of this script that uses DBI is preferable to use.

use Pg;
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
  "r|recipient=s" => \$recipient);

if (defined($help)) {
  show_usage();
}
my $initialDSN = "";
$initialDSN .= "dbname=$database " if (defined $database);
$initialDSN .= "host=$host " if (defined $host);
$initialDSN .= "port=$port " if (defined $port);
$initialDSN .= "user=$user " if (defined $user);
$initialDSN .= "password=$password " if (defined $password);

print "DSN: $initialDSN\n===========================\n";

my $dbh = Pg::connectdb($initialDSN);

print "Rummage for DSNs\n=============================\n";
# Query to find live DSNs
my $dsnsquery =
qq{
   select p.pa_server, p.pa_conninfo
   from "_$cluster".sl_path p
--   where exists (select * from "_$cluster".sl_subscribe s where
--                          (s.sub_provider = p.pa_server or s.sub_receiver = p.pa_server) and
--                          sub_active = 't')
   group by pa_server, pa_conninfo;
};

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

report_on_problems ();

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

  my $HILISTENPAGES = 5000;
  if ($relpages > $HILISTENPAGES) {
    add_problem ($node, "pg_listener relpages high - $relpages", 
		 qq{Number of pages in table pg_listener is $relpages
This is higher than the warning level of $HILISTENPAGES.

Perhaps a long running transaction is preventing pg_listener from
being vacuumed out?
}); 
  }

  my $HILISTENTUPLES = 200000;
  if ($reltuples > $HILISTENTUPLES) {
    add_problem ($node, "pg_listener reltuples high - $reltuples",
		 qq{Number of tuples in system table pg_listener is $reltuples.
This is higher than the warning level of $HILISTENTUPLES.

Perhaps a long running transaction is preventing pg_listener from
being vacuumed out?
});
  }

  my $HISLTUPLES=200000;
  print "\nSize Tests\n================================================\n";
  my $sizequeries = qq{select relname, relpages, reltuples from pg_catalog.pg_class where relname in ('sl_log_1', 'sl_log_2', 'sl_seqlog') order by relname;};
  $res = $dbh->exec($sizequeries);
  while (my @row = $res->fetchrow) {
    my ($relname, $relpages, $reltuples) = @row;
    printf "%15s  %8d %9f\n", $relname, $relpages, $reltuples;
    if ($reltuples > $HISLTUPLES) {
      add_problem($node, "$relname tuples = $reltuples > $HISLTUPLES",
		  qq{Number of tuples in Slony-I table $relname is $reltuples which
exceeds $HISLTUPLES.

You may wish to investigate whether or not a node is down, or perhaps
if sl_confirm entries have not been propagating properly.
});

    }
  }

  print "\nListen Path Analysis\n===================================================\n";
  my $inadequate_paths = qq{
select li_origin, count(*) from "_$cluster".sl_listen
group by li_origin
having count(*) < (select count(*) - 1 from "_$cluster".sl_node );
};
  $res = $dbh->exec($inadequate_paths);
  while (my @row = $res->fetchrow) {
    my ($origin, $count) = @row;
    printf "Problem node: %4d  Listen path count for node: %d\n", $origin, $count;
    $listenproblems++;
  }
  my $missing_paths = qq{
   select * from (select n1.no_id as origin, n2.no_id as receiver
     from "_$cluster".sl_node n1, "_$cluster".sl_node n2 where n1.no_id != n2.no_id) as foo
   where not exists (select 1 from "_$cluster".sl_listen
                     where li_origin = origin and li_receiver = receiver);
};
  $res = $dbh->exec($missing_paths);
  my $allmissingpaths;
  while (my @row = $res->fetchrow) {
    my ($origin, $receiver) = @row;
    my $string = sprintf "(origin,receiver) where there is exists a direct path missing in sl_listen: (%d,%d)\n",
      $origin, $receiver;
    print $string;
    $listenproblems++;
    $allmissingpaths .= $string;
  }
  if ($allmissingpaths) {
    add_problem($node, "Missing sl_listen paths", qq{$allmissingpaths

Please check contents of table sl_listen; some STORE LISTEN requests may be
necessary.
});
  }

  # Each subscriber node must have a direct listen path
  my $no_direct_path = qq{
    select sub_set, sub_provider, sub_receiver from "_$cluster".sl_subscribe where not exists
        (select 1 from "_$cluster".sl_listen 
         where li_origin = sub_provider and li_receiver = sub_receiver and li_provider = sub_provider);
};
  $res = $dbh->exec($no_direct_path);
  while (my @row = $res->fetchrow) {
    my ($set, $provider, $receiver) = @row;
    my $string = sprintf "No direct path found for set %5d from provider %5d to receiver %5d\n", $set, $provider, $receiver;
    print $string;
    add_problem($node, "Missing path from $provider to $receiver", qq{Missing sl_listen entry - $string

Please check contents of table sl_listen; some STORE LISTEN requests may be
necessary.
});
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

  my $WANTAGE = "00:30:00";
  my $event_summary = qq{
  select ev_origin, min(ev_seqno), max(ev_seqno),
         date_trunc('minutes', min(now() - ev_timestamp)),
         date_trunc('minutes', max(now() - ev_timestamp)),
         min(now() - ev_timestamp) > '$WANTAGE' as agehi
     from "_$cluster".sl_event group by ev_origin;
  };
  $res = $dbh->exec($event_summary);
  while (my @row = $res->fetchrow) {
    my ($origin, $minsync, $maxsync, $minage, $maxage, $agehi) = @row;
    printf "%7s %9d %9d %12s %12s %4s\n", $origin, $minsync, $maxsync, $minage, $maxage, $agehi;
    if ($agehi eq 't') {
      add_problem($origin, "Events not propagating to node $origin",
		  qq{Events not propagating quickly in sl_event -
For origin node $origin, earliest propagated event of age $minage > $WANTAGE

Are slons running for both nodes?

Could listen paths be missing so that events are not propagating?
});
    }
  }
  print "\n";

  print "\n---------------------------------------------------------------------------------\n";
  print "Summary of sl_confirm aging\n";
  printf "%9s  %9s  %9s  %9s  %12s  %12s\n", "Origin", "Receiver", "Min SYNC", "Max SYNC", "Age of latest SYNC", "Age of eldest SYNC";
  print "=================================================================================\n";
  my $WANTCONFIRM = "00:30:00";
  my $confirm_summary = qq{

    select con_origin, con_received, min(con_seqno) as minseq,
           max(con_seqno) as maxseq, date_trunc('minutes', min(now()-con_timestamp)) as age1,
           date_trunc('minutes', max(now()-con_timestamp)) as age2,
           min(now() - con_timestamp) > '$WANTCONFIRM' as tooold
    from "_$cluster".sl_confirm
    group by con_origin, con_received
    order by con_origin, con_received;
  };

  $res = $dbh->exec($confirm_summary);
  while (my @row = $res->fetchrow) {
    my ($origin, $receiver, $minsync, $maxsync, $minage, $maxage, $agehi) = @row;
    printf "%9s  %9s  %9s  %9s  %12s  %12s %4s\n", $origin, $receiver, $minsync, $maxsync, $minage, $maxage, $agehi;
    if ($agehi eq 't') {
      add_problem($origin, "Confirmations not propagating from $origin to $receiver",
		  qq{Confirmations not propagating quickly in sl_confirm -

For origin node $origin, receiver node $receiver, earliest propagated
confirmation has age $minage > $WANTCONFIRM

Are slons running for both nodes?

Could listen paths be missing so that confirmations are not propagating?
});
    }
  }
  print "\n";

  print "\n------------------------------------------------------------------------------\n";
  printf "\nListing of old open connections on node %d\n", $node;
  printf "%15s %15s %15s %12s %20s\n", "Database", "PID", "User", "Query Age", "Query";
  print "================================================================================\n";

  my $ELDERLY_TXN = "01:30:00";
  my $old_conn_query = qq{
     select datname, procpid, usename, date_trunc('minutes', now() - query_start), substr(current_query,0,20)
     from pg_stat_activity
     where  (now() - query_start) > '$ELDERLY_TXN'::interval and
            current_query <> '<IDLE>'
     order by query_start;
  };

  $res = $dbh->exec($old_conn_query);
  while (my @row = $res->fetchrow) {
    my ($db, $pid, $user, $age, $query) = @row;
    printf "%15s %15d %15s %12s %20s\n", $db, $pid, $user, $age, $query;
      add_problem($origin, "Old Transactions Kept Open",
		  qq{Old Transaction still running with age $age > $ELDERLY_TXN

Query: $query
});
  }
  print "\n";
  my $ELDERLY_COMPONENT = "00:05:00";
  my $old_conn_query = qq{
     select co_actor, co_pid, co_node, co_connection_pid, co_activity, co_starttime, now() - co_starttime, co_event, co_eventtype
     from "_$cluster".sl_components 
     where  (now() - co_starttime) > '$ELDERLY_COMPONENT'::interval
     order by co_starttime;
  };

  print "\n------------------------------------------------------------------------------\n";
  printf "\nListing of thread activities for node %d\n", $node;
  printf "%25s %8s %8s %8s %20s %20s %20s %15s %s\n", "actor", "pid", "node", "cpid", "activity", "start", "age", "event type", "event";
  print "==================================================================================================================================================\n";
  $res = $dbh->exec($old_conn_query);
  $res->execute();
  while (my @row = $res->fetchrow) {
    my ($actor, $pid, $node, $cpid, $activity, $start,$age, $event, $evtype) = @row;
    printf "%25s %8d %8d %8d %20s %20s %20s %15s %8d\n", $actor, $pid, $node, $cpid, $activity, $start,$age, $evtype, $event;
      add_problem($origin, "threads seem stuck",
		  qq{Slony-I components have not reported into sl_components in interval $ELDERLY_COMPONENT

Perhaps slon is not running properly?

Query: $query
});
  }
  print "\n";

}

sub show_usage {
  my ($inerr) = @_;
  if ($inerr) {
    chomp $inerr;
    print $inerr, "\n";
  }
  die "$0  --host --database --user --cluster --port=integer --password --recipient --mailprog\nnote also that libpq environment variables PGDATABASE, PGPORT, ... may also be passed in";
}

sub add_problem {
  my ($node, $short, $long) = @_;
  $PROBLEMS{"$node $short"} = $long;
}

sub report_on_problems {
  my ($totalproblems, $message);
  foreach my $key (sort keys %PROBLEMS) {
    $totalproblems++;
    $message .= "\nNode: $key\n================================================\n" . $PROBLEMS{$key} . "\n";
  }
  if ($totalproblems) {
    open(MAIL, "|$mailprog -s \"Slony State Test Warning - Cluster $cluster\" $recipient");
    print MAIL "\n";
    print MAIL $message;
    close (MAIL);
    print "\n\nSending message thus - |$mailprog -s \"Slony State Test Warning - Cluster $cluster\" $recipient\n";
    print "Message:\n\n$message\n";
  }
}
