# ----------
# Makefile for src/slonik
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.1 2004-03-10 21:27:32 wieck Exp $
# ----------

subdir = src/slon
top_builddir = ../..
include $(top_builddir)/Makefile.global

PROG		= slonik

OBJS		= 			\
	slonik.o			\
	dbutil.o			\
	parser.o

ALL =					\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)

slonik.o:			slonik.c slonik.h
dbutil.o:			dbutil.c slonik.h
parser.o:			parser.c scan.c
parser.c:			parser.y slonik.h
scan.c:				scan.l slonik.h

clean distclean maintainer-clean:
	rm -f $(ALL) $(OBJS)
	rm -f parser.c scan.c

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(bindir)


installdirs:
	$(mkinstalldirs) $(bindir)


