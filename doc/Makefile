# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.5 2004-07-20 09:15:30 dfetter Exp $
# ----------

slony_subdir = doc
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = concept implementation

DISTFILES = Makefile

all install installdirs:

clean distclean maintainer-clean:
	for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $@ ; \
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
