# ----------
# Makefile
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#       $Id: Makefile,v 1.4 2005-02-10 06:38:51 smsimms Exp $
#
# ----------

slony_subdir = tools/altperl
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile ToDo README $(wildcard *.pl) $(wildcard *.pm) slon_tools.conf-sample

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

clean distclean maintainer-clean:
	@for file in $(wildcard *.pl); do \
		tmpname=`echo $$file | $(SED) "s#\.pl##"` ; \
		rm $$tmpname ; \
	done

all:
ifndef PERL
	@echo "The altperl tools require that Perl be installed."
	exit 1;
endif
ifndef SED
	@echo "sed is required to build the altperl tools."
	exit 1;
endif
	@for file in $(wildcard *.pl) ; do \
		tmpname=`echo $$file | $(SED) "s#\.pl##"` ; \
		$(SED) -e "s#@@PERL@@#$(PERL)#;" \
                       -e "s#@@SYSCONFDIR@@#$(sysconfdir)#;" \
                       -e "s#@@PGLIBDIR@@#$(pglibdir)#;" \
                       -e "s#@@PGBINDIR@@#$(pgbindir)#;" $$file > $$tmpname; \
		chmod 755 $$tmpname; \
	done

install: all
	$(INSTALL_DATA)   slon_tools.conf-sample $(sysconfdir)
	$(INSTALL_DATA)   slon-tools.pm          $(pglibdir)
	$(INSTALL_SCRIPT) slon_start.pl          $(pgbindir)
	$(INSTALL_SCRIPT) slon_kill.pl           $(pgbindir)
	$(INSTALL_SCRIPT) slon_watchdog.pl       $(pgbindir)

