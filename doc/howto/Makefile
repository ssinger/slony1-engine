# ----------
# Makefile for the HOWTOs
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#
# ----------

slony_subdir = doc/howto
slony_top_builddir = ../..
include $(slony_top_builddir)/Makefile.global

DISTFILES = Makefile $(wildcard *.txt*) $(wildcard *.html*)

distdir: $(DISTFILES)
	mkdir -p $(distdir)/$(subdir)
	-chmod 777 $(distdir)/$(subdir)
	for file in $(DISTFILES) ; do \
      cp $$file $(distdir)/$(subdir)/$$file ; \
    done

# Here's a somewhat fiddly way of generating documentation for the set
# of functions and tables using Rod Taylor's postgresql_autodoc tool
# schemadoc is actually likely to be checked into CVS, so you don't
# _always_ want to recreate it

# Assumptions:
#  - it's safe to create database "schemadoc" on a local database
#  - my "createlang" hides in a bit of an odd place
#  - "make clean" should really drop the database
#  - you need to manually drop the database before regenning the docs

BASEDIR=$(slony_top_builddir)/src/backend
BASESQL=$(BASEDIR)/slony1_base.sql 
BASEFUNS=$(BASEDIR)/slony1_funcs.sql 
XIDSQL=localxid.sql
# Might want to add version-specific functions, too...
TEMPDB=schemadoc
TEMPSCHEMA=schemadoc
CREATELANG=$(pgbindir)/createlang   # That's how it is for me...
AUTODOC=postgresql_autodoc

schemadoc.html: $(BASESQL) $(BASEFUNS) $(XIDDIR)
	$(pgbindir)/createdb $(TEMPDB) && ( \
	$(CREATELANG) plpgsql $(TEMPDB) && \
	echo "drop schema $(TEMPSCHEMA);create schema $(TEMPSCHEMA);" | $(pgbindir)/psql $(TEMPDB) && \
	cat $(XIDSQL) $(BASEFUNS) $(BASESQL) |  sed -e "s/@NAMESPACE@/$(TEMPSCHEMA)/g"  -e "s/@CLUSTERNAME@/$(TEMPSCHEMA)/g" | $(pgbindir)/psql $(TEMPDB) && \
	$(AUTODOC) -d $(TEMPDB) -s $(TEMPSCHEMA) -t html ;\
	@$(pgbindir)/dropdb $(TEMPDB) >/dev/null 2>&1
	) || echo "unable to createdb $(TEMPDB)"

clean:
	@$(pgbindir)/dropdb $(TEMPDB) || echo "unable to dropdb $(TEMPDB)" 
