# ----------
# Makefile for src/xxid
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.13 2004-08-18 23:39:54 darcyb Exp $
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

NAME		= xxid

SO_NAME		= $(NAME)$(DLSUFFIX)
SQL_NAME73	= $(NAME).v73.sql
SQL_NAME74	= $(NAME).v74.sql
SQL_NAMES	= $(SQL_NAME73) $(SQL_NAME74)
POSTGRES_IMP=
SO_OBJS		= xxid.o

ifeq ($(PORTNAME), aix)
POSTGRES_IMP=$(slony_top_builddir)/postgresql.imp
endif

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\

DISTFILES = Makefile xxid.c xxid.v73.sql

all:	$(ALL)

$(SQL_NAME74):	$(SQL_NAME73)
	cp $< $@

$(SO_NAME):     $(SO_OBJS) $(POSTGRES_IMP)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)
	rm -f $(SQL_NAME74)

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

ifeq ($(PORTNAME), aix)
$(POSTGRES_IMP): $(pgbindir)/postgres
	$(MKLDEXPORT) $(pgbindir)/postgres > $@
endif
