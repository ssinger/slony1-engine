# ----------
# Makefile for src
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.14 2007-09-09 02:37:04 wieck Exp $
# ----------

slony_subdir = src
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile
SUBDIRS = xxid parsestatements slon slonik slony_logshipper backend ducttape
ifeq ($(PORTNAME),win32)
SUBDIRS += slevent
endif

all install installdirs clean distclean maintainer-clean:
	@for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir $@ || exit; \
    done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	@for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done
	@for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir distdir || exit; \
    done
