# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	$Id: Makefile,v 1.10 2004-12-17 00:32:18 darcyb Exp $
# ----------

slony_subdir = doc
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = concept implementation howto adminguide support

DISTFILES = Makefile

all install installdirs:

clean distclean maintainer-clean:
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $@ ; \
	done


distdir: $(DISTFILES)
	mkdir -p $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	@for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file ; \
	done
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir distdir ; \
	done

adminguide.tar:

	$(MAKE) -C adminguide clean
	$(MAKE) -C adminguide html
	$(MAKE) -C adminguide html JADEFLAGS='-V html-manifest'
	cd adminguide && $(TAR) -cf ../$@ `cat HTML.manifest` `echo *.gif | grep -v '\*'` *.css
