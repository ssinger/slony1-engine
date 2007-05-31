# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.25 2007-05-31 16:46:18 wieck Exp $
# ----------

slony_subdir = src/backend
slony_top_builddir = ../..
SLFILEDESC="Slony replication functions"
SO_NAME = $(NAME)$(DLSUFFIX)
include $(slony_top_builddir)/Makefile.global

override CFLAGS += -I$(slony_top_builddir) $(CFLAGS_SL)

NAME		= slony1_funcs

SO_OBJS	= $(NAME).o $(WIN32RES)

BASE_COMMON		= slony1_base.sql
FUNCS_COMMON	= slony1_funcs.sql

BASE_83			= slony1_base.v83.sql
FUNCS_83		= slony1_funcs.v83.sql

SQL_NAMES =				\
	$(BASE_COMMON)		\
	$(FUNCS_COMMON)		\
	$(BASE_83)			\
	$(FUNCS_83)

DISTFILES = Makefile README README.events $(wildcard *.sql) $(wildcard *.in) $(wildcard *.c)

ALL = $(SQL_NAMES)		\
	$(SO_NAME)


all:	$(ALL)

$(SO_NAME):	$(SO_OBJS)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)

splint:
	splint -I $(pgincludedir) -I $(pgincludeserverdir) +unixlib -preproc +skip-sys-headers $(wildcard *.c)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(DESTDIR)$(pgpkglibdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(pgsharedir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(pgpkglibdir)
	$(mkinstalldirs) $(DESTDIR)$(pgsharedir)



distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done

