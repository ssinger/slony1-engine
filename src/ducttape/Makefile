# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.7 2004-07-01 02:24:35 wieck Exp $
# ----------

subdir = src/ducttape
top_builddir = ../..
include $(top_builddir)/Makefile.global

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

