#!perl     # -*- perl -*-
# $Id: slon-tools.pm,v 1.13 2004-12-10 18:44:31 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

sub add_node {
  my %PARAMS = (host=> undef,
		dbname => 'template1',
		port => 5432,
		user => 'postgres',
		node => undef,
		password => undef,
		parent => 1,
		noforward => undef
	       );
  my $K;
  while ($K= shift) {
    $PARAMS{$K} = shift;
  }
   die ("I need a node number") unless $PARAMS{'node'};
  my $node = $PARAMS{'node'};
  push @NODES, $node;
  my $loginstr;
  my $host = $PARAMS{'host'};
  if ($host) {
    $loginstr .= "host=$host";
    $HOST[$node] = $host;
  } else {
    die("I need a host name") unless $PARAMS{'host'};
  }
  my $dbname = $PARAMS{'dbname'};
  if ($dbname) {
    $loginstr .= " dbname=$dbname";
    $DBNAME[$node] = $dbname;
  }
  my $user=$PARAMS{'user'};
  $loginstr .= " user=$user";
  $USER[$node]= $user;

  my $port = $PARAMS{'port'};
  if ($port) {
    $loginstr .= " port=$port";
    $PORT[$node] = $port;
  } else {
    die ("I need a port number");
  }
  my $password = $PARAMS{'password'};
  if ($password) {
    $loginstr .= " password=$password";
    $PASSWORD[$node] = $password;
  }
  $DSN[$node] = $loginstr;
  my $parent = $PARAMS{'parent'};
  if ($parent) {
    $PARENT[$node] = $parent;
  }
  my $noforward = $PARAMS{'noforward'};
  if ($noforward) {
    $NOFORWARD[$node] = $noforward;
  }
}

# This is the usual header to a slonik invocation that declares the
# cluster name and the set of nodes and how to connect to them.
sub genheader {
  my $header = "cluster name = $SETNAME;\n";
  foreach my $node (@NODES) {
    if ($DSN[$node]) {
      my $dsn = $DSN[$node];
      $header .= " node $node admin conninfo='$dsn';\n";
    }
  }
  return $header;
}

# Stores copy of slonik script in log file in $LOGDIR
# then invokes it and deletes it
sub run_slonik_script {
  my ($script) = @_;
  chomp $script;
  open(OUT, ">>$LOGDIR/slonik_scripts.log");
  my $now = `date`;
  chomp $now;
  print OUT "# -------------------------------------------------------------\n";
  print OUT "# Script: $script submitted at $now \n";
  print OUT "# -------------------------------------------------------------\n";
  close OUT;
  `cat $script >> $LOGDIR/slonik_scripts.log`;
  #print `slonik < $script`;
  print `cat $script`;
  unlink($script);
}

sub ps_args {
  my $sys=`uname`;
  chomp $sys;   # strip off edges
  if ($sys eq "Linux") {
    return "/bin/ps -auxww";
  } elsif ($sys eq "FreeBSD") {
    return "/bin/ps -auxww";
  } elsif ($sys eq "SunOS") {
    return "/usr/ucb/ps -auxww";
  } elsif ($sys eq "AIX") {
    return "/usr/bin/ps auxww";
  } 
  return "/usr/bin/ps -auxww";    # This may be questionable for other systems; extend as needed!    
}

sub get_pid {
  my ($node) = @_;
  $node =~ /node(\d*)$/;
  my $nodenum = $1;
  my $pid;
  my $tpid;
  my ($dbname, $dbport, $dbhost) = ($DBNAME[$nodenum], $PORT[$nodenum], $HOST[$nodenum]);
  #  print "Searching for PID for $dbname on port $dbport\n";
  my $command =  ps_args() . "| egrep \"[s]lon .*$SETNAME\" | egrep \"host=$dbhost dbname=$dbname.*port=$dbport\" | sort -n | awk '{print \$2}'";
  #print "Command:\n$command\n";
  open(PSOUT, "$command|");
  while ($tpid = <PSOUT>) {
    chomp $tpid;
    $pid = $tpid;
  }
  close(PSOUT);
  return $pid;
}

sub start_slon {
  my ($nodenum) = @_;
  my ($dsn, $dbname) = ($DSN[$nodenum], $DBNAME[$nodenum]);
  my $cmd;
  `mkdir -p $LOGDIR/slony1/node$nodenum`;
  if ($APACHE_ROTATOR) {
    $cmd = "$SLON_BIN_PATH/slon -s 1000 -d2 $SETNAME '$dsn' 2>&1 | $APACHE_ROTATOR \"$LOGDIR/slony1/node$nodenum/" . $dbname . "_%Y-%m-%d_%H:%M:%S.log\" 10M&";
  } else {
    $cmd = "$SLON_BIN_PATH/slon -s 1000 -d2 $SETNAME '$dsn' 2>&1 > $LOGDIR/slony1/node$nodenum/$dbname.log &";
  }
  print "Invoke slon for node $nodenum - $cmd\n";
  system $cmd;
}


$killafter="00:20:00";  # Restart slon after this interval, if there is no activity
sub query_slony_status {
  my ($nodenum) = @_;

# Old query - basically looked at how far we are behind
#   my $query = qq{
#   select now() - ev_timestamp > '$killafter'::interval as event_old, now() - ev_timestamp as age,
#        ev_timestamp, ev_seqno, ev_origin as origin
# from _$SETNAME.sl_event events, _$SETNAME.sl_subscribe slony_master
#   where 
#      events.ev_origin = slony_master.sub_provider and
#      not exists (select * from _$SETNAME.sl_subscribe providers
#                   where providers.sub_receiver = slony_master.sub_provider and
#                         providers.sub_set = slony_master.sub_set and
#                         slony_master.sub_active = 't' and
#                         providers.sub_active = 't')
# order by ev_origin desc, ev_seqno desc limit 1;
# };

# New query: Looks to see if an event has been confirmed, for the set,
# for the master node, within the interval requested

  my $query = qq{
select * from 
(select now() - con_timestamp < '$killafter'::interval, now() - con_timestamp as age,
       con_timestamp
from _$SETNAME.sl_confirm c, _$SETNAME.sl_subscribe slony_master
  where c.con_origin = slony_master.sub_provider and
             not exists (select * from _$SETNAME.sl_subscribe providers
                  where providers.sub_receiver = slony_master.sub_provider and
                        providers.sub_set = slony_master.sub_set and
                        slony_master.sub_active = 't' and
                        providers.sub_active = 't') and
        c.con_received = _$SETNAME.getLocalNodeId('_$SETNAME') and
        now() - con_timestamp < '$killafter'::interval
limit 1) as slave_confirmed_events
union all (select
now() - con_timestamp < '$killafter'::interval, now() - con_timestamp as age,
       con_timestamp
from _$SETNAME.sl_confirm c, _$SETNAME.sl_subscribe slony_master
  where c.con_origin = _$SETNAME.getLocalNodeId('_$SETNAME') and
             exists (select * from _$SETNAME.sl_subscribe providers
                  where providers.sub_provider = _$SETNAME.getLocalNodeId('_$SETNAME') and
                        slony_master.sub_active = 't') and
        now() - con_timestamp < '$killafter'::interval
limit 1)
;
  };
  my ($port, $host, $dbname)= ($PORT[$nodenum], $HOST[$nodenum], $DBNAME[$nodenum]);
  my $result=`$SLON_BIN_PATH/psql -p $port -h $host -c "$query" --tuples-only $dbname`;
  chomp $result;
  #print "Query was: $query\n";
  #print "Result was: $result\n";
  return $result;
}

# This function checks to see if there is a still-in-progress subscription
# It does so by looking to see if there is a SUBSCRIBE_SET event corresponding
# to a sl_subscribe entry that is not yet active.
sub node_is_subscribing {
  my $see_if_subscribing = qq {
select * from "_$SETNAME".sl_event e, "_$SETNAME".sl_subscribe s
where ev_origin = "_$SETNAME".getlocalnodeid('_$SETNAME') and  -- Event on local node
      ev_type = 'SUBSCRIBE_SET' and                            -- Event is SUBSCRIBE SET
      --- Then, match criteria against sl_subscribe
      sub_set = ev_data1 and sub_provider = ev_data2 and sub_receiver = ev_data3 and
      (case sub_forward when 'f' then 'f'::text when 't' then 't'::text end) = ev_data4

      --- And we're looking for a subscription that is not yet active
      and not sub_active
limit 1;   --- One such entry is sufficient...
};
  my ($port, $host, $dbname)= ($PORT[$nodenum], $HOST[$nodenum], $DBNAME[$nodenum]);
  my $result=`$SLON_BIN_PATH/psql -p $port -h $host -c "$query" --tuples-only $dbname`;
  chomp $result;
  #print "Query was: $query\n";
  #print "Result was: $result\n";
  return $result;
}

1;
