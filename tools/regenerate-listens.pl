#!@@PERL@@
# $Id: regenerate-listens.pl,v 1.1 2005-02-14 22:24:08 smsimms Exp $
# Copyright 2005
# Christopher B. Browne
# cbbrowne@acm.org
# Afilias Canada

# regenerate-listens.pl connects to a Slony-I node, and queries
# sl_subscribe, sl_set, sl_path, and sl_node to compute what the
# "listen paths" should look like.

# This will be obsoleted by Slony-I 1.1, as the function
# RebuildListenEntries() and RebuildListenEntriesOne() run through the
# very same logic "inside the server," or at least, automatically
# invoked by slonik.

# But until 1.1, this should be useful for building/rebuilding
# listener paths

use DBI;
use Getopt::Long;

my $goodopts = GetOptions("help", "host=s", "user=s", "password=s", "db=s",
			  "port=s", "cluster=s" );

if (defined($opt_help)) {
  print "Help - $opt_help\n";
  die qq{ genlisten [--help] [--host=host] [--user=user] [--password=password]
[--db=database] [--port=port] [--cluster=Slony-I cluster]
};
}

my $sinfo = "dbi:Pg";
if (defined($opt_db)) {
  $sinfo .= ":dbname=$opt_db";
} else {
  die "must specify database name\n";
}
if (defined($opt_host)) {
  $sinfo .= ";host=$opt_host";
}
if (defined($opt_port)) {
  $sinfo .= ";port=$opt_port";
}
if (defined($opt_cluster)) {
  $cluster = $opt_cluster;
} else {
  die "No Slony-I cluster specified\n";
}

$dbh = DBI->connect($sinfo,$opt_user,$opt_password)
  or die "No connection to database " . $DBI::errstr;

my $nodes_query = $dbh->prepare(qq{ 
   select n1.no_id as origin, n2.no_id as receiver 
    from "_$cluster".sl_node n1, "_$cluster".sl_node n2
    where n1.no_id <> n2.no_id;
}) or die $dbh->errstr;

$nodes_query->execute() or die $dbh->errstr;

while (my @row = $nodes_query->fetchrow_array) {
  my ($origin, $receiver) = @row;
  rebuildlistenentries ($origin, $receiver);
}
foreach my $origin (sort keys %PROVIDER) {
  foreach my $receiver (sort keys %PROVIDER) {
    if ($origin != $receiver) {
      $providers=$PROVIDER{$origin}{$receiver};
      foreach $provider (split(/:/, $providers)) {
	print "store listen (origin = $origin, receiver = $receiver, provider = $provider);\n";
      }
    }
  }
}



sub rebuildlistenentries {
  my ($origin, $receiver) = @_;

  my $subquery = $dbh->prepare(qq{ 
   select distinct sub_provider 
   from "_$cluster".sl_subscribe, "_$cluster".sl_set, "_$cluster".sl_path
   where sub_set = set_id
	and set_origin = $origin
	and sub_receiver = $receiver
	and sub_provider = pa_server
	and sub_receiver = pa_client;
  }) or die $dbh->errstr;
  $subquery->execute() or die $dbh->errstr;
  while (my @row = $subquery->fetchrow_array) {
    my ($provider) = @row;
    $PROVIDER{$origin}{$receiver}=$provider;
    return 1;
  }
  my $path_query = $dbh->prepare(qq{
   select true from "_$cluster".sl_path
   where pa_server = $origin and
         pa_client = $receiver
   limit 1;
  }) or die $dbh->errstr;
  $path_query->execute() or die $dbh->errstr;
  while (my @row = $path_query->fetchrow_array) {
    $PROVIDER{$origin}{$receiver}=$origin;
    return 1;
  }

  my $providers_query = $dbh->prepare(qq{
     select distinct provider from (
			select sub_provider as provider
					from "_$cluster".sl_subscribe
					where sub_receiver = $receiver
			union
			select sub_receiver as provider
					from "_$cluster".sl_subscribe
					where sub_provider = $receiver
					and exists (select true from "_$cluster".sl_path
								where pa_server = sub_receiver
								and pa_client = sub_provider)
			) as S;
     }) or die $dbh->errstr;

  $providers_query->execute() or die $dbh->errstr;
  while (my @row = $providers_query->fetchrow_array) {
    my ($provider) = @_;
    $PROVIDER{$origin}{$receiver}=$provider;
    return 1;
  }
  my $last_resort = $dbh->prepare(qq{
   select pa_server as provider
   from "_$cluster".sl_path
   where pa_client = $receiver;
  }) or die $dbh->errstr;
  $last_resort->execute() or die $dbh->errstr;
  while (my @row = $last_resort->fetchrow_array) {
    my ($provider) = @_;
    if ($PROVIDER{$origin}{$receiver}) {
      $PROVIDER{$origin}{$receiver}=$provider;
    } else {
      $PROVIDER{$origin}{$receiver}.=":" . $provider;
    }
  }
  return 1;
}
