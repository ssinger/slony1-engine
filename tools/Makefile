# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.2 2004-11-25 21:57:18 cbbrowne Exp $
# ----------

slony_subdir = tools
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = altperl

DISTFILES = Makefile slony_setup.pl generate_syncs.sh

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
