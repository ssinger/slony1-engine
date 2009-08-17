# ----------
# Makefile for src
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.13.2.3 2009-08-17 17:39:57 devrim Exp $
# ----------

slony_subdir = src
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile
SUBDIRS = xxid parsestatements slon slonik backend ducttape
ifeq ($(PORTNAME),win32)
SUBDIRS += slevent
else
SUBDIRS += slony_logshipper
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
