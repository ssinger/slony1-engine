# ----------
# Makefile for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: Makefile,v 1.5 2004-02-20 15:13:27 wieck Exp $
# ----------

top_builddir = .
include $(top_builddir)/Makefile.global

all install installdirs clean:
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

distclean: clean
	rm -f Makefile.global Makefile.port
	rm -f config.log config.status config.h

maintainer-clean: distclean
	rm -rf configure autom4te.cache
