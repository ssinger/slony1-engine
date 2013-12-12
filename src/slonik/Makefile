# ----------
# Makefile for src/slonik
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# ----------

slony_subdir = src/slonik
slony_top_builddir = ../..
SLFILEDESC="Slony command interpreter"
include $(slony_top_builddir)/Makefile.global
ifeq ($(HAVE_PGPORT),1)
CPPFLAGS:=$(CPPFLAGS_SERVER) $(CPPFLAGS_CLIENT)
else
CPPFLAGS:=$(CPPFLAGS_CLIENT)
endif
ifeq ($(PORTNAME), aix)
  CFLAGS += -D_LARGE_FILES
endif

CFLAGS += -I$(slony_top_builddir) -DPGSHARE="\"$(pgsharedir)\"" 

PROG		= slonik

ifeq ($(PORTNAME), win)
PROG            = slonik.exe
LDFLAGS += -lpgport
endif
ifeq ($(PORTNAME), win32)
PROG            = slonik.exe
LDFLAGS += -lpgport
else ifdef HAVE_PGPORT
LDFLAGS += -lpgport
ifdef HAVE_PGCOMMON
LDFLAGS += -lpgcommon
endif
endif

OBJS		= 			\
	slonik.o			\
	dbutil.o			\
	parser.o $(WIN32RES)            \
	../parsestatements/scanner.o    \
	scan.o
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

scan.c: SCANNER_HEADER=scan.h

scan.c:				scan.l slonik.h
ifdef FLEX 
	$(FLEX) $(FLEXFLAGS) --header-file=$(SCANNER_HEADER) -o'$@' $<
else
	@echo "Missing flex $< $@"
	@exit
endif

splint:
	splint -I $(pgincludedir) -I $(pgincludeserverdir) +unixlib -preproc +skip-sys-headers $(wildcard *.c)

clean distclean:
	rm -f $(ALL) $(OBJS) $(PROG).core

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
