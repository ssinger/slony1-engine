# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.3 2004-02-20 15:13:27 wieck Exp $
# ----------

subdir = src
top_builddir = ..
include $(top_builddir)/Makefile.global

all install installdirs:

clean distclean maintainer-clean:
	$(MAKE) -C concept $@
	$(MAKE) -C implementation $@
