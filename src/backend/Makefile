# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.12 2004-07-01 02:24:35 wieck Exp $
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global
CFLAGS += -I$(slony_top_builddir)
include $(PGSOURCETREE)/src/Makefile.shlib


FUNCS_NAME		= slony1_funcs

FUNCS_SONAME	= $(FUNCS_NAME)$(DLSUFFIX)
FUNCS_SOOBJS	= $(FUNCS_NAME).o

BASE_COMMON		= slony1_base.sql
FUNCS_COMMON	= slony1_funcs.sql

BASE_73			= slony1_base.v73.sql
FUNCS_73		= slony1_funcs.v73.sql

BASE_74			= slony1_base.v74.sql
FUNCS_74		= slony1_funcs.v74.sql

SQL_NAMES =				\
	$(BASE_COMMON)		\
	$(FUNCS_COMMON)		\
	$(BASE_73)			\
	$(FUNCS_73)			\
	$(BASE_74)			\
	$(FUNCS_74)

DISTFILES = Makefile README README.events $(wildcard *.sql) $(wildcard *.in) $(wildcard *.c)

ALL = $(SQL_NAMES)		\
	$(FUNCS_SONAME)


all:	$(ALL)

$(FUNCS_SONAME):	$(FUNCS_SOOBJS)

clean distclean maintainer-clean:
	rm -f $(FUNCS_SONAME) $(FUNCS_SOOBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(FUNCS_SONAME) $(DESTDIR)$(pkglibdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(DESTDIR)$(datadir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	$(mkinstalldirs) $(DESTDIR)$(datadir)



distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

