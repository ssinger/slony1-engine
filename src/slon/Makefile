# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.3 2003-12-13 17:02:03 wieck Exp $
# ----------

subdir = src/slon
top_builddir = ../..
include $(top_builddir)/Makefile.global

CFLAGS += -g
LDFLAGS += -lpthread

PROG		= slon

OBJS		= 		\
	slon.o			\
	local_node.o	\
	remote_node.o	\
	scheduler.o

ALL =				\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)

slon.o:		slon.c slon.h

local_node.o:	local_node.c slon.h

remote_node.o:	remote_node.c slon.h

scheduler.o:	scheduler.c slon.h

clean distclean maintainer-clean:
	rm -f $(ALL) $(OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(bindir)


installdirs:
	$(mkinstalldirs) $(bindir)


