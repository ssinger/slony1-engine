# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.13 2004-07-12 18:52:56 xfade Exp $
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global
override CFLAGS += -I$(slony_top_builddir)

NAME		= slony1_funcs

SO_NAME	= $(NAME)$(DLSUFFIX)
SO_OBJS	= $(NAME).o

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
	$(SO_NAME)


all:	$(ALL)

$(SO_NAME):	$(SO_OBJS)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(DESTDIR)$(pgpkglibdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(DESTDIR)$(pgsharedir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(pgpkglibdir)
	$(mkinstalldirs) $(DESTDIR)$(pgsharedir)



distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

