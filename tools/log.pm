#!/usr/bin/perl
# 

$LOGDIR="/opt/logs/general";
`mkdir -p $LOGDIR`;
$HOST = `hostname`;
$IDENT = "-";
$USER = `whoami`;
$SIZE = "-";
$FLOG = "facts.log";
chomp($HOST, $IDENT, $USER, $SIZE);
my $sleep_seconds = 10;

sub initialize_flog {
  my ($cluster) = @_;
  open(OUTPUT, ">$LOGDIR/$cluster.$FLOG.tmp");
  print OUTPUT "# Initialize\n";
  close OUTPUT;
}
sub log_fact {
  my ($cluster, $fact) = @_;
  chomp $fact;
  open(OUTPUT, ">>$LOGDIR/$cluster.$FLOG.tmp");
  print OUTPUT $fact, "\n";
  close OUTPUT;
}

sub finalize_flog {
  my ($cluster) = @_;
  `mv $LOGDIR/$cluster.$FLOG.tmp $LOGDIR/$cluster.$FLOG`;
}

sub alog {
  my ($source, $dest, $message, $rc) = @_;
  chomp ($source, $dest, $message, $rc);

  #print"Master DB: $source  Slave DB: $dest  Message: [$message] RC=$rc\n"; 
  apache_log("replication.log", "Master DB: $source  Slave DB: $dest  Message: [$message]", $rc);
}

sub apache_log {
  my ($logfile, $request, $status) = @_;
  my $date = `date`;
  chomp($request, $status, $date);
  my $LOGENTRY ="$HOST $IDENT $USER [$date] \"$request\" $status $SIZE";
  open(OUTPUT, ">>$LOGDIR/$logfile");
  print OUTPUT $LOGENTRY, "\n";
  close OUTPUT;
}
