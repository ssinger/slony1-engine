# ----------
# Makefile for the Slony-I implementation paper
#
#	Copyright (c) 2003-2009,  PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# 
# ----------

slony_subdir = doc/implementation
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

GOPTS	= -U -t -p -ms -mpspic

# IMG_WID	= 1799
# IMG_HT	= 1440
IMG_WID	= 900
IMG_HT	= 720

SRC_CONCEPT =	Slony-I-implementation.nr	\
				Makefile			\
				figure-1.pic		\
				figure-2.pic		\
				Slon.eps

PDF		= Slony-I-implementation.pdf

PS		= Slony-I-implementation.ps

TXT		= Slony-I-implementation.txt

DISTFILES = Makefile $(wildcard *.pic) Slon_900x720.jpg Slony-I-implementation.nr

all:		all-ps all-pdf all-txt

all-ps:		Slon.eps $(PS)

all-pdf:	Slon.eps $(PDF)

all-txt:	Slon.eps $(TXT)

installdirs:
ifdef docdir
	$(mkinstalldirs) $(DESTDIR)$(docdir)/implementation
endif

install: all installdirs
ifdef docdir
	for file in $(PS) $(PDF) $(TXT) Slon.eps Slon_$(IMG_WID)x$(IMG_HT).jpg ; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/implementation || exit; \
	done
endif

clean distclean maintainer-clean:
	rm -f $(PS) $(PDF) $(TXT) tmp.* Slon.eps

Slony-I-implementation.ps:	$(SRC_CONCEPT)
Slony-I-implementation.pdf:	$(SRC_CONCEPT)
Slony-I-implementation.txt:	$(SRC_CONCEPT)

Slon.eps:	Slon_$(IMG_WID)x$(IMG_HT).jpg
ifdef DJPEG
  ifdef PNMTOPS
		$(DJPEG) $< | $(PNMTOPS) -rle -noturn >$@
  else
    ifdef CONVERT
		$(CONVERT) $< >$@ || echo "unable to create $@ from $<"
    else
		@echo "unable to create $@ from $<"
    endif
  endif
else
  ifdef CONVERT
		$(CONVERT) $< >$@ || echo "unable to create $@ from $<"
  else
		@echo "unable to create $@ from $<"
  endif
endif

%.ps:	%.nr
ifdef GROFF
		@echo ".XS 1"	> tmp.idx
		@echo "dummy"	>>tmp.idx
		@echo ".XE"		>>tmp.idx
		@echo ""		> tmp.old
		-$(GROFF) $(GOPTS) $< >$@ 2>/dev/null
		@n=0 ; \
		while ! cmp tmp.idx tmp.old >/dev/null ; do \
			cp tmp.idx tmp.old ; \
			n=`expr $$n + 1` ; \
			if [ $$n -gt 10 ] ; then break ; fi ; \
			echo "$(GROFF) $(GOPTS) $< >$@" ; \
			$(GROFF) $(GOPTS) $< >$@ ; \
		done
		@rm -f tmp.*
endif

%.txt:	%.nr
ifdef GROFF
		@echo ".XS 1"	> tmp.idx
		@echo "dummy"	>>tmp.idx
		@echo ".XE"		>>tmp.idx
		@echo ""		> tmp.old
		-$(GROFF) -Tlatin1 $(GOPTS) $< >$@ 2>/dev/null
		@n=0 ; \
		while ! cmp tmp.idx tmp.old >/dev/null ; do \
			cp tmp.idx tmp.old ; \
			n=`expr $$n + 1` ; \
			if [ $$n -gt 10 ] ; then break ; fi ; \
			echo "$(GROFF) -Tlatin1 $(GOPTS) $< >$@" ; \
			$(GROFF) -Tlatin1 $(GOPTS) $< >$@ ; \
		done
		@rm -f tmp.*
endif

%.pdf:	%.ps
ifdef PS2PDF
		$(PS2PDF) $<
endif

distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done

