# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = src/backend
slony_top_builddir = ../..
SLFILEDESC="Slony replication functions"
NAME		= slony1_funcs
SO_NAME = $(NAME).$(VERSION)$(DLSUFFIX)
include $(slony_top_builddir)/Makefile.global
CPPFLAGS:=$(CPPFLAGS_SERVER)
override CFLAGS += -I$(slony_top_builddir) $(CFLAGS_SL)


SO_OBJS	= $(NAME).o $(WIN32RES)

BASE_COMMON		= slony1_base.sql
FUNCS_COMMON	= slony1_funcs.sql

BASE_83			= slony1_base.v83.sql
FUNCS_83		= slony1_funcs.v83.sql
BASE_84			= slony1_base.v84.sql
FUNCS_84		= slony1_funcs.v84.sql

SQL_NAMES =				\
	$(BASE_COMMON)		\
	$(FUNCS_COMMON)		\
	$(BASE_83)			\
	$(BASE_84)			\
	$(FUNCS_83)			\
	$(FUNCS_84)

DISTFILES = Makefile README README.events $(wildcard *.sql) $(wildcard *.in) $(wildcard *.c)

ALL = $(SQL_NAMES)		\
	$(SO_NAME)


all:	$(ALL)

$(SO_NAME):	$(SO_OBJS)

$(NAME).o:	$(NAME).c avl_tree.c avl_tree.h

avl_tree.c:	../misc/avl_tree.c
	cp $< $@

avl_tree.h:	../misc/avl_tree.h
	cp $< $@

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS) avl_tree.c avl_tree.h

splint:
	splint -I $(pgincludedir) -I $(pgincludeserverdir) +unixlib -preproc +skip-sys-headers $(wildcard *.c)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(DESTDIR)$(pgpkglibdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(pgsharedir)/`basename $$file .sql`.$(VERSION).sql || exit ; \
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

