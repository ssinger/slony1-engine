# ----------
# Makefile for src
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.9 2004-07-13 18:51:36 xfade Exp $
# ----------

slony_subdir = src
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile
SUBDIRS = xxid slon slonik backend ducttape

all install installdirs clean distclean maintainer-clean:
	for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir $@ || exit; \
    done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done
	for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir distdir ; \
    done
