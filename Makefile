# Makefile for Slony-I
# Author: Jan Wieck, Afilias USA INC.
#
#	$Header: /local/home/ssinger/cvs2svn/cvs2svn-2.3.0/slony-cvsd/slony1-engine/Makefile,v 1.2 2003-11-17 17:33:11 wieck Exp $
#

top_builddir = .
include $(top_builddir)/Makefile.global

all install installdirs clean:
	$(MAKE) -C src $@

distclean: clean
	rm -f Makefile.global Makefile.port
	rm -f config.log config.status config.h

maintainer-clean: distclean
	rm -rf configure autom4te.cache
