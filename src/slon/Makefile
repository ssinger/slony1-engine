# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.12 2004-03-31 17:25:17 wieck Exp $
# ----------

slony_subdir = src/slon
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

CC = $(PTHREAD_CC)
CFLAGS += $(PTHREAD_CFLAGS) -I$(slony_top_builddir) -I$(PGSOURCETREE)/src/interfaces/libpq
LDFLAGS += $(PTHREAD_CFLAGS) $(rpath) $(PTHREAD_LIBS) -L$(PGSOURCETREE)/src/interfaces/libpq -lpq

PROG		= slon

OBJS		= 			\
	slon.o				\
	runtime_config.o	\
	local_listen.o		\
	remote_listen.o		\
	remote_worker.o		\
	sync_thread.o		\
	cleanup_thread.o	\
	scheduler.o			\
	dbutils.o			\
	misc.o

DISTFILES = Makefile.in README $(wildcard *.c) $(wildcard *.h)

ALL =				\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)

cleanup_thread.o:	cleanup_thread.c slon.h
dbutils.o:			dbutils.c slon.h
local_listen.o:		local_listen.c slon.h
misc.o:				misc.c slon.h
remote_listen.o:	remote_listen.c slon.h
remote_worker.o:	remote_worker.c slon.h
runtime_config.o:	runtime_config.c slon.h
scheduler.o:		scheduler.c slon.h
slon.o:				slon.c slon.h
sync_thread.o:		sync_thread.c slon.h

clean distclean maintainer-clean:
	rm -f $(ALL) $(OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(bindir)


installdirs:
	$(mkinstalldirs) $(bindir)



distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

