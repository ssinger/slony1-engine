# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.8 2004-03-03 21:52:46 wieck Exp $
#
# Process this file with autoconf to produce a configure script.
# ----------
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src])
AC_PREFIX_DEFAULT(/usr/local/pgsql)

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
    irix*) template=irix5 ;;
   linux*) template=linux ;;
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

AC_MSG_RESULT([$template])

# Checks for programs.
AC_PROG_CC
AC_PROG_LD
AC_SUBST(LD)
AC_SUBST(with_gnu_ld)
AC_PATH_PROG(bash_path,bash)
AC_SUBST(bash_path)

# Checks for libraries.

# Checks for header files.

# The installation prefix must be the postgres home. Otherwise
# we will never be able to administer a box remote.
pghome=$prefix
test "x$pghome" = xNONE && pghome=$ac_default_prefix
if test ! -f "$pghome/include/server/executor/spi.h" ; then
    AC_MSG_ERROR([
           $pghome/include/server/executor/spi.h NOT found
           Was PostgreSQL installed including 'make install-all-headers'?])
fi
AC_SUBST(PGHOME, $pghome)

# ----
# Check if our libpq contains several functions
# ----
save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="-I${pghome}/include $CFLAGS"
LIBS="-L${pghome}/lib -lpq $LIBS"

# PQfreemem()
AC_MSG_CHECKING([for PQfreemem() in libpq])
have_pqfreemem=no
AC_TRY_LINK([#include <libpq-fe.h>],
	   [PQfreemem(NULL);],
	   [have_pqfreemem=yes])
AC_MSG_RESULT($have_pqfreemem)
if test $have_pqfreemem = yes ; then
	AC_DEFINE(HAVE_PQFREEMEM,1,)
fi

# PQputCopyData()
AC_MSG_CHECKING([for PQputCopyData() in libpq])
have_pqputcopydata=no
AC_TRY_LINK([#include <libpq-fe.h>],
	   [PQputCopyData(NULL,NULL,0);],
	   [have_pqputcopydata=yes])
AC_MSG_RESULT($have_pqputcopydata)
if test $have_pqputcopydata = yes ; then
	AC_DEFINE(HAVE_PQPUTCOPYDATA,1,)
fi

CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_HEADER_STDC
ACX_PTHREAD()
AC_CHECK_FUNCS([strchr strpbrk])

AC_CONFIG_FILES([
	Makefile.global 
	src/slon/Makefile
])

AC_CONFIG_LINKS([
  Makefile.port:makefiles/Makefile.${template}
])


AC_OUTPUT
