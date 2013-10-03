# ----------
# Makefile for src/slon
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#
# ----------

slony_subdir = src/slon
slony_top_builddir = ../..
SLFILEDESC="Slony replication engine"
include $(slony_top_builddir)/Makefile.global
CPPFLAGS:=$(CPPFLAGS_CLIENT)
SFILES=$(wildcard *.c)
CC = $(PTHREAD_CC)

override CFLAGS += $(PTHREAD_CFLAGS) -I$(slony_top_builddir) -I$(slony_top_builddir)/$(slony_subdir)
override LDFLAGS += $(PTHREAD_LIBS) 

PROG		= slon

ifeq ($(PORTNAME), win)
PROG            = slon.exe
endif
ifeq ($(PORTNAME), win32)
PROG            = slon.exe
endif

OBJS		= \
    slon.o			\
    runtime_config.o	\
    local_listen.o		\
    remote_listen.o		\
    remote_worker.o		\
    sync_thread.o		\
    monitor_thread.o	\
    cleanup_thread.o	\
    scheduler.o		\
    dbutils.o		\
    conf-file.o		\
    confoptions.o		\
    misc.o                  \
    ../parsestatements/scanner.o

ifeq ($(PORTNAME), win32)
OBJS += port/pipe.o port/win32service.o $(WIN32RES)
override LDFLAGS += -lwsock32
endif


DISTFILES = Makefile README $(wildcard *.c) $(wildcard port/*.c) $(wildcard *.h) $(wildcard *.l) $(wildcard *.y)

ALL =				\
    $(PROG)


all:	$(ALL)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(PTHREAD_CFLAGS) $(LDFLAGS)

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
monitor_thread.o:	monitor_thread.c slon.h
conf-file.o:		conf-file.c slon.h confoptions.h
confoptions.o:		confoptions.c slon.h confoptions.h

conf-file.c:		conf-file.l
ifdef FLEX
	$(FLEX) $(FLEXFLAGS) -o'$@' $<
else
	@echo "Missing flex $< $@"
	@exit
endif

clean distclean:
	rm -f $(ALL) $(OBJS) $(PROG).core

maintainer-clean: clean
	rm -f conf-file.c

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(DESTDIR)$(slonbindir)


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(slonbindir)

splint:
	splint -I $(pgincludedir) -I $(pgincludeserverdir) +unixlib -preproc +skip-sys-headers $(SFILES)

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file || exit; \
	done
