#!perl     # -*- perl -*-
# $Id: slon-tools.pm,v 1.9 2004-09-09 17:04:07 cbbrowne Exp $
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
  print OUT "# ------------------------------------------------------------- \n";
  print OUT "# Script: $script submitted at $now \n";
  print OUT "# ------------------------------------------------------------- \n";
  close OUT;
  `cat $script >> $LOGDIR/slonik_scripts.log`;
  print `$SLON_BIN_PATH/slonik < $script`;
  unlink($script);
}

sub ps_args {
  my $sys=`uname`;
  chomp $sys;    # Strip off cruft
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
  my ($retpid, $pid);
  my ($dbname, $dbport, $dbhost) = ($DBNAME[$nodenum], $PORT[$nodenum], $HOST[$nodenum]);
  #  print "Searching for PID for $dbname on port $dbport\n";
  open(PSOUT, ps_args() . "| egrep \"[s]lon .*$SETNAME\" | egrep \"host=$dbhost dbname=$dbname.*port=$dbport\" | sort -n | awk '{print \$2}'|");
  while ($pid = <PSOUT>) {
    chomp $pid;
    $retpid = $pid;
  }
  close(PSOUT);
  return $retpid;
}

sub start_slon {
  my ($nodenum) = @_;
  my ($dsn, $dbname) = ($DSN[$nodenum], $DBNAME[$nodenum]);
  my $cmd;
  if ($APACHE_ROTATOR) {
    $cmd = "$SLON_BIN_PATH/slon -s 1000 -d2  $SETNAME '$dsn' 2>&1 | $APACHE_ROTATOR \"$LOGDIR/slony1/node$nodenum/$dbname_%Y-%m-%d_%H:%M:%S.log\" 10M &";
  } else {
    $cmd = "$SLON_BIN_PATH/slon -s 1000 -d2  $SETNAME '$dsn' 2>&1 > $LOGDIR/slony1/node$nodenum/$dbname.log &";
  }
  print "Invoke slon: $cmd\n";
  system $cmd;
}



return 1;
