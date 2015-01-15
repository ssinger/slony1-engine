# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2009, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
#
# Process this file with autoconf to produce a configure script.
# ----------
m4_define([SLONREL_VERSION], esyscmd([echo "$Name:  $" | \
  sed -e 's/\:\ REL_//' -e 's/\$//g' -e 's/_/./g' -e 's/\./\_/3' \
    -e 's/\ //g' -e s/\:/`echo 2.2.4`/ | tr -d '\n']))

m4_pattern_allow([^SLON_AC_])


AC_INIT(slony1,[SLONREL_VERSION])
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


# PGAC_PATH_FLEX
# --------------
# Look for Flex, set the output variable FLEX to its path if found.
# Reject versions before 2.5.31, as we need a reasonably non-buggy reentrant
# scanner.    We need at least 2.5.31 because we depend on
# yyget_leng() to be defined since later some versions of flex switched
# the definition of yyleng from an int to a size_t
#  Also find Flex if its installed under `lex', but do not
# accept other Lex programs.

AC_DEFUN([PGAC_PATH_FLEX],
[AC_CACHE_CHECK([for flex], pgac_cv_path_flex,
[# Let the user override the test
if test -n "$FLEX"; then
  pgac_cv_path_flex=$FLEX
else
  pgac_save_IFS=$IFS
  IFS=$PATH_SEPARATOR
  for pgac_dir in $PATH; do
    IFS=$pgac_save_IFS
    if test -z "$pgac_dir" || test x"$pgac_dir" = x"."; then
      pgac_dir=`pwd`
    fi
    for pgac_prog in flex lex; do
      pgac_candidate="$pgac_dir/$pgac_prog"
      if test -f "$pgac_candidate" \
        && $pgac_candidate --version </dev/null >/dev/null 2>&1
      then
        echo '%%'  > conftest.l
        if $pgac_candidate -t conftest.l 2>/dev/null | grep FLEX_SCANNER >/dev/null 2>&1; then
          pgac_flex_version=`$pgac_candidate --version 2>/dev/null`
          if echo "$pgac_flex_version" | sed ['s/[.a-z]/ /g'] | awk '{ if ([$]1 = 2 && [$]2 = 5 && [$]3 >= 31) exit 0; else exit 1;}'
          then
            pgac_cv_path_flex=$pgac_candidate
            break 2
          else
            AC_MSG_WARN([
*** The installed version of Flex, $pgac_candidate, is too old to use with Slony-I.
*** Flex version 2.5.31 or later is required, but this is $pgac_flex_version.])
          fi
        fi
      fi
    done
  done
  rm -f conftest.l lex.yy.c
  : ${pgac_cv_path_flex=no}
fi
])[]dnl AC_CACHE_CHECK

if test x"$pgac_cv_path_flex" = x"no"; then
  AC_MSG_WARN([
*** Without Flex you will not be able to build Slony-I from CVS nor
*** change any of the scanner definition files.  You can obtain Flex from
*** a GNU mirror site.  (If you are using the official distribution of
*** Slony-I then you do not need to worry about this because the Flex
*** output is pre-generated.)])

  LEX=
else
  LEX=$pgac_cv_path_flex
  pgac_flex_version=`$FLEX --version 2>/dev/null`
  AC_MSG_NOTICE([using $pgac_flex_version])
fi

AC_SUBST(LEX)
AC_SUBST(FLEXFLAGS)
])# PGAC_PATH_FLEX


AC_CHECK_PROGS(YACC, ['bison -y' , 'yacc'])
AC_CHECK_PROGS(SED, ['sed'])

AC_SUBST(LD)
AC_SUBST(CC)
AC_SUBST(GCC)
AC_SUBST(YACC)
AC_SUBST(YFLAGS)
AC_SUBST(LEXFLAGS)
AC_SUBST(YFLAGS)
#AC_SUBST(SLONREL_VERSION)
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
AC_CHECK_TYPES([size_t, ssize_t])
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
AC_ARG_WITH(slonybindir, 	[  --with-slonybindir=<dir>	    Location to install slony binaries (slon,slonik,etc..) Default is $PREFIX/bin. ])
AC_ARG_WITH(perltools,          [  --with-perltools=<dir>           Location to install the perl management tools. Default $PREFIX/bin. ])
AC_ARG_WITH(perlsharedir,       [  --with-perlsharedir=<dir>        Location to install slon-tools.pm. Default $pglibdir. ])
AC_ARG_WITH(docs,		[  --with-docs=<yes|no>             Build the sgml documentation [default=no]])
AC_ARG_WITH(docdir,		[  --with-docdir=<dir>              Location to install all the documentation. Default is $PREFIX/doc. ])
AC_ARG_WITH(d2mdir,		[  --with-d2mdir=<dir>              Location of docbook2man-spec.pl (Manpages will be skipped if not specified)])
AC_ARG_WITH(mandir,		[  --with-mandir=<dir>              Location to install the manpages. Default is $PREFIX/man.])
AC_ARG_WITH(pgport,             [  --with-pgport=<yes|no>           Link with pgport [default=no]])

SLON_AC_ARG_BOOL(enable, engine, yes,
              [  --disable-engine     Don't build slony1-engine source. (Used when building documentation only)])

AC_SUBST(enable_engine)

#Our current path
SLONYPATH=`pwd`

PGAC_PATH_FLEX()

# ----
# PostgreSQL checks
# ----
AC_MSG_CHECKING(if you have requested slony1-engine building)
AC_MSG_RESULT($enable_engine)

if test "$enable_engine" = "yes"; then

ACX_LIBPQ()
ACX_SLONYTOOLS()

AC_SUBST(PG_VERSION_MAJOR, $PG_VERSION_MAJOR)
AC_SUBST(PG_VERSION_MINOR, $PG_VERSION_MINOR)
AC_SUBST(PG_VERSION, $PG_VERSION)

AC_SUBST(PGINCLUDEDIR, $PG_INCLUDEDIR)
AC_SUBST(PGINCLUDESERVERDIR, $PG_INCLUDESERVERDIR)
AC_SUBST(PGLIBDIR, $PG_LIBDIR)
AC_SUBST(PGPKGLIBDIR, $PG_PKGLIBDIR)
AC_SUBST(PGSHAREDIR, $PG_SHAREDIR)
AC_SUBST(PGBINDIR, $PG_BINDIR)
AC_SUBST(TOOLSBIN, $TOOLSBIN)

AC_SUBST(SLONYPATH)
AC_SUBST(HOST_OS,$host_os)
AC_SUBST(PORTNAME,$template)

AC_SUBST(HAVE_PGPORT,$HAVE_PGPORT)
AC_SUBST(HAVE_PGCOMMON,$HAVE_PGCOMMON)
if test "${SLON_BINDIR}" = ""; then
 if test ${prefix} = "NONE"; then
   SLON_BINDIR="${PG_BINDIR}"
 else
   SLON_BINDIR="${prefix}/bin"
 fi
fi
AC_SUBST(SLONBINDIR,$SLON_BINDIR)

fi

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
    SLON_AC_PROG_SGMLSPL
    SLON_AC_PROG_D2M

    SLON_AC_PROG_JADE
    SLON_AC_CHECK_DOCBOOK(4.2)
    SLON_AC_PATH_DOCBOOK_STYLESHEETS
    SLON_AC_PATH_COLLATEINDEX

    AC_SUBST(docdir, $docdir)
    AC_SUBST(mandir, $mandir)
else
  AC_MSG_RESULT(no)
fi

if test x"$with_perlsharedir" = x""; then
    with_perlsharedir="$PG_LIBDIR"
fi

AC_SUBST(perlsharedir, $with_perlsharedir)
AC_SUBST(CPPFLAGS_SERVER)
AC_SUBST(CPPFLAGS_CLIENT)
AC_CONFIG_FILES([
    Makefile.global GNUmakefile
])

AC_OUTPUT([
    slony1.spec
    Makefile.port:makefiles/Makefile.${template}
])
