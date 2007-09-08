# ----------
# Makefile for src/slony_logshipper
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.1.2.1 2007-09-08 14:21:40 wieck Exp $
# ----------

slony_subdir = src/slony_logshipper
slony_top_builddir = ../..
SLFILEDESC="Slony command interpreter"
include $(slony_top_builddir)/Makefile.global

ifeq ($(PORTNAME), aix)
  CFLAGS += -D_LARGE_FILES
endif

CFLAGS += -I$(slony_top_builddir) -DPGSHARE="\"$(pgsharedir)\"" 


PROG		= slony_logshipper

ifeq ($(PORTNAME), win)
PROG            = slony_logshipper.exe
LDFLAG		= $(LDFLAG) -lpgport
endif
ifeq ($(PORTNAME), win32)
PROG            = slony_logshipper.exe
LDFLAG		= $(LDFLAG) -lpgport
endif

OBJS		= 					\
	slony_logshipper.o			\
	dbutil.o					\
	ipcutil.o					\
	parser.o $(WIN32RES)		\
	../parsestatements/scanner.o

DISTFILES = Makefile $(wildcard *.c) $(wildcard *.h) $(wildcard *.l) $(wildcard *.y)

ALL =					\
	$(PROG)


all:	$(ALL)

$(PROG):	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)
slony_logshipper.o:			slony_logshipper.c slony_logshipper.h
dbutil.o:			dbutil.c slony_logshipper.h
parser.o:			parser.c scan.c

parser.c:			parser.y slony_logshipper.h
ifdef YACC
	$(YACC) -d $(YFLAGS) $<
	mv -f y.tab.c parser.c
else
	@echo "Missing yacc $< $@"
	@exit 1
endif

scan.c:				scan.l slony_logshipper.h
ifdef FLEX 
	$(FLEX) $(FLEXFLAGS) -o'$@' $<
else
	@echo "Missing flex $< $@"
	@exit
endif

clean distclean:
	rm -f $(ALL) $(OBJS) $(PROG).core
	rm -f parser.c scan.c y.tab.h

maintainer-clean:	clean
	rm -f parser.c scan.c y.tab.h

install: all installdirs
	$(INSTALL_SCRIPT) $(PROG) $(DESTDIR)$(slonbindir)


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(slonbindir)


distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done
