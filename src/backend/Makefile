# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.1 2003-11-28 14:59:44 wieck Exp $
# ----------

subdir = src/backend
top_builddir = ../..
include $(top_builddir)/Makefile.global

FUNCS_NAME		= slony1_funcs

FUNCS_SONAME	= $(FUNCS_NAME)$(DLSUFFIX)
FUNCS_SOOBJS	= $(FUNCS_NAME).o

BASE_COMMON		= slony1_base.sql
FUNCS_COMMON	= slony1_funcs.sql

BASE_73			= slony1_base.v73.sql
FUNCS_73		= slony1_funcs.v73.sql

SQL_NAMES =				\
	$(BASE_COMMON)		\
	$(FUNCS_COMMON)		\
	$(BASE_73)			\
	$(FUNCS_73)

ALL = $(SQL_NAMES)		\
	$(FUNCS_SONAME)


all:	$(ALL)

$(FUNCS_SONAME):	$(FUNCS_SOOBJS)

clean distclean maintainer-clean:
	rm -f $(FUNCS_SONAME) $(FUNCS_SOOBJS) $(FUNCS_COMMON)

install: all installdirs
	$(INSTALL_SCRIPT) $(FUNCS_SONAME) $(libdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(libdir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(libdir)


