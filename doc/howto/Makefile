# ----------
# Makefile for the HOWTOs
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#
# ----------

slony_subdir = doc/howto
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile $(wildcard *.txt*) $(wildcard *.html*)

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

clean:

