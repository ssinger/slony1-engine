# ----------
# Makefile for Slony-I
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: Makefile,v 1.3 2003-11-18 15:11:35 wieck Exp $
# ----------

top_builddir = .
include $(top_builddir)/Makefile.global

all install installdirs clean:
	$(MAKE) -C src $@

distclean: clean
	rm -f Makefile.global Makefile.port
	rm -f config.log config.status config.h

maintainer-clean: distclean
	rm -rf configure autom4te.cache
