#!/usr/bin/perl
# $Id: update_node.pl,v 1.1 2004-07-25 04:02:51 cbbrowne Exp $
# Author: Christopher Browne
# Copyright 2004 Afilias Canada

require 'slon-tools.pm';
require 'slon.env';

open(SLONIK, "|slonik");
print SLONIK qq{
        cluster name = $SETNAME ;
        node 1 admin conninfo = '$CINFO1';
        node 2 admin conninfo = '$CINFO2';

	update functions (id = 1);
	update functions (id = 2);
};
