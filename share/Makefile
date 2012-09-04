# ----------
# Makefile for share
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = share
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile slon.conf-example upstart-slon.conf-sample

CONF_NAMES = slon.conf-sample upstart-slon.conf-sample

install: 
	for file in $(CONF_NAMES); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(sysconfdir) || exit ; \
	done

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done
