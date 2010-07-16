#!/usr/bin/perl
# #
# Documentation listed below.
# Credits:
# Afilias Canada
# Original script by jgoddard (2005-02-14)
# Modified by nadx (2005-05-11)
# Packaged by tgoodair (2005-05-12)

use strict;

use DBD::Pg;
use Getopt::Std;

our ($opt_h, $opt_d, $opt_p, $opt_U, $opt_w, $opt_c) = '';
my ($conn, $res, $status, @tuple);
my $query = 'SELECT * FROM replication_status' ;
my @rep_time;

# Issue a warning when replication is behind by 20 minutes
my $threshold_warning = 20;

# Issue a critical alert when replication is behind by 40 minutes
my $threshold_critical = 40;

# Get the command line options
getopt('hdpUwc');
if (not $opt_h or not $opt_d or not $opt_p or not $opt_U)
{
	print("$0 -h <host> -d <db> -p <port> -U <username> -w <warning threshold> -c <critical threshold>\n");
	exit(3);
}

if (($opt_w =~ /^\d+$/) && ($opt_w)) {$threshold_warning = $opt_w};
if (($opt_c =~ /^\d+$/) && ($opt_c)) {$threshold_critical = $opt_c};

if ($threshold_critical < $threshold_warning) { print "Warning: Critical threshold is less than warning threshold.\n"; }

# .pgpass isn't read, so we're putting the password here
my $password = "piTyThaF00!";

# Connect to the database
$conn = Pg::setdbLogin($opt_h, $opt_p, '', '', $opt_d, $opt_U, $password);
$status = $conn->status;

if ($status ne 'PGRES_CONNECTION_OK')
{
	chomp(my $error = $conn->errorMessage);
	print("$error\n");
	exit(2);
}

# Do the query
$res = $conn->exec($query);
$status = $res->resultStatus;
if ($status ne 'PGRES_TUPLES_OK')
{
	chomp(my $error = $conn->errorMessage);
	print("$error\n");
	exit(3);
}

# Get the results
# tuple[0]object
# tuple[1]transaction date time
# tuple[2]age in minutes old
@tuple = $res->fetchrow;

# Debugging
# Uncomment the below to swap the minute for seconds.  This is to simulate
# crazy replication times for when replication is not falling behind.
#$rep_time[1] = $rep_time[2]

# Check for a warning
if ($tuple[2] >= $threshold_warning and $tuple[2] < $threshold_critical)
{
	print("WARNING: $tuple[0], Created $tuple[1], Behind $tuple[2] minutes\n");
	exit(1);
}
# Or for a critical
elsif ($tuple[2] >= $threshold_critical)
{
	print("CRITICAL: $tuple[0], Created $tuple[1], Behind $tuple[2] minutes\n");
	exit(2);
}
# Otherwise, everything is ok
else
{
	printf("OK: $tuple[0], Created $tuple[1], Behind $tuple[2] minute%s\n",$tuple[2] == 1 ? "" : "s" );
	exit(0);
}

__END__

=head1 NAME

Slony Postgres Replication Check

=head1 VERSION

2.0

=head1 SYNOPSIS

This check connects directly to the database.  It is designed to check to see how far behind Slony replication is
on the database servers based on the last transaction.  

Sample view definition:
CREATE VIEW replication_status AS
SELECT customer_name AS object_name, 
transaction_date, 
(date_part('epoch'::text, now() - transaction_date) / 60::double precision)::integer AS age
FROM customer_orders
ORDER BY id DESC
LIMIT 1;

Modify the view for your environment. The idea is that you pick a table which has frequent write transactions and contains some
kind of time stamp. Then create a replication_status view on the table which returns some kind of identifier (like a customer name),
the time of the last transaction and the age of the last transaction in minutes.

For security reasons, you should create a nagios postgres user and only grant it select privileges on this view.

Query is as follows:
SELECT * FROM replication_status

The results displayed:
 object_name |        transaction_date        | age
-------------+------------------------+-----
 "B.A." Baracus   | 2005-05-10 15:09:57+00 |   0

The age column displays age of the last transaction in minutes.  We have tentatively set the warning threshold in Nagios to 20 minutes
and the critical threshold to 40 minutes.  

=head1 USAGE

USAGE: psql_replication_check.pl -h <host> -d <db> -p <port> -U <username>

psql_replication_check.pl = script name
-h = hostname of database server
-d = databse
-p = database port
-U = username

=cut
