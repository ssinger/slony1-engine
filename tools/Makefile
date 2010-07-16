# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = tools
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = altperl

DISTFILES = Makefile slony_setup.pl generate_syncs.sh slony1_extract_schema.sh

all install installdirs clean distclean maintainer-clean:
	for subdir in $(SUBDIRS) ; do \
          $(MAKE) -C $$subdir $@ || exit; \
        done


distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file || exit; \
	done
	for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir distdir || exit; \
	done
