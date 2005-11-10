# ----------
# Makefile for src/xxid
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.17.2.1 2005-11-10 23:39:36 wieck Exp $
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global
override CFLAGS += $(CFLAGS_SL)
override CFLAGS += -I$(slony_top_builddir)

NAME		= xxid

SO_NAME		= $(NAME)$(DLSUFFIX)
SQL_NAME73	= $(NAME).v73.sql
SQL_NAME74	= $(NAME).v74.sql
SQL_NAME80	= $(NAME).v80.sql
SQL_NAMES	= $(SQL_NAME73) $(SQL_NAME74) $(SQL_NAME80)

SO_OBJS		= xxid.o

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\

DISTFILES = Makefile xxid.c xxid.v73.sql

all:	$(ALL)

$(SQL_NAME74):	$(SQL_NAME73)
	cp $< $@

$(SQL_NAME80):	$(SQL_NAME73)
	cp $< $@

$(SO_NAME):     $(SO_OBJS) $(POSTGRES_IMP)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)
	rm -f $(SQL_NAME74) $(SQL_NAME80)

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
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

ifeq ($(PORTNAME), aix)
$(POSTGRES_IMP): $(pgbindir)/postgres
	$(MKLDEXPORT) $(pgbindir)/postgres > $@
endif
