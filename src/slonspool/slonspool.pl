#!/usr/bin/perl
# $Id: slonspool.pl,v 1.1 2004-11-30 23:38:41 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

use Pg;
use Getopt::Long;
require "subscribe.pm";
require "gen_output.pm";
require "init.pm";

my($database,$user, $host, $cluster, $password, $port, $spoolpath, $spoolname,
   $maxsize, $maxage, $subnode, $node);

my @SETS;
my $dbh;
process_options();
initialize_configuration();
#subscribe_to_node();

while (1) {
  listen_to_node();
}

sub listen_to_node {
  while (1) {
    process_event();
    die -1;
  }
}

sub process_event {
  my $dsn = "dbname=$database host=$host port=$port user=$user";
  if ($password) {
    $dsn .= " password=$password";
  }
  print "DSN: $dsn\n";
  my $dbh = Pg::connectdb($dsn);
  print "Last err:", $dbh->errorMessage, "\n";
  my $sync_event;
  my $last_seq = qq{select con_seqno from "_$cluster".sl_confirm
                    where con_origin = $node order by con_seqno desc limit 1;};
  print $last_seq, "\n";
  my $res = $dbh->exec($last_seq);
  while (my @row = $res->fetchrow) {
    ($sync_event) = @row;
    print "Last sync: $sync_event\n";
  }
  print "Last err:", $dbh->errorMessage, "\n";
  $sync_event++;
  print "Next sync: $sync_event\n";

  my @ORIGINS;
  my $origin_query = qq{ select set_origin from "_$cluster".sl_set where set_id in ($opt_sets); };
  $res = $dbh->exec($origin_query);
  while (my @row = $res->fetchrow) {
    my ($origin) = @row;
    push @ORIGINS, $origin;
  }
  $origin_qualification = " (log_origin in (" . join(',', @ORIGINS) . ")) ";


  my $table_qualification = " (log_tableid in (" . join(',', @TABLES) . ")) ";

  print "Table qualification: $table_qualification\n";
  my $qualification .= " $origin_qualification and $table_qualification ";

  my $cursor_query = qq{
  declare LOG cursor for
    select log_origin, log_xid, log_tableid, log_actionseq, log_cmdtype, log_cmddata
    from "_$cluster".sl_log_1
    where $origin_qualification and $table_qualification
    order by log_xid, log_actionseq;};

  print "Cursor query: $cursor_query\n";

  my $lastxid = "";
  my $syncname=sprintf("log-%08d", $sync);
  open(LOGOUTPUT, ">$spoolpath/$syncname");
  print LOGOUTPUT "-- Data for sync $sync_event\n";
  print LOGOUTPUT "-- ", `date`;
  my $begin = $dbh->exec("begin;");
  my $cursorexec = $dbh->exec($cursor_query);
  print "Last err:", $dbh->errorMessage, "\n";
  my $foundsome = "YES";
  while ($foundsome eq "YES") {
    $foundsome = "NO";
    my $res = $dbh->exec("fetch forward 100 in LOG;");
    while (my @row = $res->fetchrow) {
      $foundsome = "YES";
      my ($origin, $xid, $tableid, $actionseq, $cmdtype, $cmddata) = @row;
      if ($xid ne $lastxid) { # changed xid - report that...
	if ($lastxid ne "") {  # Do nothing first time around...
	  printf LOGOUTPUT "COMMIT; -- Done xid $lastxid\n";
	}
	print LOGOUTPUT "BEGIN;\nselect fail_if_xid_applied($xid);\n";
	$lastxid = $xid;
      }
      if ($cmdtype eq "I") {
	printf LOGOUTPUT "insert into %s %s;\n", $TABLENAME[$tableid], $cmddata;
      } elsif ($cmdtype eq "U") {
	printf LOGOUTPUT "update only %s set %s;\n", $TABLENAME[$tableid], $cmddata;
      } elsif ($cmdtype eq "D") {
	printf LOGOUTPUT "delete from only %s where %s;\n", $TABLENAME[$tableid], $cmddata;
      } else {
	print LOGOUTPUT "problem: cmddata not in (I,U,D) = [$cmdtype]\n";
      }
    }
  }
  if ($lastxid ne "") {
    print LOGOUTPUT "COMMIT; -- Done xid $lastxid\n";
  }
  close LOGOUTPUT;
  $dbh->exec("rollback;");
  my $confirmation = qq{ insert into "_$cluster".sl_confirm (con_origin,con_received,con_seqno,con_timestamp)
                         values ($node, $subnode, $sync_event, CURRENT_TIMESTAMP); };
  print "Confirm: $confirmation\n";
  my $cursorexec = $dbh->exec($confirmation);
}

sub connect_to_node {
  my $dsn = "dbname=$database host=$host port=$port user=$user";
  if ($password) {
    $dsn .= " password=$password";
  }
  $dbh = Pg::connectdb($dsn);
}

sub process_options {

  $goodopts = GetOptions("help", "database=s", "host=s", "user=s",
			 "cluster=s", "password=s", "port=s", "sets=s",
			 "spoolpath=s", "spoolname=s", "pgbins=s",
			 "maxsize=i", "maxage=i", "node=i", "subnode=i");

  if (defined ($opt_help)) {
    show_usage();
  }

  $cluster=$opt_cluster if (defined($opt_cluster));
  $subnode = $opt_subnode if (defined ($opt_subnode));
  $node = $opt_node if (defined($opt_node));
  $database=$opt_database if (defined ($opt_database));
  $user = $opt_user if (defined ($opt_user));
  $host = $opt_host if (defined($opt_host));
  $password = $opt_password if (defined($opt_password));
  $port = $opt_port if (defined($opt_port));
  $pgbins = $opt_pgbins if (defined($opt_pgbins));
  $spoolpath = $opt_spoolpath if (defined($opt_spoolpath));
  $spoolname   = $opt_spoolname   if (defined($opt_spoolname));
  if (defined($opt_sets)) {
    @SETS=split (/,/, $opt_sets);
  }
  if (defined($opt_maxsize)){
    $maxsize = $opt_maxsize;
  } else {
    $maxsize = 10000;
  }
  if (defined($opt_maxage)){
    $maxsize = $opt_maxage;
  } else {
    $maxage = 300;
  }
}

sub show_usage {
  print qq{slonspool:
     --help                get help
     --cluster=s           Slony-I cluster name
     --subnode=s   Node number subscribed through
     --node=i              Node number to use to request
     --pgbins=s            Location of PostgreSQL binaries including slonik and pg_dump
     --database=s          database to connect to
     --host=s              host for database
     --user=s              user for database
     --password=s          password for database (you should probably use .pgpass instead)
     --port=i              port number to connect to
     --sets=s              Sets to replicate (comma-delimited) - e.g --sets=1,2,4 
     --spoolpath=s         directory in which to spool output
     --spoolname=s         naming convention for spoolfiles
     --maxsize=i           maximum size of spool files, in kB - default =10000KB
     --maxage=i            maximum age of spool files in seconds - default 300
};
  die -1;
}
