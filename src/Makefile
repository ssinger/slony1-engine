# ----------
# Makefile for src
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.5 2004-02-20 15:13:28 wieck Exp $
# ----------

subdir = src
top_builddir = ..
include $(top_builddir)/Makefile.global

all install installdirs clean distclean maintainer-clean:
	$(MAKE) -C xxid $@
	$(MAKE) -C slon $@
	$(MAKE) -C backend $@
	$(MAKE) -C ducttape $@
