# ----------
# Makefile for share
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.1 2004-11-02 17:55:06 darcyb Exp $
# ----------

slony_subdir = share
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile slon.conf-example

CONF_NAMES = slon.conf-sample

install: 
	for file in $(CONF_NAMES); do \
	  $(INSTALL_SCRIPT) $$file $(DESTDIR)$(pgsharedir) || exit ; \
	done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done
