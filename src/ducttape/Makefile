# ----------
# Makefile for src/ducttape
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.3 2004-01-09 21:33:14 wieck Exp $
# ----------

subdir = src/ducttape
top_builddir = ../..
include $(top_builddir)/Makefile.global

SCHEMA_SH	= slony1_schema.sh
INITCL_SH	= slony1_initcluster.sh
ADDNODE_SH	= slony1_addnode.sh

COMMON_SH	= slony1_common.sh

SCRIPTS		=			\
	$(SCHEMA_SH)		\
	$(INITCL_SH)		\
	$(ADDNODE_SH)

INCS		= 			\
	$(COMMON_SH)

ALL =					\
	$(SCRIPTS)			\
	$(INCS)


all:	$(ALL)

clean distclean maintainer-clean:
	rm -f $(ALL)

install: all installdirs
	for file in $(SCRIPTS); do \
	  $(INSTALL_SCRIPT) $$file $(bindir)/`basename $$file .sh` || exit ; \
	done ; \
	for file in $(INCS); do \
	  $(INSTALL_SCRIPT) $$file $(libdir)/$$file || exit ; \
	done



installdirs:
	$(mkinstalldirs) $(bindir)
	


