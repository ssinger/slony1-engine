# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.8 2004-09-02 18:08:00 darcyb Exp $
# ----------

slony_subdir = doc
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = concept implementation howto

DISTFILES = Makefile

all install installdirs:

clean distclean maintainer-clean:
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $@ ; \
	done


distdir: $(DISTFILES)
	mkdir -p $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	@for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file ; \
	done
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir distdir ; \
	done
