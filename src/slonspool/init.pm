#!/usr/bin/perl
# $Id: init.pm,v 1.1 2004-11-30 23:38:40 cbbrowne Exp $

# Data structures...
# %NODES{}{}
# Fields:
#  $NODE{$i}{last_event} - Last event processed for node
#
# %SET{}{}
# Fields:
#  $SET{$i}{origin}  - origin node
#  $SET{$i}{comment} - Comment about set
#  $SET{$i}{provider} - node that provides data to our favorite node
#
# %TABLES
#  $TABLES{$i}{name}
#  $TABLES{$i}{namespace}
#  $TABLES{$i}{set}

# Populate latest information about subscription providers and such...
sub load_configuration {
  my $dsn = "dbname=$database host=$host port=$port user=$user";
  if ($password) {
    $dsn .= " password=$password";
  }
  $dbh = Pg::connectdb($dsn);

  # Populate %NODE with confirmation information
  my $confirm_query = qq{ select con_origin, con_seqno from "_$cluster".sl_confirm where received = $node; };
  my $res = $dbh->exec($confirm_query);
  while (my @row = $res->fetchrow) {
    my ($origin, $sync) = @row;
    if ($NODE{$origin}{last_event} < $sync) {
      $NODE{$origin}{last_event} = $sync;
    }
  }

  # Populate %SET with set info for the sets being handled
  my $sub_set_query = qq{ select set_id, set_origin from "_$cluster".sl_set where set_id in ($opt_sets);};
  my $res = $dbh->exec($confirm_query);
  while (my @row = $res->fetchrow) {
    my ($set, $origin) = @row;
    $SET{$set}{origin} = $origin;
  }

  my $tables_query = qq{select t.tab_id, t.tab_set, n.nspname, r.relname from "_$cluster".sl_table t, pg_catalog.pg_namespace n, pg_catalog.pg_class r where r.oid = t.tab_reloid and n.oid = r.relnamespace and tab_set in ($opt_sets) ;};
  $res = $dbh->exec($tables_query);
  while (my @row = $res->fetchrow) {
    my ($id, $set, $namespace, $tname) = @row;
    $TABLES{$id}{name} = $tname;
    $TABLES{$id}{namespace} = $namespace;
    $TABLES{$id}{set} = $set;
  }
}

sub storeNode {
  my ($id, $comment) = @_;
  $NODES[$id] = $comment;
}

1;
