# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.8 2004-07-20 09:15:31 dfetter Exp $
# ----------

slony_subdir = src/ducttape
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile README test_1_pgbench test_2_extra_1 test_2_extra_2 test_2_pgbench test_3_pgbench

ALL =


all:	$(ALL)


clean distclean maintainer-clean:
	rm -f $(ALL)

install: all installdirs


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir)



distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

