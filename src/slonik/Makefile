# ----------
# Makefile for src/slonik
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.7.2.2 2004-10-19 18:17:37 wieck Exp $
# ----------

slony_subdir = src/slonik
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

CFLAGS += -I$(slony_top_builddir) -I$(PGSOURCETREE)/src/interfaces/libpq -DPGSHARE="\"$(datadir)\""
override LDFLAGS += $(rpath) -L$(PGSOURCETREE)/src/interfaces/libpq -lpq
ifeq ($(PORTNAME), aix)
  CFLAGS += -D_LARGE_FILES
endif


PROG		= slonik

OBJS		= 			\
	slonik.o			\
	dbutil.o			\
	parser.o

DISTFILES = Makefile.in $(wildcard *.c) $(wildcard *.h) $(wildcard *.l) $(wildcard *.y)

ALL =					\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)
slonik.o:			slonik.c slonik.h
dbutil.o:			dbutil.c slonik.h
parser.o:			parser.c scan.c

parser.c:	parser.y slonik.h
ifdef YACC
	$(YACC) -d $(YFLAGS) $<
	mv -f y.tab.c parser.c
else
	@$(missing) yacc $< $@
endif

scan.c:		scan.l slonik.h
ifdef FLEX
	$(FLEX) $(FLEXFLAGS) -o'$@' $<
else
	@$(missing) flex $< $@
endif


clean distclean:
	rm -f $(ALL) $(OBJS)

maintainer-clean:	clean
	rm -f parser.c scan.c

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(DESTDIR)$(bindir)


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir)


distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

