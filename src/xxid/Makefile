# ----------
# Makefile for src/xxid
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = src/xxid
slony_top_builddir = ../..
SLFILEDESC = "Slony xxid datatype"
SO_NAME		= $(NAME)$(DLSUFFIX)
include $(slony_top_builddir)/Makefile.global
override CFLAGS += $(CFLAGS_SL)
override CFLAGS += -I$(slony_top_builddir)

NAME		= xxid

SQL_NAME74	= $(NAME).v74.sql
SQL_NAME80	= $(NAME).v80.sql
SQL_NAME81	= $(NAME).v81.sql
SQL_NAME84  = $(NAME).v84.sql
SQL_NAMES	= $(SQL_NAME74) $(SQL_NAME80) $(SQL_NAME81) $(SQL_NAME84)
GENSQLNAMES = $(filter-out $(SQL_NAME74), $(SQL_NAMES))

SO_OBJS		= xxid.o $(WIN32RES)

ALL =					\
	$(SO_NAME)			\
	$(SQL_NAMES)		\

DISTFILES = Makefile xxid.c xxid.v74.sql

all:	$(ALL)

$(SQL_NAME80):	$(SQL_NAME74)
	cp $< $@

$(SQL_NAME81):	$(SQL_NAME80)
	cp $< $@

$(SO_NAME):     $(SO_OBJS) $(POSTGRES_IMP)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)
	rm -f $(GENSQLNAMES)

install: all installdirs
	$(INSTALL_SCRIPT) $(SO_NAME) $(DESTDIR)$(pgpkglibdir)
	for file in $(SQL_NAMES); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(pgsharedir) || exit ; \
	done


installdirs:
	$(mkinstalldirs) $(DESTDIR)$(pgpkglibdir)
	$(mkinstalldirs) $(DESTDIR)$(pgsharedir)


distdir: $(DISTFILES)
	mkdir $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file || exit; \
    done

ifeq ($(PORTNAME), aix)
$(POSTGRES_IMP): $(pgbindir)/postgres
	$(MKLDEXPORT) $(pgbindir)/postgres > $@
endif

splint:
	splint -I $(pgincludedir) -I $(pgincludeserverdir) +unixlib -preproc +skip-sys-headers xxid.c
