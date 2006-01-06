# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003-2006, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.9.2.1 2006-01-06 17:07:47 cbbrowne Exp $
# ----------

slony_subdir = src/ducttape
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

TESTS = $(wildcard test_[0-9A-F]_*)
DISTFILES = Makefile README $(TESTS)
ALL =


all:	$(ALL)


clean distclean maintainer-clean:
	rm -f $(ALL)

install: all installdirs


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir)

test:
	for script in $(TESTS) ; do \
	  ./$$script ; \
	done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

