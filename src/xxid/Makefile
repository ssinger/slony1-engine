# Makefile for src/xxid
# Author: Jan Wieck, Afilias USA INC.
#
#	$Header: /local/home/ssinger/cvs2svn/cvs2svn-2.3.0/slony-cvsd/slony1-engine/src/xxid/Attic/Makefile,v 1.1 2003-11-17 17:29:00 wieck Exp $
#

subdir = src/xxid
top_builddir = ../..
include $(top_builddir)/Makefile.global

NAME		= xxid

SO_NAME		= $(NAME)$(DLSUFFIX)
SQL_NAME73	= $(NAME).v73.sql
SQL_NAMES	= $(SQL_NAME73)

SO_OBJS		= xxid.o

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\


all:	$(ALL)

$(SO_NAME):	$(SO_OBJS)

clean distclean maintainer-clean:
	rm -f $(ALL) $(SO_OBJS)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(libdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(libdir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(libdir)


