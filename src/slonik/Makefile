# ----------
# Makefile for src/slonik
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.13 2004-12-11 00:08:35 darcyb Exp $
# ----------

slony_subdir = src/slonik
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

ifeq ($(PORTNAME), aix)
  CFLAGS += -D_LARGE_FILES
endif

override CFLAGS += -I$(slony_top_builddir) -I$(pgincludedir) -I$(pgincludeserverdir) -DPGSHARE="\"$(pgsharedir)\""
override LDFLAGS += $(rpath) -lpq

PROG		= slonik

ifeq ($(PORTNAME), win)
PROG            = slonik.exe
endif

OBJS		= 			\
	slonik.o			\
	dbutil.o			\
	parser.o

DISTFILES = Makefile $(wildcard *.c) $(wildcard *.h) $(wildcard *.l) $(wildcard *.y)

ALL =					\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)
slonik.o:			slonik.c slonik.h
dbutil.o:			dbutil.c slonik.h
parser.o:			parser.c scan.c

parser.c:			parser.y slonik.h
ifdef YACC
	$(YACC) -d $(YFLAGS) $<
	mv -f y.tab.c parser.c
else
	@echo "Missing yacc $< $@"
	@exit 1
endif

scan.c:				scan.l slonik.h
ifdef FLEX
	$(FLEX) $(FLEXFLAGS) -o'$@' $<
else
	@echo "Missing flex $< $@"
	@exit
endif

clean distclean:
	rm -f $(ALL) $(OBJS) $(PROG).core

maintainer-clean:	clean
	rm -f parser.c scan.c y.tab.h

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(DESTDIR)$(pgbindir)


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(pgbindir)


distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

