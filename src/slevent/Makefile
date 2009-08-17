#-------------------------------------------------------------------------
#
# Makefile for src/slevent
#
# Copyright (c) 1996-2009, PostgreSQL Global Development Group
#
#-------------------------------------------------------------------------

slony_subdir = src/slevent
slony_top_builddir = ../..
SLFILEDESC="Slony-I Eventlog Message Library"
SLFILESHLIB=1
include $(slony_top_builddir)/Makefile.global

OBJS=slevent.o slmsgevent.o
NAME=slevent.dll

all: slevent.dll 

install: all install-lib

slevent.dll: $(OBJS) slevent.def
	dllwrap --def slevent.def -o $(NAME) $(OBJS)
	
slmsgevent.o: slmsgevent.rc win32ver.rc
	windres slmsgevent.rc -o slmsgevent.o --include-dir=$(slony_top_builddir)

all-lib: $(NAME)

install-lib: $(NAME)
	$(INSTALL_SCRIPT) $< $(DESTDIR)$(pgpkglibdir)/$<

uninstall-lib:
	rm -f $(DESTDIR)$(libdir)/$(NAME)

clean distclean:
	rm -f $(OBJS) $(NAME) win32ver.rc

clean-lib:
	rm -f $(NAME)
