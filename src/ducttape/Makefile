# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.10 2005-07-04 23:45:21 darcyb Exp $
# ----------

slony_subdir = src/ducttape
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

TESTSSRC = `ls test_[0-9A-F]_*.in`
TESTS = `ls test_[0-9A-F]_*.in | sed -e 's/\.in//'`
DISTFILES = Makefile README $(TESTSSRC)
ALL =


all:	$(ALL)
	@for script in $(TESTS); do \
		if test x"$(host_os)" = "xmingw32"; then \
			sed -e 's/TERMPROG=xterm/TERMPROG=rxvt/' -e 's/WHICHPROG=which/WHICHPROG=\/bin\/which/' $$script.in > $$script; \
		else \
			cp $$script.in $$script; \
		fi; \
	done
	

clean distclean maintainer-clean:
	@for script in $(TESTS); do \
		if [ -f $$script ]; then \
			rm -f $$script; \
		fi; \
	done;\

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

