# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.5 2004-02-20 15:13:28 wieck Exp $
# ----------

subdir = src/slon
top_builddir = ../..
include $(top_builddir)/Makefile.global

CFLAGS += -g
LDFLAGS += -lpthread

PROG		= slon

OBJS		= 			\
	slon.o				\
	runtime_config.o	\
	sync_thread.o		\
	cleanup_thread.o	\
	scheduler.o			\
	dbutils.o			\
	misc.o

ALL =				\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)

slon.o:		slon.c slon.h

sync_thread.o:	sync_thread.c slon.h

cleanup_thread.o:	cleanup_thread.c slon.h

scheduler.o:	scheduler.c slon.h

dbutils.o:	dbutils.c slon.h

misc.o:	misc.c slon.h

clean distclean maintainer-clean:
	rm -f $(ALL) $(OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(bindir)


installdirs:
	$(mkinstalldirs) $(bindir)


