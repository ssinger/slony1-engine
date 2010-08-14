# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = src/ducttape
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

TESTSSRC = `ls test_[0-9A-Z]_*.in`
TESTS = `ls test_[0-9A-Z]_*.in | sed -e 's/\.in//'`
DISTFILES = Makefile README $(TESTSSRC)
ALL =


all:	$(ALL)
	@for script in $(TESTS); do \
		if test x"$(host_os)" = "xmingw32"; then \
			sed -e 's/TERMPROG=xterm/TERMPROG=rxvt/' -e 's/WHICHPROG=which/WHICHPROG=\/bin\/which/' $$script.in > $$script; \
		else \
			cp $$script.in $$script; \
			chmod ugo+x $$script; \
		fi; \
	done

clean distclean maintainer-clean:
	@for script in $(TESTS); do \
		if [ -f $$script ]; then \
			rm -f $$script; \
		fi; \
	done;\
	rm -rf dump.tmp.*
	rm -rf offline_dump.sql offline_logs

install: all installdirs


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir)

test:
	for script in $(TESTS) ; do \
	  ./$$script || exit; \
	done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done

