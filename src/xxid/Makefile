# ----------
# Makefile for src/xxid
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.11 2004-07-12 18:52:18 xfade Exp $
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

NAME		= xxid

SO_NAME		= $(NAME)$(DLSUFFIX)
SQL_NAME73	= $(NAME).v73.sql
SQL_NAME74	= $(NAME).v74.sql
SQL_NAMES	= $(SQL_NAME73) $(SQL_NAME74)

SO_OBJS		= xxid.o

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\

DISTFILES = Makefile xxid.c xxid.v73.sql.in

all:	$(ALL)

$(SQL_NAME74):	$(SQL_NAME73)
	cp $< $@

$(SO_NAME):     $(SO_OBJS)

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

