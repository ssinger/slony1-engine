# ----------
# Makefile for src/slonik
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.8 2004-07-12 18:53:27 xfade Exp $
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
parser.c:			parser.y slonik.h
scan.c:				scan.l slonik.h

clean distclean:
	rm -f $(ALL) $(OBJS)

maintainer-clean:	clean
	rm -f parser.c scan.c

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

