# ----------
# Makefile for doc
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = doc
slony_top_builddir = ..
include $(slony_top_builddir)/Makefile.global

SUBDIRS = adminguide

DISTFILES = Makefile

all clean distclean maintainer-clean install installdirs:
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $@ || exit; \
	done


distdir: $(DISTFILES)
	mkdir -p $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	@for file in $(DISTFILES) ; do \
		cp $$file $(distdir)/$(subdir)/$$file || exit; \
	done
	@for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir distdir || exit; \
	done

adminguide.tar:

	$(MAKE) -C adminguide clean
	$(MAKE) -C adminguide html
	$(MAKE) -C adminguide html JADEFLAGS='-V html-manifest'
	cd adminguide && $(TAR) -cf ../$@ `cat HTML.manifest` `echo *.gif | grep -v '\*'` *.css
