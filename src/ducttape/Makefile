# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.1 2003-11-28 15:09:39 wieck Exp $
# ----------

subdir = src/ducttape
top_builddir = ../..
include $(top_builddir)/Makefile.global

SCHEMA_SH	= slony1_schema.sh

SCRIPTS		=			\
	$(SCHEMA_SH)

ALL =					\
	$(SCRIPTS)


all:	$(ALL)

clean distclean maintainer-clean:
	rm -f $(ALL)

install: all installdirs
	for file in $(SCRIPTS); do \
	  $(INSTALL_SCRIPT) $$file $(bindir)/`basename $$file .sh` || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(bindir)
	


