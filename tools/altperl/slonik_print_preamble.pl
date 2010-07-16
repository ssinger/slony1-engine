#!@@PERL@@
# 
# Author: Mark Stosberg
# Based on work by: Christopher Browne
# Parts Copyright 2006 Summerault, LLC
# Parts Copyright 2004-2009 Afilias Canada

use Getopt::Long;

$CONFIG_FILE = '@@SYSCONFDIR@@/slon_tools.conf';
$SHOW_USAGE  = 0;

# Read command-line options
GetOptions("config=s" => \$CONFIG_FILE,
	   "help"     => \$SHOW_USAGE);

my $USAGE =
"Usage: slonik_print_preamble [--config file]

    print the preamble required by all slonik scripts, using the
    details from the config file

  Write the results of this command to a file, and then use a text
  editor to complete a custom command to send to slonik.
";

if ($SHOW_USAGE) {
    print $USAGE;
    exit 0;
}

require '@@PERLSHAREDIR@@/slon-tools.pm';
require $CONFIG_FILE;

my $slonik = '';
$slonik .= genheader();

run_slonik_script($slonik, 'PRINT PREAMBLE');
