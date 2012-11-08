# -*- perl -*-
# 
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use POSIX;
use Errno;
use File::Temp qw/ tempfile tempdir /;

sub add_node {
  my %PARAMS = (host=> undef,
		dbname => 'template1',
		port => 5432,
		user => 'postgres',
		node => undef,
		password => undef,
		parent => undef,
		noforward => undef,
		sslmode => undef,
		options => undef,
		config => undef
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
  my $sslmode = $PARAMS{'sslmode'};
  if ($sslmode) {
    $loginstr .= " sslmode=$sslmode";
    $SSLMODE[$node] = $sslmode;
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
  my $options = $PARAMS{'options'};
  if ($options) {
    $OPTIONS[$node] = $options;
  }
  my $config = $PARAMS{ 'config' };
  if ($config) {
    $CONFIG[$node] = $config;
  }
}

# This is the usual header to a slonik invocation that declares the
# cluster name and the set of nodes and how to connect to them.
sub genheader {
  my $header = "cluster name = $CLUSTER_NAME;\n";
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
  open(OUT, ">>$LOGDIR/slonik_scripts.log");
  my $now = `date`;
  chomp $now;
  print OUT "# -------------------------------------------------------------\n";
  print OUT "# Script: $script submitted at $now \n";
  print OUT "# -------------------------------------------------------------\n";
  print OUT $script;
  close OUT;
  print $script;
}

sub ps_args {
  my $sys=`uname`;
  chomp $sys;   # strip off edges
  if ($sys eq "Linux") {
    return "/bin/ps auxww";
  } elsif ($sys eq "FreeBSD") {
    return "/bin/ps -auxww";
  } elsif ($sys eq "SunOS") {
    return "/usr/ucb/ps -auxww";
  } elsif ($sys eq "AIX") {
    return "/usr/bin/ps auxww";
  } elsif ($sys eq "Darwin") {
    return "/bin/ps auxww";
  }
  return "/usr/bin/ps -auxww";    # This may be questionable for other systems; extend as needed!
}

sub get_pid {
  my ($node) = @_;
  $node =~ /^(?:node)?(\d+)$/;
  my $nodenum = $1;
  my $pid;
  my ($dsn, $config) = ($DSN[$nodenum], $CONFIG[$nodenum]);
  #  print "Searching for PID for $dbname on port $dbport\n";

  $PIDFILE_DIR ||= '/var/run/slony1';
  $PIDFILE_PREFIX ||= $CLUSTER_NAME;

  my $pidfile;
  $pidfile = "$PIDFILE_DIR/$PIDFILE_PREFIX" . "_node$nodenum.pid";

  open my $in, '<' , $pidfile or return '';

  while( <$in> ) {
    $pid = $_;
  }

  #print "Command:\n$command\n";
  chomp $pid;

  #make sure the pid actually exists
  kill(0,$pid);
  if ($! == Errno::ESRCH) {
	  return 0;
  }

  return $pid;
}

sub start_slon {
  my ($nodenum) = @_;
  my ($dsn, $dbname, $opts, $config) = ($DSN[$nodenum], $DBNAME[$nodenum], $OPTIONS[$nodenum], $CONFIG[$nodenum]);
  $SYNC_CHECK_INTERVAL ||= 1000;
  $DEBUGLEVEL ||= 0;
  $LOG_NAME_SUFFIX ||= '%Y-%m-%d';
  $PIDFILE_DIR ||= '/var/run/slony1';
  $PIDFILE_PREFIX ||= $CLUSTER_NAME;

  # system("mkdir -p $PIDFILE_DIR" );
  system("mkdir -p $LOGDIR/node$nodenum");

  my $cmd,$pidfile;

  $pidfile = "$PIDFILE_DIR/$PIDFILE_PREFIX" . "_node$nodenum.pid";

  if ($config) {
     $cmd = "@@SLONBINDIR@@/slon -p $pidfile -f $config ";
  } else {
     $cmd = "@@SLONBINDIR@@/slon -p $pidfile -s $SYNC_CHECK_INTERVAL -d$DEBUGLEVEL $opts $CLUSTER_NAME '$dsn' ";
  }
  my $logfilesuffix = POSIX::strftime( "$LOG_NAME_SUFFIX",localtime );
  chomp $logfilesuffix;

  if ($APACHE_ROTATOR) {
    $cmd .= "2>&1 | $APACHE_ROTATOR \"$LOGDIR/node$nodenum/" . $dbname . "-$logfilesuffix.log\" 10M &";
  } else {
    $cmd .= "> $LOGDIR/node$nodenum/$dbname-$logfilesuffix.log 2>&1 &";
  }
  print "Invoke slon for node $nodenum - $cmd\n";
  system ($cmd);
  # give time to slon daemon start and create pid file
  sleep 3;
}


$killafter="00:20:00";  # Restart slon after this interval, if there is no activity
sub query_slony_status {
  my ($nodenum) = @_;

# Old query - basically looked at how far we are behind
#   my $query = qq{
#   select now() - ev_timestamp > '$killafter'::interval as event_old, now() - ev_timestamp as age,
#        ev_timestamp, ev_seqno, ev_origin as origin
# from _$CLUSTER_NAME.sl_event events, _$CLUSTER_NAME.sl_subscribe slony_master
#   where 
#      events.ev_origin = slony_master.sub_provider and
#      not exists (select * from _$CLUSTER_NAME.sl_subscribe providers
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
from "_$CLUSTER_NAME".sl_confirm c, "_$CLUSTER_NAME".sl_subscribe slony_master
  where c.con_origin = slony_master.sub_provider and
             not exists (select * from "_$CLUSTER_NAME".sl_subscribe providers
                  where providers.sub_receiver = slony_master.sub_provider and
                        providers.sub_set = slony_master.sub_set and
                        slony_master.sub_active = 't' and
                        providers.sub_active = 't') and
        c.con_received = "_$CLUSTER_NAME".getLocalNodeId('_$CLUSTER_NAME') and
        now() - con_timestamp < '$killafter'::interval
limit 1) as slave_confirmed_events
union all (select
now() - con_timestamp < '$killafter'::interval, now() - con_timestamp as age,
       con_timestamp
from "_$CLUSTER_NAME".sl_confirm c, "_$CLUSTER_NAME".sl_subscribe slony_master
  where c.con_origin = "_$CLUSTER_NAME".getLocalNodeId('_$CLUSTER_NAME') and
             exists (select * from "_$CLUSTER_NAME".sl_subscribe providers
                  where providers.sub_provider = "_$CLUSTER_NAME".getLocalNodeId('_$CLUSTER_NAME') and
                        slony_master.sub_active = 't') and
        now() - con_timestamp < '$killafter'::interval
limit 1)
;
  };
  my ($port, $host, $dbname, $dbuser, $passwd)= ($PORT[$nodenum], $HOST[$nodenum], $DBNAME[$nodenum], $USER[$nodenum], $PASSWORD[$nodenum]);
  my $result;
  if ($passwd) {
     my ($fh, $filename) = tempfile();
     chmod( 0600, $filename);
     print $fh "$host:$port:$dbname:$dbuser:$passwd";
     close $fh;
     $result=`PGPASSFILE=$filename @@PGBINDIR@@/psql -p $port -h $host -U $dbuser -c "$query" --tuples-only $dbname`;
     unlink $filename;
  } else {
     $result=`@@PGBINDIR@@/psql -p $port -h $host -U $dbuser -c "$query" --tuples-only $dbname`;
  }
  chomp $result;
  #print "Query was: $query\n";
  #print "Result was: $result\n";
  return $result;
}

# This is a horrible function name, but it really *is* what it should
# be called.
sub get_set {
    my $set = shift();
    my $match;
    my $name;

    # If the variables are already set through $ENV{SLONYSET}, just
    # make sure we have an integer for $SET_ID
    if ($TABLE_ID) {
	return 0 unless $set =~ /^(?:set)?(\d+)$/;
	return $1;
    }

    # Die if we don't have any sets defined in the configuration file.
    unless (defined $SLONY_SETS
	    and ref($SLONY_SETS) eq "HASH"
	    and keys %{$SLONY_SETS}) {
	die "There are no sets defined in your configuration file.";
    }

    # Is this a set name or number?
    if ($SLONY_SETS->{$set}) {
	$match = $SLONY_SETS->{$set};
	$name  = $set;
    }
    elsif ($set =~ /^(?:set)?(\d+)$/) {
	$set = $1;
	($name) = grep { $SLONY_SETS->{$_}->{"set_id"} == $set } keys %{$SLONY_SETS};
	$match = $SLONY_SETS->{$name};
    }
    else {
	return 0;
    }

    # Set the variables for this set.
    $SET_NAME     = $name;
    $SET_ORIGIN   = ($match->{"origin"} or $MASTERNODE);
    $TABLE_ID     = $match->{"table_id"};
    $SEQUENCE_ID  = $match->{"sequence_id"};
    @PKEYEDTABLES = @{$match->{"pkeyedtables"}};
    %KEYEDTABLES  = %{$match->{"keyedtables"}};
    @SEQUENCES    = @{$match->{"sequences"}};
    $FOLD_CASE    = ($match->{"foldCase"} or 0);

	if(defined($match->{"serialtables"}) &&
	   scalar(@{$match->{"serialtables"}}) > 0 ) {
		   # slony generated primary keys have
		   # been deprecated.
		   #
		die "primary keys generated by slony (serialtables) are no longer "
			. "supported by slony-I. Please remove serialtables"
			.  "from your config file";
	}
    return $match->{"set_id"};
}

# This function checks to see if there is a still-in-progress subscription
# It does so by looking to see if there is a SUBSCRIBE_SET event corresponding
# to a sl_subscribe entry that is not yet active.
sub node_is_subscribing {
  my ($nodenum) = @_;
  my $query = qq{
select * from "_$CLUSTER_NAME".sl_event e, "_$CLUSTER_NAME".sl_subscribe s
where ev_origin = "_$CLUSTER_NAME".getlocalnodeid('_$CLUSTER_NAME') and  -- Event on local node
      ev_type = 'SUBSCRIBE_SET' and                            -- Event is SUBSCRIBE SET
      --- Then, match criteria against sl_subscribe
      sub_set::text = ev_data1 and sub_provider::text = ev_data2 and sub_receiver::text = ev_data3 and
      (case sub_forward when 'f' then 'f'::text when 't' then 't'::text end) = ev_data4
      --- And we're looking for a subscription that is not yet active
      and not sub_active
limit 1;   --- One such entry is sufficient...
};
  my ($port, $host, $dbname, $dbuser, $passwd)= ($PORT[$nodenum], $HOST[$nodenum], $DBNAME[$nodenum], $USER[$nodenum], $PASSWORD[$nodenum]);
  my $result;
  if ($passwd) {
     my ($fh, $filename) = tempfile();
     chmod(0600,$filename);
     print $fh "$host:$port:$dbname:$dbuser:$passwd";
     close $fh;
     $result=`PGPASSFILE=$filename @@PGBINDIR@@/psql -p $port -h $host -U $dbuser -c "$query" --tuples-only $dbname`;
     unlink $filename;
  } else {
     $result=`@@PGBINDIR@@/psql -p $port -h $host -c "$query" -U $dbuser --tuples-only $dbname`;
  }
  chomp $result;
  #print "Query was: $query\n";
  #print "Result was: $result\n";
  return $result;
}

1;
