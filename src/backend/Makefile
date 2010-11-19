# ----------
# Makefile for src/backend
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#	
# ----------

slony_subdir = src/backend
slony_top_builddir = ../..
SLFILEDESC="Slony replication functions"
SO_NAME = $(NAME)$(DLSUFFIX)
include $(slony_top_builddir)/Makefile.global

override CFLAGS += -I$(slony_top_builddir) $(CFLAGS_SL)

NAME		= slony1_funcs

SO_OBJS	= $(NAME).o $(WIN32RES)

BASE_COMMON		= slony1_base.sql
FUNCS_COMMON	= slony1_funcs.sql

BASE_74			= slony1_base.v74.sql
FUNCS_74		= slony1_funcs.v74.sql

BASE_80			= slony1_base.v80.sql
FUNCS_80		= slony1_funcs.v80.sql

BASE_81			= slony1_base.v81.sql
FUNCS_81		= slony1_funcs.v81.sql

BASE_84			= slony1_base.v84.sql
FUNCS_84		= slony1_funcs.v84.sql

SQL_NAMES =				\
	$(BASE_COMMON)		\
	$(FUNCS_COMMON)		\
	$(BASE_74)			\
	$(FUNCS_74)			\
	$(BASE_80)			\
	$(FUNCS_80)			\
	$(BASE_81)			\
	$(FUNCS_81) \
	$(BASE_84) \
	$(FUNCS_84)

DISTFILES = Makefile README README.events $(wildcard *.sql) $(wildcard *.in) $(wildcard *.c)

ALL = $(SQL_NAMES)		\
	$(SO_NAME)


all:	$(ALL)

$(SO_NAME):	$(SO_OBJS)

clean distclean maintainer-clean:
	rm -f $(SO_NAME) $(SO_OBJS)

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

