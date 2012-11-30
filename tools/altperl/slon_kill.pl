#!@@PERL@@
# 
# Kill all slon instances for the current cluster
# Author: Christopher Browne
# Copyright 2004-2009 Afilias Canada

use Getopt::Long;

# Defaults
$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;
$WATCHDOG_ONLY = 0;
$ONLY_NODE = 0;

# Read command-line options
GetOptions("config=s"   => \$CONFIG_FILE,
	   "help"       => \$SHOW_USAGE,
	   "w|watchdog" => \$WATCHDOG_ONLY,
	   "only-node=i" => \$ONLY_NODE);

my $USAGE =
"Usage: slon_kill [--config file] [-w|--watchdog] 

    --config file  Location of the slon_tools.conf file

    -w
    --watchdog     Only kill the watchdog process(es)

    Kills all running slon and slon_watchdog on this machine for every
    node in the cluster.

    --only-node=i Only kill slon processes for the indicated node
";

if ($SHOW_USAGE) {
  print $USAGE;
  exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

print "slon_kill.pl...   Killing all slon and slon_watchdog instances for the cluster $CLUSTER_NAME\n";
print "1.  Kill slon watchdogs\n";

$found="n";

# kill the watchdogs
if($ONLY_NODE) {
  kill_watchdog($ONLY_NODE);
} else {
  for my $nodenum (@NODES) {
    kill_watchdog($nodenum);
  }
}
if ($found eq 'n') {
    print "No watchdogs found\n";
}

unless ($WATCHDOG_ONLY) {
    print "\n2. Kill slon processes\n";

    # kill the slon daemons
    $found="n";

    if($ONLY_NODE) {
      kill_slon_node( $ONLY_NODE );
    } else {
      for my $nodenum (@NODES) {
        kill_slon_node( $nodenum );
      }
    }

    if ($found eq 'n') {
      print "No slon processes found\n";
    }
}

sub kill_watchdog($) {
  my ($nodenum) = @_;

  my $config_regexp = quotemeta( $CONFIG_FILE );

  my $command =  ps_args() . "| egrep \"[s]lon_watchdog[2]? .*=$config_regexp node$nodenum \" | awk '{print \$2}' | sort -n";

  #print "Command:\n$command\n";
  open(PSOUT, "$command|");

  while ($pid = <PSOUT>) {
      chomp $pid;
      if (!($pid)) {
          print "No slon_watchdog is running for the cluster $CLUSTER_NAME, node $nodenum!\n";
      } else {
          $found="y";
          kill 9, $pid;
          print "slon_watchdog for cluster $CLUSTER_NAME node $nodenum killed - PID [$pid]\n";
      }
  }
  close(PSOUT);
}

sub kill_slon_node($) {
  my ($nodenum) = @_;

  my $pid = get_pid($nodenum);

  #print "Command:\n$command\n";
  if (!($pid)) {
    print "No slon is running for the cluster $CLUSTER_NAME, node $nodenum!\n";
  } else {
    $found="y";
    kill 15, $pid;
    print "slon for cluster $CLUSTER_NAME node $nodenum killed - PID [$pid]\n";
  }
}
