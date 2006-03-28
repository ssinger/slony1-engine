# ----------
# Makefile
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#       $Id: Makefile,v 1.13 2006-03-28 20:55:46 cbbrowne Exp $
#
# ----------

slony_subdir = tools/altperl
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile ToDo README $(wildcard *.pl) $(wildcard *.pm) slon_tools.conf-sample

all:
ifdef toolsbin
ifndef PERL
	@echo "The altperl tools require that Perl be installed."
	exit 1;
endif
ifndef SED
	@echo "sed is required to build the altperl tools."
	exit 1;
endif
	@for file in $(wildcard *.pl) $(wildcard *.pm); do \
		tmpname=`echo $$file | $(SED) "s#\.p[lm]##"` ; \
		$(SED) -e "s#@@PERL@@#$(PERL)#;" \
                       -e "s#@@SYSCONFDIR@@#$(sysconfdir)#;" \
                       -e "s#@@PGLIBDIR@@#$(pglibdir)#;" \
                       -e "s#@@PGBINDIR@@#$(pgbindir)#;" \
                       -e "s#@@TOOLSBIN@@#$(toolsbin)#;" \
                       $$file > $$tmpname; \
		chmod 755 $$tmpname; \
	done
endif

install: installdirs
ifndef toolsbin
	@echo "The altperl tools won't be installed unless --with-perltools is specified in configure"
else
	$(INSTALL_DATA)   slon_tools.conf-sample $(DESTDIR)$(sysconfdir)
	$(INSTALL_DATA)   slon-tools             $(DESTDIR)$(perlsharedir)/slon-tools.pm
	for file in $(wildcard *.pl) ; do \
		tmpname=`echo $$file | $(SED) "s#\.pl##"` ; \
		$(INSTALL_SCRIPT) $$tmpname $(DESTDIR)$(toolsbin) ; \
	done
endif

installdirs:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	$(mkinstalldirs) $(DESTDIR)$(pglibdir)
	$(mkinstalldirs) $(DESTDIR)$(toolsbin)

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file ; \
	done

clean distclean maintainer-clean:
	@for file in $(wildcard *.pl) $(wildcard *.pm); do \
		tmpname=`echo $$file | $(SED) "s#\.p[lm]##"` ; \
		rm -f $$tmpname ; \
	done

