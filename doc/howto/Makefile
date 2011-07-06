# ----------
# Makefile for the HOWTOs
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#
# ----------

slony_subdir = doc/howto
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile $(wildcard *.txt*) $(wildcard *.html*)

distdir: $(DISTFILES)
	mkdir -p $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done

all:

installdirs:
ifdef docdir
	$(mkinstalldirs) $(DESTDIR)$(docdir)/howto
endif

install: all installdirs
ifdef docdir
	for file in $(wildcard *.txt) $(wildcard *.html ); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/howto || exit; \
	done
endif

clean distclean:
