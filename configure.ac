
# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.52 2005-11-11 19:51:22 xfade Exp $
#
# Process this file with autoconf to produce a configure script.
# ----------
m4_define([SLONREL_VERSION], esyscmd([echo "$Name:  $" | \
  sed -e 's/\:\ REL_//' -e 's/\$//g' -e 's/_/./g' -e 's/\./\_/3' \
    -e 's/\ //g' -e s/\:/`date +HEAD_%Y%m%d`/ | tr -d '\n']))

AC_INIT(postgresql-slony1-engine,[SLONREL_VERSION])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src])

AC_CANONICAL_HOST

template=
AC_MSG_CHECKING([which template to use])

case $host_os in
     aix*) template=aix ;;
    beos*) template=beos ;;
    bsdi*) template=bsdi ;;
  cygwin*) template=win ;;
  darwin*) template=darwin ;;
    dgux*) template=dgux ;;
 freebsd*) template=freebsd ;;
    hpux*) template=hpux ;;
    irix*) template=irix ;;
   linux*|gnu*|k*bsd*-gnu) 
           template=linux ;;
   mingw*) template=win32 ;;
  netbsd*) template=netbsd ;;
nextstep*) template=nextstep ;;
 openbsd*) template=openbsd ;;
     osf*) template=osf ;;
     qnx*) template=qnx4 ;;
     sco*) template=sco ;;
 solaris*) template=solaris ;;
   sunos*) template=sunos4 ;;
 sysv4.2*)
        case $host_vendor in
          univel) template=univel ;;
        esac ;;
   sysv4*) template=svr4 ;;
   sysv5*) template=unixware ;;
  ultrix*) template=ultrix4 ;;
esac

if test x"$template" = x"" ; then
  AC_MSG_ERROR([[
*******************************************************************
Slony-I has apparently not been ported to your platform yet.
To try a manual configuration, look into the src/template directory
for a similar platform and use the '--with-template=' option.

Please also contact <slony1-general@gborg.postgresql.org> to see about
rectifying this.  Include the above 'checking host system type...'
line.
*******************************************************************]])
fi
AC_MSG_RESULT([$template])

# Checks for programs.
#AC_PROG_CC
SLON_AC_COMPILER()
AC_PROG_LD
PGAC_PATH_PERL
AC_PATH_PROG(TAR, tar)
AC_CHECK_PROGS(LEX, ['flex' , 'lex'])
AC_CHECK_PROGS(YACC, ['bison -y' , 'yacc'])
AC_CHECK_PROGS(SED, ['sed'])

AC_SUBST(LD)
AC_SUBST(CC)
AC_SUBST(YACC)
AC_SUBST(YFLAGS)
AC_SUBST(LEXFLAGS)
AC_SUBST(YFLAGS)
AC_SUBST(SLONREL_VERSION)
AC_SUBST(with_gnu_ld)
AC_SUBST(PERL)
AC_SUBST(SED)

dnl Check whether to use the evil rpath or not
SLON_AC_ARG_BOOL(enable, rpath, yes,
              [  --disable-rpath         do not embed shared library search path in executables])
AC_SUBST(enable_rpath)

ACX_PTHREAD()

##Portability checks
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([sys/socket.h]) 
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([inttypes.h])

AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])

AC_CHECK_TYPES([int32_t, uint32_t, u_int32_t])
AC_CHECK_TYPES([int64_t, uint64_t, u_int64_t])
AC_CHECK_TYPES([ssize_t])
SLON_AC_FUNC_POSIX_SIGNALS()

# ----
# Locate PostgreSQL paths
# ----

AC_ARG_WITH(pgconfigdir,        [  --with-pgconfigdir=<dir>         Location of the PostgreSQL pg_config program. ])
AC_ARG_WITH(pgbindir,           [  --with-pgbindir=<dir>            Location of the PostgreSQL postmaster. ])
AC_ARG_WITH(pgincludedir,       [  --with-pgincludedir=<dir>        Location of the PostgreSQL headers. ])
AC_ARG_WITH(pgincludeserverdir, [  --with-pgincludeserverdir=<dir>  Location of the PostgreSQL server headers. ])
AC_ARG_WITH(pglibdir,           [  --with-pglibdir=<dir>            Location of the PostgreSQL libs. ])
AC_ARG_WITH(pgpkglibdir,        [  --with-pgpkglibdir=<dir>         Location of the PostgreSQL pkglibs. E.g. plpgsql.so ])
AC_ARG_WITH(pgsharedir,         [  --with-pgsharedir=<dir>          Location of the PostgreSQL share dir. E.g. postgresql.conf.sample ])
AC_ARG_WITH(netsnmp,            [  --with-netsnmp=<dir>             Enable snmp support <dir> is the location of net-snmp-config. **EXPERIMENTAL** ])
AC_ARG_WITH(perltools,          [  --with-perltools=<dir>           Location to install the perl management tools Default $PREFIX/bin. ])
AC_ARG_WITH(docdir,		[  --with-docdir=<dir>              Location to install all the documentation Default is $PREFIX/doc. ])
AC_ARG_WITH(docs,		[  --with-docs=<yes|no>             Build the sgml documentation [default=no]])

#Our current path
SLONYPATH=`pwd`

# ----
# PostgreSQL checks
# ----
ACX_LIBPQ()
ACX_LIBSNMP()
ACX_SLONYTOOLS()

AC_SUBST(PGINCLUDEDIR, $PG_INCLUDEDIR)
AC_SUBST(PGINCLUDESERVERDIR, $PG_INCLUDESERVERDIR)
AC_SUBST(PGLIBDIR, $PG_LIBDIR)
AC_SUBST(PGPKGLIBDIR, $PG_PKGLIBDIR)
AC_SUBST(PGSHAREDIR, $PG_SHAREDIR)
AC_SUBST(PGBINDIR, $PG_BINDIR)
AC_SUBST(HAVE_NETSNMP, $HAVE_NETSNMP)
AC_SUBST(NETSNMP_CFLAGS, $NETSNMP_CFLAGS)
AC_SUBST(NETSNMP_AGENTLIBS, $NETSNMP_AGENTLIBS)
AC_SUBST(TOOLSBIN, $TOOLSBIN)
AC_SUBST(DOCDIR, $DOCDIR)

AC_SUBST(SLONYPATH)
AC_SUBST(HOST_OS,$host_os)
AC_SUBST(PORTNAME,$template)

#-- if test ${prefix} = "NONE"; then
SLONBINDIR="${PG_BINDIR}"
#-- else
#--   SLONBINDIR="${prefix}/bin"
#-- fi
AC_SUBST(SLONBINDIR,$SLONBINDIR)

# ----
# Documentation checks
# ----
SLON_AC_DOCS()
ACX_SLONYDOCS()

AC_SUBST(with_docs, $with_docs)

AC_MSG_CHECKING(if you have requested documentation building)
if test "$with_docs" = "yes"; then

	AC_MSG_RESULT($with_docs)

	# ----
	# Tools for building docs
	# ---

	SLON_AC_PROG_GROFF
	SLON_AC_PROG_PS2PDF
	SLON_AC_PROG_DJPEG
	SLON_AC_PROG_PNMTOPS
	SLON_AC_PROG_CONVERT
	SLON_AC_PROG_PGAUTODOC

	#
	# Check for DocBook and tools
	#
	SLON_AC_PROG_NSGMLS
	SLON_AC_PROG_JADE
	SLON_AC_CHECK_DOCBOOK(4.2)
	SLON_AC_PATH_DOCBOOK_STYLESHEETS
	SLON_AC_PATH_COLLATEINDEX
	AC_SUBST(SGMLSPL, $SGMLSPL)
else
  AC_MSG_RESULT(no)
fi

AC_CONFIG_FILES([
	Makefile.global GNUmakefile
])

AC_OUTPUT([
    postgresql-slony1-engine.spec
    Makefile.port:makefiles/Makefile.${template}
])
