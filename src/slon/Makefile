# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.25 2004-12-11 00:08:34 darcyb Exp $
# ----------

slony_subdir = src/slon
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

CC = $(PTHREAD_CC)

override CFLAGS +=  $(PTHREAD_CFLAGS) -I$(slony_top_builddir) -I$(pgincludedir)
override LDFLAGS += $(rpath) $(PTHREAD_LIBS) -L$(pglibdir) -lpq

PROG		= slon

ifeq ($(PORTNAME), win)
PROG            = slon.exe
endif

OBJS		= 			\
	slon.o				\
	runtime_config.o	\
	local_listen.o		\
	remote_listen.o		\
	remote_worker.o		\
	sync_thread.o		\
	cleanup_thread.o	\
	stats_thread.o		\
	scheduler.o			\
	dbutils.o			\
	conf-file.o	\
	confoptions.o	\
	misc.o

DISTFILES = Makefile README $(wildcard *.c) $(wildcard *.h)

ALL =				\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(PTHREAD_LDFLAGS) $(LDFLAGS) -o $(PROG)

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
conf-file.o:		conf-file.c slon.h confoptions.h
confoptions.o:		confoptions.c slon.h confoptions.h

conf-file.c:		conf-file.l
	$(FLEX) -o$(@) $<

clean distclean:
	rm -f $(ALL) $(OBJS) $(PROG).core

maintainer-clean: clean
	rm -f conf-file.c

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

