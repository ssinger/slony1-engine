# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.18 2004-03-31 13:46:09 wieck Exp $
#
# Process this file with autoconf to produce a configure script.
# ----------

AC_INIT(slony1-engine, 20040330)
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
ACX_PTHREAD()

# Locate the PostgreSQL source tree

AC_ARG_WITH(pgsourcetree, [  --with-pgsourcetree=<dir> Location of PostgreSQL source tree], AC_SUBST(PGSOURCETREE, $with_pgsourcetree))

if test "x$with_pgsourcetree" = "x" ; then
	AC_MSG_ERROR([
    ********************
    * Compiling Slony-I currently requires a source installation
    * of PostgreSQL. Please specify the location of the tree you
    * built as --with-pgsourcetree=<dir>
    ********************
	])
fi

# ----
# Check if our libpq contains several functions
# ----
save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="-I${with_pgsourcetree}/src/interfaces/libpq -I${with_pgsourcetree}/src/include $CFLAGS"
LIBS="-L${with_pgsourcetree}/src/interfaces/libpq -lpq $LIBS"

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

# PQsetNoticeReceiver()
AC_MSG_CHECKING([for PQsetNoticeReceiver() in libpq])
have_pqsetnoticereceiver=no
AC_TRY_LINK([#include <libpq-fe.h>],
	   [PQsetNoticeReceiver(NULL,NULL,NULL);],
	   [have_pqsetnoticereceiver=yes])
AC_MSG_RESULT($have_pqsetnoticereceiver)
if test $have_pqsetnoticereceiver = yes ; then
	AC_DEFINE(HAVE_PQSETNOTICERECEIVER,1,)
fi

AC_MSG_CHECKING([for typcache.h])
if test -f "${with_pgsourcetree}/src/include/utils/typcache.h"; then
	have_typcache=yes
	AC_DEFINE(HAVE_TYPCACHE,1,)
else
	have_typcache=no
fi
AC_MSG_RESULT($have_typcache)

CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

AC_CONFIG_FILES([
	Makefile.global 
])

AC_OUTPUT([
    postgresql-slony1-engine.spec
])
