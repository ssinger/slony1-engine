# ----------
# Makefile for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: Makefile,v 1.7 2004-03-18 19:32:21 wieck Exp $
# ----------

top_builddir = .
include $(top_builddir)/Makefile.global
TAR = tar
GZIP = gzip --best

DISTFILES = aclocal.m4 \
  config.h.in \
  configure \
  configure.ac \
  COPYRIGHT \
  Makefile \
  Makefile.global.in \
  postgresql-slony1-engine.spec \
  postgresql-slony1-engine.spec.in \
  $(wildcard config/*) \
  $(wildcard makefiles/*)


SUBDIRS = src doc

all install installdirs clean:
	for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir $@ ; \
    done

distclean: clean
	rm -f Makefile.global Makefile.port
	rm -f src/slon/Makefile
	rm -f src/slonik/Makefile
	rm -f config.log config.status config.h
	rm -f postgresql-$(PACKAGE).spec
	rm -f $(PACKAGE)-$(VERSION).tar.gz
	rm -rf autom4te.cache

maintainer-clean: distclean
	rm -rf configure autom4te.cache

dist: distdir
	-chmod -R a+r $(distdir)
	$(TAR) chof - $(distdir) | $(GZIP) -c > $(distdir).tar.gz
	-rm -rf $(distdir)

distdir: $(DISTFILES)
	-rm -rf $(distdir)
	mkdir $(distdir)
	mkdir $(distdir)/config
	mkdir $(distdir)/makefiles
	-chmod -R 777 $(distdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$$file ; \
    done
	for subdir in $(SUBDIRS) ; do \
	  $(MAKE) -C $$subdir distdir ; \
    done

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
