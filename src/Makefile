# ----------
# Makefile for src
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.4 2003-11-28 15:36:33 wieck Exp $
# ----------

subdir = src
top_builddir = ..
include $(top_builddir)/Makefile.global

all install installdirs clean distclean maintainer-clean:
	$(MAKE) -C xxid $@
	$(MAKE) -C slon $@
	$(MAKE) -C backend $@
	$(MAKE) -C ducttape $@
