#!/usr/bin/perl
# $Id: slon-tools.pm,v 1.1 2004-07-25 04:02:50 cbbrowne Exp $
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
  return $header
}
return 1;
