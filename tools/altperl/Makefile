# ----------
# Makefile
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#       $Id: Makefile,v 1.3 2005-02-10 05:49:20 smsimms Exp $
#
# ----------

slony_subdir = tools/altperl
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile ToDo README $(wildcard *.pl) $(wildcard *.pm) $(wildcard *.env)

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
	@echo "You need perl to make use of these tools"
else
ifndef SED
		@echo "You need sed to use slony"
		exit 1;
else
		@for file in $(wildcard *.pl) ; do \
			tmpname=`echo $$file | $(SED) "s#\.pl##"` ; \
			$(SED) -e "s#@@PERL@@#$(PERL)#; s#@@SYSCONFDIR@@#$(sysconfdir)#; s#@@PGLIBDIR@@#$(pglibdir)#;" $$file > $$tmpname; \
			chmod 755 $$tmpname; \
		done
endif
endif
