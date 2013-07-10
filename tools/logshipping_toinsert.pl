#!/usr/bin/perl
##
# converts a Slony 2.2 (or later) log shipping file that uses
# the COPY command to populate sl_log_archive into a SQL
# file that uses INSERT statements to insert directly into
# the data tables.
##


use strict;

sub print_insert($$);

my $state='START';
my $in_file = $ARGV[0];
my @copy_columns;

if (!defined($in_file))
{
	print STDERR "usage:logshipping_to_insert.pl filename\n";
	exit -1;
}

open(INFILE, "<$in_file") or die 
	"error opening $in_file";

###
# read/write the file until the first
# COPY (...) sl_log_archive.
##
while(<INFILE>) {
	my $line = $_;
	if ($line=~m/^COPY .*\."sl_log_archive" \((.*)\)/) {
		#found copy.
		####
		# Store the column order
		#
		##
		foreach my $col (split(',',$1)) {
			$col =~s/^[[:space:]]*//g;
			$col=~s/[[:space:]]*$//g;
			push(@copy_columns, $col);
		}
		$state='IN_COPY';
		last;
	}
	printf $line;
}



##
# for each line parse COPY blocks.
#
##
#print $state;
if ($state eq 'IN_COPY') {
	while(<INFILE>) {
		chomp $_;
		my $line = $_;
		if ($line eq '\\.') {
			$state='POST_COPY';
			last;
		}
		#print "$line\n";
		my $idx=0;
		my %fields;
		foreach my $col (split('\t',$line)) {
			$fields{$copy_columns[$idx]}=$col;
			$idx++;
		}
		my @data=split(//,$fields{'log_cmdargs'});
		my @parsed;
		my $in_quote=0;
		my $last_start=1;
		my $idx;
		for($idx=2; $idx < @data; $idx++){
			if($data[$idx] eq '"' && 
				$data[$idx-1] ne '\\' ) 
			{
				if($in_quote>0)
				{
					push(@parsed,join('',@data[$in_quote+1..
												 $idx-1]));
					$in_quote=0;
				}
				else
				{
					$in_quote=$idx;
				}
				$idx++;
				$last_start=$idx+1;
				next;
			}
			elsif ($data[$idx] eq ',' && $in_quote==0) {
					push(@parsed,join('',@data[$last_start..
												   $idx-1]));
					$last_start=$idx+1;
					next;
			}
		}
		push(@parsed,join('',@data[$last_start..
								   $idx-2]));
		#translate into an INSERT/UPDATE/DELETE/TRUNCATE
		if ($fields{'log_cmdtype'} eq 'I') {			
			print_insert(\%fields,\@parsed);
		}
		elsif ($fields{'log_cmdtype'} eq 'U') {
			print_update(\%fields,\@parsed);
		}
		elsif ($fields{'log_cmdtype'} eq 'D') {
			print_delete(\%fields,\@parsed);
		}
		elsif ($fields{'log_cmdtype'} eq 'T') {
			print_truncate(\%fields,\@parsed);
		}
		elsif ($fields{'log_cmdtype'} eq 'S') {
			print @data[2..scalar(@data)-3];
			print "\n";
		}

		
	}
}

###
# at \. stop and copy the rest of the file.
# 
#
while(<INFILE>) {
	print $_;
}


sub print_insert($$)
{
	my $fields=shift;
	my $parsed=shift;
	
	my $query="INSERT INTO \"" . $fields->{'log_tablenspname'} . '"."' .
		$fields->{'log_tablerelname'} . '" (';
	my $first=0;
	for(my $idx=0; $idx < @{$parsed}; $idx=$idx+2) {
		if($first == 1)
		{
			$query.= ',';
		}
		$query.= '"' . $parsed->[$idx] . '"';
		$first=1;
	}
	$query.=') VALUES (';
	$first=0;
	for(my $idx=1; $idx < @{$parsed}; $idx=$idx+2) {
		if($first == 1)
		{
			$query.=',';
		}
		$query.= "'" . $parsed->[$idx] . "'";
		$first=1;
	}
	$query.= ');';
	print "$query\n";
}


sub print_update($$)
{
	my $fields=shift;
	my $parsed=shift;
	
	my $query="UPDATE \"" . $fields->{'log_tablenspname'} . '"."' .
		$fields->{'log_tablerelname'} . '" SET ';
	my $first=0;
	my $idx;
	for($idx=0; $idx < $fields->{'log_cmdupdncols'}*2; $idx=$idx+2) {
		if($first == 1)
		{
			$query.= ',';
		}
		$query.= '"' . $parsed->[$idx] . '"=\'' . $parsed->[$idx+1] . "'";
		$first=1;
	}
	$query.=' WHERE ';
	$first=0;
	for(;$idx < @{$parsed}; $idx=$idx+2) {
		if($first == 1)
		{
			$query.=' AND ';
		}
		$query.= '"' . $parsed->[$idx] . '"=\''. $parsed->[$idx+1] . "'" ;
		$first=1;
	}
	$query.= ';';
	print "$query\n";
}


sub print_delete($$)
{
	my $fields=shift;
	my $parsed=shift;
	
	my $query="DELETE FROM \"" . $fields->{'log_tablenspname'} . '"."' .
		$fields->{'log_tablerelname'} . '" WHERE ';
	my $first=0;
	my $idx=0;
	$first=0;
	for(;$idx < @{$parsed}; $idx=$idx+2) {
		if($first == 1)
		{
			$query.=' AND ';
		}
		$query.= '"' . $parsed->[$idx] . '"=\''. $parsed->[$idx+1] . "'" ;
		$first=1;
	}
	$query.= ';';
	print "$query\n";
}


sub print_truncate($$)
{
	my $fields=shift;
	my $parsed=shift;
	
	my $query="TRUNCATE ONLY \"" . $fields->{'log_tablenspname'} . '"."' .
		$fields->{'log_tablerelname'} . '" CASCADE; ';
	print "$query\n";
}
