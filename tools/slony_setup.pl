#!/usr/bin/env perl -w

# Copyright (c) 2003-2004, PostgreSQL Global Development Group

# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose, without fee, and without a written agreement
# is hereby granted, provided that the above copyright notice and this
# paragraph and the following two paragraphs appear in all copies.

# IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
# LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
# DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=head1 NAME

slony_setup.pl - A script for setting up Slony postgres replication

=head1 DESCRIPTION

Slony setup is intended to facilitate the installation of the slony
replication engine. Basically it asks a bunch of questions and creates
a shell (bash) script which can then be run on the master and also
provides the commands to be run on slaves.

The initial goal for slony_setup.pl is to allow a postgres DBA to start
with:

1. A postgres master host that needs replication.

2. N slaves hosts.

3. Every postgres host allows authenticated connections from all
participating hosts over TCP/IP.

4. A "slony" postgres superuser and a "slony" system user on every
participating host.

Given the above, the setup script should then create the necessary
script to effect complete slony replication.

=head1 BUGS

Several, I'm sure...

=head1 TODO

Make the version check use psql -c 'select version ()'.

Add support for running all commands to slaves over ssh.

Allow the script to execute the commands in perl at the end of
the script.

Add more checking to see if databases, tables, users, groups, and
languages are setup on each slave, that the sysids match, etc.

Uninstall option for individual nodes.

Figure out the dependencies when not all tables in a given database are
selected.

Cascading of slaves.

Add support for generating failover scripts.

=head1 CHANGELOG

05-28-2004

Added support for replicating sequences.
Made the queries that get the table names and sequence names use quote_ident()
Fixed some cosmetic issues.

05-06-2004

Initial CVS import

=head1 AUTHOR

Thoughts? Send them to Daniel Ceregatti (AKA Primer in #postgresql on
irc.freenode.net) at vi at sh dot nu.

=cut

use strict;
eval {
	require Data::Dumper;
};

my $dumper = ($@) ? 0 : 1;

$|= 1;

$SIG{TERM} = \&clean_up;
$SIG{INT} = \&clean_up;
$SIG{KILL} = \&clean_up;

#
# Create a temporary working directory and set some variables
#

my $exitcode = -1;
my $tmpdir = "/tmp/slony.$$";
mkdir $tmpdir,0700 or &death ("Can't create temporary working directory: $!");

my $pgpassfile = $ENV{"HOME"} . "/.pgpass";
my $pgpassbackup = "$tmpdir/.pgpass.backup";
my $dump_file = "/tmp/slony_dump";

#
# Make sure we have the correct version
#

my $psql = "";

my $ret = &check_version;

if ($ret == 1) {
	print "\nYou must have PostgreSQL version 7.3 or greater to use slony\n\n";
	&clean_up;
} elsif ($ret == 2) {
	print "\nThe psql executable was not found. PostgreSQL must be installed\n";
	print "and its executables should be in your PATH\n\n";
	&clean_up;
}

#
# Print the README
#

my $text = qq (
Slony setup will guide you through the process of setting up the slony-engine
replication system.  Some presumptions will be made in order to keep this
installation process as simple as possible.  The default values may be over-
ridden.  Slony requires, and the install process presumes that:

1. Your master server listens on TCP/IP.
2. At least one slave listens on TCP/IP.
3. You have a "slony" postgres superuser (createuser -a -d -P slony) as well
   as a "slony" system user on all participating hosts.
4. Each host participating in replication allows every other host that is
   participating in the replication to connect and authenticate to postgres
   as the slony postgres user over TCP/IP.
5. Slony binaries have been installed on all participating hosts.

Optional, but helpful:

1. The user that will run the slony processes on all hosts has remote ssh
   access from the host slony_setup.sh runs.

You will be prompted for the following information:

1. The hostname of the master server.
2. The hostnames of slave servers.
3. Authentication credentials for all hosts.

Once this information is obtained, the script will then check connectivity to
these hosts, look for available databases, and prompt for addition of these
databases and their tables into replication.

When all the information needed to complete the setup is obtained, the process
will then prompt the user to run the commands to finalize the installation.
The user need not finalize the installation at that time, for the process will
write a perl script to a file that can then be run to create the replication
without running slony_setup.pl again.
);

&pager ($text);

print "\nAre you ready to proceed? (Y|n) ";
&clean_up if ! &get_one_bool;

#
# Backup the ~/.pgpass, if it exists
#

if (-f $pgpassfile) {
	&backup_file ($pgpassfile, $pgpassbackup);
}

#
# Check for a previous failed run and import that data
#

my %data;
my $cache = 0;

if (-f $dump_file) {
	print "\nA previous instance of slony_setup data was detected.\n";
	print "Do you want to import this data? (Y|n) ";
	if (&get_one_bool) {
		open F, $dump_file and do {
			my $text = join ('', <F>);
			close F;
			my $VAR1;
			eval $text;
			%data = %{$VAR1};
			$cache = 1;
		}
	}
}

#
# Other global variables

my $which = "master";
my $slavenumber = 0;
my $slave_commands = "";
my $same = 0;

#
# Loop until we get good connections
#

while (1) {

	if ($same == 0 && ($cache == 0 || !$data{$which})) {
		$data{$which}{"hostname"} = "none";
		$data{$which}{"port"} = 5432;
		$data{$which}{"databasename"} = "template1";
		$data{$which}{"username"} = "slony";
		$data{$which}{"password"} = "";
		$same=1
	}

	#
	# Ask the user about the master database
	#

	&get_info ($which);

	#
	# Test the connection
	#

	$ret = &test_conn ($which);

	#
	# Prompt the user to retry if the connection fails
	#

	if ($ret != 0) {
		print "\n\nConnection to $which failed. Try again? (Answering NO will abort) (Y|n) ";
		&clean_up if ! &get_one_bool;
	} else {
		print "\n\n$which connection successfull\n";
		&get_databases ($which);
		$slavenumber++;
		$which = "slave_$slavenumber";
		$same = 0;
		if ($slavenumber > 1) {
			print "\nAdd another slave? (Y|n) ";
			last if (!&get_one_bool);
		}
	}
}

#
# Ask the user if all databases in the "instance" should be replicated
#

print "\nShould all databases on $data{'master'}{'hostname'} be replicated? (Y|n) ";

#
# Loop over the databases and prompt for addition into replication
#

if (!&get_one_bool) {
	print "\nAdd databases to replication:\n";
	foreach my $database (keys %{$data{"master"}{"databases"}}) {
		print " Add $database? (Y|n) ";
		if (!&get_one_bool) {
			delete $data{"master"}{"databases"}{$database};
		}
	}
}

#
# Loop over the databases and ask the user if all tables in the database
# should be replicated. We ask this here because the user may want to
# replicate all databases but not all tables.
#

foreach my $database (keys %{$data{"master"}{"databases"}}) {
	&get_tables ("master", $database);
	&get_sequences ("master", $database);

	#
	# Loop over the tables and prompt for addition
	#
	
	print "\nShould all tables in the $database database be replicated?
(Note: the script cannot guarantee the schema will be properly
 installed on slaves if you choose No) (Y|n) ";
	
	if (&get_one_bool) {
		$data{"master"}{"databases"}{$database}{"all_tables"} = 1;
	} else {
		print "\nAdd tables in database $database to replication:\n";
		foreach my $table (keys %{$data{"master"}{"databases"}{$database}{"tables"}}) {
			print " Add $database.$table? (Y|n) ";
			delete $data{"master"}{"databases"}{$database}{"tables"}{$table} if ! &get_one_bool;
		}
	}

	#
	# Loop over the sequences and prompt for addition
	#
	
	print "\nShould all sequences in the $database database be replicated? (Y|n) ";
	
	if (&get_one_bool) {
		$data{"master"}{"databases"}{$database}{"all_sequences"} = 1;
	} else {
		print "\nAdd sequences in database $database to replication:\n";
		foreach my $sequence (keys %{$data{"master"}{"databases"}{$database}{"sequences"}}) {
			print " Add $sequence? (Y|n) ";
			delete $data{"master"}{"databases"}{$database}{"sequences"}{$sequence} if ! &get_one_bool;
		}
	}
}

#
# Ensure that if no tables were added manually that the database isn't replicated
#

foreach my $database (keys %{$data{"master"}{"databases"}}) {
	delete $data{"master"}{"databases"}{$database} if ! keys %{$data{"master"}{"databases"}{$database}{"tables"}};
}

#
# Check all tables for a "primary key" ?
#

my $summary = "\n\nSummary:";

for my $datum (keys %data) {
	
	$summary .= "

$datum Hostname: " . $data{$datum}{"hostname"} . "
$datum Port:     " . $data{$datum}{"port"} . "
$datum Username: " . $data{$datum}{"username"} . "
$datum Password: VALIDATED";

}

foreach my $database (keys %{$data{"master"}{"databases"}}) {
	$summary .=  "\n\nThe following tables in database $database will be replicated:\n\n";
	foreach my $table (keys %{$data{"master"}{"databases"}{$database}{"tables"}}) {
		$summary .=  "    $table\n";
	}
	$summary .=  "\n\nThe following sequences in database $database will be replicated:\n\n";
	foreach my $sequence (keys %{$data{"master"}{"databases"}{$database}{"sequences"}}) {
		$summary .=  "    $sequence\n";
	}
	$summary .=  "\n";
}

&pager ($summary);

print "Save? (Y|n) ";

#
# Get the users on the master
#

&get_users ("master");

&get_groups ("master");

#
# Create an array with slave info
#

foreach my $slave (keys %data) {
	next if $slave eq "master";
	$slave =~ m/slave_(\d+)/;
	$data{"slaves"}{$1} = $data{$slave};
}

if (&get_one_bool) {

	#
	# Make this a variable!! FIXME!!
	#
  open F, ">/tmp/slony_master_setup.sh" or death ("Can't open /tmp/slony_master_setup.sh: $!");
	print F "#/bin/bash\n\n";
	
	#
	# Start by adding the complete uninstall option
	#

	$data{"master"}{"coninfo"} = "'dbname=DATABASE_NAME_HOLDER host=" . $data{"master"}{"hostname"} .
		" port=" . $data{"master"}{"port"} .
		" user=" . $data{"master"}{"username"};
	$data{"master"}{"coninfo"} .= " password=" . $data{"master"}{"password"} if $data{"master"}{"password"} !~ /(\s+|\*)/;
	$data{"master"}{"coninfo"} .= "'";

	my $all_conn = "slonik <<_EOF_ 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n\tcluster name = T1;
	node 1 admin conninfo = " . $data{"master"}{"coninfo"} . ";\n";

	foreach my $slave (sort keys %{$data{"slaves"}}) {
		$data{"slaves"}{$slave}{"coninfo"} = "'dbname=DATABASE_NAME_HOLDER host=" . $data{"slaves"}{$slave}{"hostname"} .
			" port=" . $data{"slaves"}{$slave}{"port"} .
			" user=" . $data{"slaves"}{$slave}{"username"};
		$data{"slaves"}{$slave}{"coninfo"} .= " password=" . $data{"slaves"}{$slave}{"password"} if $data{"slaves"}{$slave}{"password"} !~ /(\s+|\*)/;
		$data{"slaves"}{$slave}{"coninfo"} .= "'";
		$all_conn .= "\tnode " . ($slave + 1) . " admin conninfo = " . $data{"slaves"}{$slave}{"coninfo"} . ";\n";
	}
	
	print F "if [ x\$1 = \"xuninstall\" ]\nthen\n";
	
	foreach my $database (keys %{$data{"master"}{"databases"}}) {
		my $conn = $all_conn;
		$conn =~ s/DATABASE_NAME_HOLDER/$database/g;
		print F $conn . "\ttry {
		uninstall node (id = 1);
	}
	on error {
		echo 'Could not uninstall slony on node 1';
		exit -1;
	}
";

		foreach my $slave (sort keys %{$data{"slaves"}}) {
			print F "\ttry {
		uninstall node (id = " . ($slave + 1) . ");
	}
	on error {
		echo 'Could not uninstall Slony on node " . ($slave + 1) . "';
		exit -1;
	}
";
		}
	
		print F "\techo 'Slony successfully uninstalled on database $database';
_EOF_

if [ \$? -ne 0 ]
then
	echo Errors were detected. Please review /tmp/slony_setup.log.  Uninstall halted.
	exit -1
fi

";
	
	}

	print F "exit 0
fi

rm -f ~/.pgpass
rm -f /tmp/slony_setup.log

";

	#
	# Create the new ~/.pgpass
	#
	open FH, $pgpassfile or death ("Can't open $pgpassfile: $!");
	while (<FH>) {
		chomp;
		print F "echo \"$_\" >> ~/.pgpass\n";
	}
	close FH;
	print F "\nchmod 600 ~/.pgpass\n\n";

	foreach my $slave (keys %{$data{"slaves"}}) {
		#
		# Create all the users that exist on the master database on each slave, except the
		# postgres and slony users
		#
		foreach my $user (sort keys %{$data{"master"}{"users"}}) {
			print F "$psql" .
				" -h " . $data{"slaves"}{$slave}{"hostname"} .
				" -p " . $data{"slaves"}{$slave}{"port"} .
				" -U " . $data{"slaves"}{$slave}{"username"} .
				" -d template1" .
				" -c \\\n\"insert into pg_shadow (usename, usesysid, usecreatedb, usesuper, usecatupd, passwd, valuntil, useconfig) values (\\\n" .
				"'" . $data{"master"}{"users"}{$user}{"usename"} . "'," .
#				"'" . $data{"master"}{"users"}{$user}{"usesysid"} . "'," .
				"(select case when max (usesysid) + 1 < 100 then 100 else max (usesysid) + 1 end from pg_shadow),\\\n" .
				"'" . $data{"master"}{"users"}{$user}{"usecreatedb"} . "'," .
				"'" . $data{"master"}{"users"}{$user}{"usesuper"} . "'," .
				"'" . $data{"master"}{"users"}{$user}{"usecatupd"} . "'," .
				"'" . $data{"master"}{"users"}{$user}{"passwd"} . "'," .
				(($data{"master"}{"users"}{$user}{"valuntil"}) ? "'" . $data{"master"}{"users"}{$user}{"valuntil"} . "'": "null") . "," .
				(($data{"master"}{"users"}{$user}{"useconfig"}) ? "'" . $data{"master"}{"users"}{$user}{"useconfig"} . "'": "null") . ")\"" .
				" 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
			print F "\nif [ \$? -ne 0 ]\n";
			print F "then\n";
			print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
			print F "\texit -1\n";
			print F "fi\n\n";
		}
		#
		# Create all the groups that exist on the master database on each slave
		#
		foreach my $group (sort keys %{$data{"master"}{"groups"}}) {
			print F "$psql" .
				" -h " . $data{"slaves"}{$slave}{"hostname"} .
				" -p " . $data{"slaves"}{$slave}{"port"} .
				" -U " . $data{"slaves"}{$slave}{"username"} .
				" -d template1" .
				" -c \\\n\"insert into pg_group (groname, grosysid, grolist) values (\\\n" .
				"'" . $data{"master"}{"groups"}{$group}{"groname"} . "'," .
				"'" . $data{"master"}{"groups"}{$group}{"grosysid"} . "'," .
				"'" . $data{"master"}{"groups"}{$group}{"grolist"} . "')\"" .
				" 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
			print F "\nif [ \$? -ne 0 ]\n";
			print F "then\n";
			print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
			print F "\texit -1\n";
			print F "fi\n\n";
		}
		#
		# Make sure plpgsql is created in template1 on all slaves
		#
		print F "createlang" .
			" -h " . $data{"slaves"}{$slave}{"hostname"} .
			" -p " . $data{"slaves"}{$slave}{"port"} .
			" -U " . $data{"slaves"}{$slave}{"username"} .
			" plpgsql template1" .
			" 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
		print F "\nif [ \$? -ne 0 ]\n";
		print F "then\n";
		print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
		print F "\texit -1\n";
		print F "fi\n\n";
		foreach my $database (keys %{$data{"master"}{"databases"}}) {
			#
			# Create the databases to be replicated on all slaves
			#
			print F "createdb" .
				" -h " . $data{"slaves"}{$slave}{"hostname"} .
				" -p " . $data{"slaves"}{$slave}{"port"} .
				" -U " . $data{"slaves"}{$slave}{"username"} .
				" -O " . $data{"master"}{"databases"}{$database}{"owner"} .
				" $database 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
			print F "\nif [ \$? -ne 0 ]\n";
			print F "then\n";
			print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
			print F "\texit -1\n";
			print F "fi\n\n";
			#
			# Use the command that copies the entire schema at once, as this assures us
			# that dependencies will be done in order
			#
			if ($data{"master"}{"databases"}{$database}{"all_tables"} == 1) {
				print F "pg_dump" .
					" -h " . $data{"master"}{"hostname"} .
					" -p " . $data{"master"}{"port"} .
					" -U " . $data{"master"}{"username"} .
					" -s " . $database .
					" 2>> /tmp/slony_setup.log | \\\n" .
					"psql" .
					" -h " . $data{"slaves"}{$slave}{"hostname"} .
					" -p " . $data{"slaves"}{$slave}{"port"} .
					" -U " . $data{"slaves"}{$slave}{"username"} .
					" -d " . $database .
					" 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
				print F "\nif [ \$? -ne 0 ]\n";
				print F "then\n";
				print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
				print F "\texit -1\n";
				print F "fi\n\n";
			#
			# Else, do it the hard way, and hope no dependencies are broken
			#
			} else {
				foreach my $table (keys %{$data{"master"}{"databases"}{$database}{"tables"}}) {
					my ($schema, $tablename) = split (/\./, $table);
					print F "pg_dump" .
						" -h " . $data{"master"}{"hostname"} .
						" -p " . $data{"master"}{"port"} .
						" -U " . $data{"master"}{"username"} .
						" -n " . $schema .
						" -t " . $tablename .
						" -s " . $database .
						" 2>> /tmp/slony_setup.log | \\\n" .
						"psql" .
						" -h " . $data{"slaves"}{$slave}{"hostname"} .
						" -p " . $data{"slaves"}{$slave}{"port"} .
						" -U " . $data{"slaves"}{$slave}{"username"} .
						" -d " . $database .
						" 2>> /tmp/slony_setup.log 1>> /tmp/slony_setup.log\n";
					print F "\nif [ \$? -ne 0 ]\n";
					print F "then\n";
					print F "\techo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.\n";
					print F "\texit -1\n";
					print F "fi\n\n";
				}
			}
		}
	}
	
	foreach my $database (keys %{$data{"master"}{"databases"}}) {
		my $conn = $all_conn;

		$conn =~ s/DATABASE_NAME_HOLDER/$database/g;
		
		print F "$conn\ttry {
		echo 'Initializing the cluster';
		init cluster (id = 1, comment = 'Node 1');
	}
	on error {
		echo 'Could not initialize the cluster!';
		exit -1;
	}
	echo 'Database cluster initialized as Node 1';";

		foreach my $slave (sort keys %{$data{"slaves"}}) {
			print F "
	try {
		echo 'Storing node " . ($slave + 1) . "';
		store node (id = " . ($slave + 1) . ", comment = 'Node " . ($slave + 1) . "');
	}
	on error {
		echo 'Could not create node " . ($slave + 1) . "!';
		exit -1;
	}
	echo 'Node " . ($slave + 1) . " created';";
		}

		print F "
	try {
		echo 'Creating store paths';\n";
		foreach my $slave (keys %{$data{"slaves"}}) {
			my $conn = $data{"master"}{"coninfo"};
			$conn =~ s/DATABASE_NAME_HOLDER/$database/g;
			print F "\t\tstore path (server = 1, client = " . ($slave + 1) . ", conninfo = " . $conn . ");\n";
		}
		
		foreach my $slave (keys %{$data{"slaves"}}) {
			my $conn = $data{"slaves"}{$slave}{"coninfo"};
			$conn =~ s/DATABASE_NAME_HOLDER/$database/g;
			print F "\t\tstore path (server = " . ($slave + 1) . ", client = 1, conninfo = " . $conn . ");\n";
			foreach my $subslave (keys %{$data{"slaves"}}) {
				next if $slave == $subslave;
				print F "\t\tstore path (server = " . ($slave + 1) . ", client = " . ($subslave + 1) . ", conninfo = " . $data{"slaves"}{$slave}{"coninfo"} . ");\n";
			}
		}
		print F "\t}
	on error {
		echo 'Could not create store paths!';
		exit -1;
	}
	echo 'Store paths created';
	try {
		echo 'Storing listen network';
";

#<JanniCash> make it as I said. As long as you don't support cascading in your script,
#	let the master listen on all slaves for their events (origin=that_slave, provider=that_slave, receiver=master)
#<JanniCash> and let every slave listen for (origin=all_other_nodes, provider=master, receiver=slave)

		foreach my $slave (sort keys %{$data{"slaves"}}) {
			print F "\t\tstore listen (origin = 1, provider = 1, receiver = " . ($slave + 1) . ");\n";
		}

		foreach my $slave (sort keys %{$data{"slaves"}}) {
			print F "\t\tstore listen (origin = " . ($slave + 1) . ", provider = " . ($slave + 1) . ", receiver = 1);\n";
			foreach my $subslave (sort keys %{$data{"slaves"}}) {
				next if $slave == $subslave;
				print F "\t\tstore listen (origin = " . ($subslave + 1) . ", provider = 1, receiver = " . ($slave + 1) . ");\n";
			}
		}

		print F "\t}
	on error {
		echo 'Could not store listen network!';
		exit -1;
	}
	echo 'listen network stored';
	try {
		create set (id = 1, origin = 1, comment = '$database tables');
	}
	on error {
		echo 'Could not create subscription set!';
		exit -1;
	}
	echo 'Subscription set created';
	try {
		echo 'Adding tables to the subscription set';\n";

		my $count = 1;
		foreach my $table (keys %{$data{"master"}{"databases"}{$database}{"tables"}}) {
			print F "
		echo '  Adding table $table...';
		set add table (set id = 1, origin = 1, id = $count, full qualified name = '$table', comment = 'Table $table');
		echo '    done';\n";
			$count++;
		}

		$count = 1;
		
		print F "\n\t\techo 'Adding sequences to the subscription set';\n";
		
		foreach my $sequence (keys %{$data{"master"}{"databases"}{$database}{"sequences"}}) {
			print F "
		echo '  Adding sequence $sequence...';
		set add sequence (set id = 1, origin = 1, id = $count, full qualified name = '$sequence', comment = 'Sequence $sequence');
		echo '    done';\n";
			$count++;
		}

		print F "
	}
	on error {
		echo 'Could not add tables and sequences!';
		exit -1;
	}
	echo 'All tables added';
_EOF_

if [ \$? -ne 0 ]
then
	echo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.
	exit -1
else
";
		my $command = "slon T1 dbname=$database 2> /tmp/slon-$database.err 1> /tmp/slon-$database.out &\n";
		
		$slave_commands .= $command;
		
		print F "\t" . $command . "\techo slon has been started on the master and placed into the background. It is
	echo logging STDOUT to /tmp/slon-$database.out and STDERR to /tmp/slon-$database.err.
	echo
	echo Now start slon on all slaves by running the following command on all slaves as the
	echo slony system user:
	echo
	echo \"slon T1 dbname=$database 2> /tmp/slon-$database.err 1> /tmp/slon-$database.out &\"
	echo
	echo Once slon is running on all slaves, hit any key to proceed with the installation
	read -s -n1 a
	echo
fi

$conn
";
		
		print F "\ttry {\n";
		foreach my $slave (sort keys %{$data{"slaves"}}) {
			print F "\t\tsubscribe set (id = 1, provider = 1, receiver = " . ($slave + 1) . ", forward = no);\n";
		}
		print F "\t}
	on error {
		echo 'Could not subscribe the set to the slaves';
		exit -1;
	}
	echo 'Database $database subscribed to slaves';
_EOF_

if [ \$? -ne 0 ]
then
	echo Errors were detected. Please review /tmp/slony_setup.log and fix the errors.
	exit -1
fi\n";
		
	}
}

print F "
echo The installation has succeeded. At this time the slaves should be receiving the data
echo from the master.";

close F;

my $end = "
The setup script was saved as /tmp/slony_master_setup.sh.  This script must
be executed on the master as the slony system user.  If all goes well, slony
will be setup.  If not, errors should be reported in /tmp/slony_setup.log.

Additionally, a data dump of all data collected by this script has been stored
in the file /tmp/slony_dump. You might want to save this file if you want to
run this script again with many of the same values.  A subsequent run of
slony_setup.pl looks for slony_dump in /tmp.

You must also run the following command(s) on each slave as the slony system
user.  These commands should be also set to start and stop when postgres starts
and stops:

$slave_commands
Good luck!
";

&pager ($end);

#
# Backup all the collected data to a file for future use
#

&dump_to_file;

#
# Clean up
#

$exitcode = 0;
&clean_up;

#
# Functions
#

sub pager () {
	my $text = shift;
	my $pager = $ENV{"PAGER"} || 'less';
	open P, "| $pager" or &death ("Can't open pipe to $pager: $!");
	print P $text;
	close P;
}
	
sub check_version () {
	my $pgversion_major = 0;
	my $pgversion_minor = 0;
	$psql = `which psql`;
	chomp ($psql);
	if (-x $psql) {
		open P, "$psql --version |";
		while (<P>) {
			$_ =~ m/\s+(\d+)\.(\d+)((\w+)||(\.(\d+)))?\s+/;
			$pgversion_major = $1;
			$pgversion_minor = $2;
		}
		close P;
		if ($pgversion_major == 7 && $pgversion_minor >= 3) {
			return 0;
		} else {
			return 1
		}
	}
	return 2;
}

sub get_info () {

	my $which = shift;
	print "\n";
	
	print "Enter the hostname or IP address of the $which database (" . $data{$which}{"hostname"} . "): ";
	my $temp = <>;
	chomp ($temp);
	$data{$which}{"hostname"} = $temp if $temp;
	
	print "Enter the port address of the $which database (" . $data{$which}{"port"} . "): ";
	$temp = <>;
	chomp ($temp);
	$data{$which}{"port"} = $temp if $temp;
	
	print "Enter the username of the $which database (" . $data{$which}{"username"} . "): ";
	$temp = <>;
	chomp ($temp);
	$data{$which}{"username"} = $temp if $temp;
	
	system "stty -echo";
	if ($data{$which}{"password"}) {
		print "Enter the password (A password is cached. Hit enter to use it): ";
	} else {
		print "Enter the password: ";
	}
	$temp = <>;
	chomp ($temp);
	system "stty echo";
	$data{$which}{"password"} = $temp if $temp;
	
	my $pgpass =
		$data{$which}{"hostname"} . ":" .
		$data{$which}{"port"} . ":" .
		"*" . ":" .
		$data{$which}{"username"} . ":" .
		$data{$which}{"password"} . "\n";
	open F, ">>$pgpassfile" or &death ("Can't open file $pgpassfile: $!");
	print F $pgpass;
	close F;
	chmod 0600, $pgpassfile or &death ("Unable to chmod 600 $pgpassfile");
}

sub get_databases () {
	my $which = shift;
	open P, "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d template1" .
		" -t -l |"
		|| &death ("Can't open pipe to $psql: $!");
	while (<P>) {
		$_ =~ m/\s+(\S+)\s+\|\s+(\S+)\s+\|\s+(\S+)\s+/;
		next if ! $3;
		my $name = $1;
		my $owner = $2;
		my $encoding = $3;
		if ($name !~ m/^template(0|1)$/) {
			$data{$which}{"databases"}{$name}{"owner"} = $owner;
		}
	}
	close P;
}

sub get_tables () {
	my ($which, $database) = @_;
	open P, "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d $database " .
		" -t" .
		" -c " . qq("select "pg_catalog".quote_ident(schemaname), "pg_catalog".quote_ident(tablename), "pg_catalog".quote_ident(tableowner) from pg_tables where schemaname not in ('information_schema', 'pg_catalog')") . " |"
		|| &death ("Can't open pipe to $psql: $!");
	while (<P>) {
		$_ =~ m/\s+(\S+)\s+\|\s+(\S+)\s+\|\s+(\S+)\s+/;
		my $schema = $1;
		my $table = $2;
		my $owner = $3;
		$data{$which}{"databases"}{$database}{"tables"}{$schema . "." . $table} = 1;
	}
	close P;
}

sub get_sequences () {
	my ($which, $database) = @_;
	open P, "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d $database " .
		" -t" .
		" -c " . qq("select "pg_catalog".quote_ident(nspname) || '.' || "pg_catalog".quote_ident(relname) from pg_class c, pg_namespace n where c.relnamespace = n.oid and c.relkind = 'S'") . " |"
		|| &death ("Can't open pipe to $psql: $!");
	while (<P>) {
		$_ =~ m/\s+(\S+)\s+/;
		my $sequence_name = $1;
		$data{$which}{"databases"}{$database}{"sequences"}{$sequence_name} = 1;
	}
	close P;
}

sub get_users () {
	my $which = shift;
	open P, "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d template1 " .
		" -t" .
		" -c 'select * from pg_shadow' |"
		|| &death ("Can't open pipe to $psql: $!");
	while (<P>) {
		$_ =~ m/\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)$/;
		my $usename = $1;
		my $usesysid = $2;
		my $usecreatedb = $3;
		my $usesuper = $4;
		my $usecatupd = $5;
		my $passwd = $6;
		my $valuntil = $7;
		my $useconfig = $8;
		if ($usename && $usename !~ m/^(postgres|slony)$/) {
			$data{$which}{"users"}{$usesysid}{"usename"} = $usename;
			$data{$which}{"users"}{$usesysid}{"usesysid"} = $usesysid;
			$data{$which}{"users"}{$usesysid}{"usecreatedb"} = $usecreatedb;
			$data{$which}{"users"}{$usesysid}{"usesuper"} = $usesuper;
			$data{$which}{"users"}{$usesysid}{"usecatupd"} = $usecatupd;
			$data{$which}{"users"}{$usesysid}{"passwd"} = $passwd;
			$data{$which}{"users"}{$usesysid}{"valuntil"} = $valuntil;
			$data{$which}{"users"}{$usesysid}{"useconfig"} = $useconfig;
		}
	}
	close P;
}

sub get_groups () {
	my $which = shift;
	open P, "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d template1 " .
		" -t" .
		" -c 'select * from pg_group' |"
		|| &death ("Can't open pipe to $psql: $!");
	while (<P>) {
		$_ =~ m/\s+(\S*)\s+\|\s+(\S*)\s+\|\s+(\S*)$/;
		my $groname = $1;
		my $grosysid = $2;
		my $grolist = $3;
		if ($groname) {
			$data{$which}{"groups"}{$grosysid}{"groname"} = $groname;
			$data{$which}{"groups"}{$grosysid}{"grosysid"} = $grosysid;
			$data{$which}{"groups"}{$grosysid}{"grolist"} = $grolist;
		}
	}
	close P;
}

sub test_conn () {
	my $which = shift;
	my $ret = system "$psql -h " . $data{$which}{"hostname"} .
		" -p " . $data{$which}{"port"} .
		" -U " . $data{$which}{"username"} .
		" -d template1" .
		" -t" .
		" -c 'select current_timestamp' 1> $tmpdir/testpgconn 2> /dev/null";
	if ($ret != 0) {
		# Remove the failed attempt from ~/.pgpass
		open F, "$pgpassfile" || death ("Can't open $pgpassfile: $!");
		my @lines = <F>;
		close F;
		pop @lines;
		open F, ">$pgpassfile" || death ("Can't open $pgpassfile: $!");
		print F @lines;
		close F;
		return -1;
	}
  return 0;
}

sub get_one {
	my $temp;
	system "stty -echo";
	system "stty raw";
	sysread STDIN, $temp, 1;
	system "stty sane";
	system "stty echo";
	return $temp;
}

sub get_one_bool {
	my $temp = &get_one;
	if ($temp =~ m/n/i) {
		print "N\n";
		return 0;
	} else {
		print "Y\n";
		return 1;
	}
}

sub dump_to_file {
	return if ! $dumper;
	open F, ">$dump_file" or &death ("Can't open $dump_file: $!");
	foreach my $host (keys %data) {
		delete $data{$host}{"databases"};
		delete $data{$host}{"users"};
		delete $data{$host}{"groups"};
		delete $data{$host}{"coninfo"};
	}
	delete $data{"slaves"};
	print F Data::Dumper::Dumper (\%data);
	close F;
}

sub backup_file {
	my $orig = shift;
	my $backup = shift;
	open F, $orig or &death ("Count not open $orig: $!");
	open FH, ">$backup" or &death ("Count not open $backup: $!");
	while (<F>) {
		print FH $_;
	}
	close FH;
	close F;
	unlink $orig or &death ("Cannot unlink $orig: $!");
}

sub clean_up () {
	print "\n\nCleaning up\n";
	unlink $pgpassfile;
	if (-f $pgpassbackup) {
		print "Restoring previous $pgpassfile\n";
		&backup_file ($pgpassbackup, $pgpassfile);
	}
	if (-d $tmpdir) {
		print "Removing temporary directory\n";
		`rm -rf $tmpdir`;
	}
	print "\n";
	system "stty echo";
	exit $exitcode;
}

sub death {
	my $message = shift;
	print "$message\n";
	&clean_up;
}
