# ----------
# Makefile for src/xxid
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.5 2004-03-12 23:55:02 wieck Exp $
# ----------

subdir = src/xxid
top_builddir = ../..
include $(top_builddir)/Makefile.global

NAME		= xxid

SO_NAME		= $(NAME)$(DLSUFFIX)
SQL_NAME73	= $(NAME).v73.sql
SQL_NAME74	= $(NAME).v74.sql
SQL_NAMES	= $(SQL_NAME73) $(SQL_NAME74)

SO_OBJS		= xxid.o

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\


all:	$(ALL)

$(SQL_NAME74):	$(SQL_NAME73)
	cp $< $@

$(SO_NAME):	$(SO_OBJS)

clean distclean maintainer-clean:
	rm -f $(ALL) $(SO_OBJS)
	rm -f $(SQL_NAME74)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(libdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(datadir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(libdir)


