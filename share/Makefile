# ----------
# Makefile for share
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.2 2005-02-11 17:05:05 darcyb Exp $
# ----------

slony_subdir = share
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile slon.conf-example

CONF_NAMES = slon.conf-sample

install: 
	for file in $(CONF_NAMES); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(pgsharedir) || exit ; \
	done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done
