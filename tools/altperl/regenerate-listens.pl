#!@@PERL@@
# $Id: regenerate-listens.pl,v 1.3 2005-02-02 17:22:29 cbbrowne Exp $
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

use Pg;
use Getopt::Long;

my $goodopts = GetOptions("help", "host=s", "user=s", "password=s", "db=s",
			  "port=s", "cluster=s" );

my $db = 'oxrsorg';
my $sinfo = "dbname=$db port=5532 host=dba2 user=cbbrowne";

if (defined($opt_help)) {
  print "Help - $opt_help\n";
  die qq{ genlisten [--help] [--host=host] [--user=user] [--password=password]
[--db=database] [--port=port] [--cluster=Slony-I cluster]
};
}

my $sinfo = "";#"dbname=$database host=$host port=$port user=$user";
if (defined($opt_db)) {
  $sinfo .= " dbname=$opt_db";
} else {
  $sinfo .= " dbname=oxrsorg";
}
if (defined($opt_host)) {
  $sinfo .= " host=$opt_host";
} else {
  $sinfo .= " host=dba2";
}
if (defined($opt_user)) {
  $sinfo .= " user=$opt_user";
} else {
  $sinfo .= " user=cbbrowne";
}
$sinfo .= " password=$opt_password"
  if (defined($opt_password));
if (defined($opt_port)) {
  $sinfo .= " port=$opt_port";
} else {
  $sinfo .= " port=5532";
}
if (defined($opt_cluster)) {
  $cluster = $opt_cluster;
} else {
  print "No Slony-I cluster specified\n";
  die -1;
}
$conn =  Pg::connectdb($sinfo);
$status_conn = $conn->status;

if ($status_conn ne PGRES_CONNECTION_OK) {
  print "No connection to database.";
  print $conn->errorMessage;
  exit(1);
}

my $nodes_query = qq{ 
   select n1.no_id as origin, n2.no_id as receiver 
    from "_$cluster".sl_node n1, "_$cluster".sl_node n2
    where n1.no_id <> n2.no_id;
};
my $res = $conn->exec($nodes_query);
while (my @row = $res->fetchrow) {
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

  my $subquery = qq{ 
   select distinct sub_provider 
   from "_$cluster".sl_subscribe, "_$cluster".sl_set, "_$cluster".sl_path
   where sub_set = set_id
	and set_origin = $origin
	and sub_receiver = $receiver
	and sub_provider = pa_server
	and sub_receiver = pa_client;
  };
  my $res = $conn->exec($subquery);
  while (my @row = $res->fetchrow) {
    my ($provider) = @row;
    $PROVIDER{$origin}{$receiver}=$provider;
    return 1;
  }
  my $path_query = qq{
   select true from "_$cluster".sl_path
   where pa_server = $origin and
         pa_client = $receiver
   limit 1;
  };
  $res = $conn->exec($path_query);
  while (my @row = $res->fetchrow) {
    $PROVIDER{$origin}{$receiver}=$origin;
    return 1;
  }

  my $providers_query = qq{
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
     };

  $res = $conn->exec($providers_query);
  while (my @row = $res->fetchrow) {
    my ($provider) = @_;
    $PROVIDER{$origin}{$receiver}=$provider;
    return 1;
  }
  my $last_resort = qq{
   select pa_server as provider
   from "_$cluster".sl_path
   where pa_client = $receiver;
  };
  $res = $conn->exec($last_resort);
  while (my @row = $res->fetchrow) {
    my ($provider) = @_;
    if ($PROVIDER{$origin}{$receiver}) {
      $PROVIDER{$origin}{$receiver}=$provider;
    } else {
      $PROVIDER{$origin}{$receiver}.=":" . $provider;
    }
  }
  return 1;
}
