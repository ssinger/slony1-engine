#!/usr/bin/perl
# $Id: subscribe.pm,v 1.1 2004-11-30 23:38:41 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

sub subscribe_to_node {
  open(SUBSCRIBE, ">$spoolpath/subscription.log");
  foreach $set (@SETS) {

    # Create namespaces
    print SUBSCRIBE "-- Subscribing node $node to set $set on $host - $port/$database/$user\n";
    my $sth = $dbh->exec("select distinct nspname from pg_class p, pg_namespace n, _oxrsorg.sl_table t where t.tab_reloid = p.oid and p.relnamespace = n.oid and tab_set = $set;");
    while ( @row = $sth->fetchrow ) {
      my ($namespace) = @row;
      print SUBSCRIBE "create schema $namespace;\n";
    }
    close(SUBSCRIBE);

    # Create tables
    $sth = $dbh->exec("select nspname || '.' || relname from pg_class p, pg_namespace n, _oxrsorg.sl_table t where t.tab_reloid = p.oid and p.relnamespace = n.oid and tab_set = $set;");
    while ( @row = $sth->fetchrow ) {
      my ($table) = @row;
      `$pgbins/pg_dump -p $port -h $host -U $user -t $table -s $database >> $spoolpath/subscription.log`;
    }
    open(SUBSCRIBE, ">>$spoolpath/subscription.log");

    # Pull data, as in copy_set (remote_worker.c)
    my $query = "begin transaction; set transaction isolation level serializable;";
    $sth = $dbh->exec($query);
    my $tquery = qq{
		    select T.tab_id,
		    "pg_catalog".quote_ident(PGN.nspname) || '.' ||
		    "pg_catalog".quote_ident(PGC.relname) as tab_fqname,
		    T.tab_idxname, T.tab_comment
		    from "$cluster".sl_table T,
		    "pg_catalog".pg_class PGC,
		    "pg_catalog".pg_namespace PGN
		    where T.tab_set = $set
		    and T.tab_reloid = PGC.oid
		    and PGC.relnamespace = PGN.oid
		    order by tab_id;
		   };
  }
  $sth=$dbh->exec($tquery);
  while (@row=$sth->fetchrow) {
    my ($table) = @row;
    print SUBSCRIBE qq{copy "$table" from stdin;\n};
    my $query = qq{copy "$table" to stdout;};
    $res = $dbh->exec($query);
    my $line = "*" x 16384;
    $ret = $dbh->getline($line, 16384);
    while ($line ne "\\.") {
      print SUBSCRIBE line, "\n";
      $ret = $dbh->getline($line, 16384);
    }
    print SUBSCRIBE "\.\n";
  }
  close SUBSCRIBE;
  my $seqquery = qq{
    select n.nspname, c.relname
    from "pg_catalog".pg_class c, "pg_catalog".pg_namespace, "$cluster".sl_sequence s
    where
      n.oid = c.relnamespace and
      c.oid = s.seq_reloid and
      s.seq_set = $set;};
  $sth=$dbh->exec($seqquery);
  while (my @row=$sth->fetchrow) {
    my ($nsp, $seqname) = @row;
    `$pgbins/pg_dump -p $port -h $host -U $user -n $nsp -t $seqname $database >> $spoolpath/subscription.log`;
  }
  # Next, populate Sync information
  # Use the last SYNC's snapshot information and set
  # the action sequence list to all actions after
  # that.

  my $squery = qq{
    select ssy_seqno, ssy_minxid, ssy_maxxid,
           ssy_xip, ssy_action_list
    from "$cluster".sl_setsync
    where ssy_setid = $set; };
  $sth=$dbh->exec($squery);
  while (my @row=$sth->fetchrow) {
    my ($seqno, $minxid, $maxxid, $xip, $actionlist) = @row;
  }
  my $createsync = qq{
    insert into "_$cluster".sl_setsync
	   (ssy_setid, ssy_origin, ssy_seqno, ssy_minxid, ssy_maxxid, ssy_xip, ssy_action_list)
    values ($set, $node, $seqno, $minxid, $maxxid, '$xip', '$actionlist');};
  print SUBSCRIBE $createsync, "\n";
}

1;
