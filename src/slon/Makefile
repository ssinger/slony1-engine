# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.2 2003-12-03 19:29:53 wieck Exp $
# ----------

subdir = src/slon
top_builddir = ../..
include $(top_builddir)/Makefile.global

PROG		= slon

OBJS		= slon.o

ALL =				\
	$(PROG)


all:	$(ALL)

slon.o:		slon.c slon.h

clean distclean maintainer-clean:
	rm -f $(ALL) $(OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(bindir)


installdirs:
	$(mkinstalldirs) $(bindir)


