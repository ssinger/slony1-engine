# ----------
# Makefile for the HOWTOs
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#       $Id: Makefile,v 1.1 2004-07-26 19:05:24 xfade Exp $
#
# ----------

slony_subdir = tools/altperl
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile ToDo README $(wildcard *.pl*) $(wildcard *.pm*) $(wildcard *.env*)

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

clean distclean maintainer-clean:

