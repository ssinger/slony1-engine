# ----------
# Makefile for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: Makefile,v 1.6 2004-03-02 15:07:45 wieck Exp $
# ----------

top_builddir = .
include $(top_builddir)/Makefile.global

all install installdirs clean:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

distclean: clean
	rm -f Makefile.global Makefile.port
	rm -f src/slon/Makefile
	rm -f config.log config.status config.h

maintainer-clean: distclean
	rm -rf configure autom4te.cache
