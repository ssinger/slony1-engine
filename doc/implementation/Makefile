# ----------
# Makefile for the Slony-I implementation paper
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: Makefile,v 1.2 2004-01-19 03:28:44 wieck Exp $
# ----------


GROFF	= groff
GOPTS	= -U -t -p -ms -mpspic
PS2PDF	= ps2pdf

DJPEG	= djpeg
PNMTOPS	= pnmtops
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


all:	all-ps all-pdf

all-ps:	$(PS)

all-pdf:	$(PDF)

all-txt:	$(TXT)

install installdirs:	all

clean distclean maintainer-clean:
	rm -f $(PS) $(PDF) $(TXT) tmp.* Slon.eps

Slony-I-implementation.ps:		$(SRC_CONCEPT)
Slony-I-implementation.pdf:	$(SRC_CONCEPT)
Slony-I-implementation.txt:	$(SRC_CONCEPT)

Slon.eps:	Slon_$(IMG_WID)x$(IMG_HT).jpg
	djpeg $< | pnmtops -rle -noturn >$@

%.ps:	%.nr
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

%.txt:	%.nr
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

%.pdf:	%.ps
	$(PS2PDF) $<

