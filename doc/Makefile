# ----------
# Makefile for doc
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.2 2004-01-17 17:30:19 wieck Exp $
# ----------

subdir = src
top_builddir = ..
include $(top_builddir)/Makefile.global

all install installdirs:

clean distclean maintainer-clean:
	$(MAKE) -C concept $@
	$(MAKE) -C implementation $@
