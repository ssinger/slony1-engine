# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.14 2004-03-12 23:55:02 wieck Exp $
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

# Checks for libraries.

# Checks for header files.

# The installation locations must be identical to those of postgresql
# or we will never be able to administer a box remote. Check for
# certain files to be in those locations.

test_prefix=/usr/local/pgsql
eval test_dir="${includedir}"
if test "${test_dir}" = "NONE/include" ; then
	test_dir="${test_prefix}/include"
fi
if test ! -f "${test_dir}/server/executor/spi.h" ; then
    AC_MSG_ERROR([
        ${test_dir}/server/executor/spi.h NOT found
        you need to specify --includedir= pointing to the same directory
        PostgreSQL was configured with and PostgreSQL must be installed 
        including 'make install-all-headers'])
fi
test_includedir="${test_dir}"

eval test_dir="${bindir}"
if test "${test_dir}" = "NONE/bin" ; then
	test_dir="${test_prefix}/bin"
fi
if test ! -f "${test_dir}/postmaster" ; then
    AC_MSG_ERROR([
        ${test_dir}/postmaster NOT found
        you need to specify --bindir= pointing to the same directory
        PostgreSQL was configured with])
fi

eval test_dir="${datadir}"
if test "${test_dir}" = "NONE/share" ; then
	test_dir="${test_prefix}/share"
fi
if test ! -f "${test_dir}/postgresql.conf.sample" ; then
    AC_MSG_ERROR([
        ${test_dir}/postgresql.conf.sample NOT found
        you need to specify --datadir= pointing to the same directory
        PostgreSQL was configured with])
fi
AC_DEFINE_UNQUOTED(PGSHARE, "${test_dir}", )

eval test_dir="${libdir}"
if test "${test_dir}" = "NONE/lib" ; then
	test_dir="${test_prefix}/lib"
fi
if test ! -f "`ls ${test_dir}/plpgsql*`" ; then
    AC_MSG_ERROR([
        ${test_dir}/plpgsql module NOT found
        you need to specify --libdir= pointing to the same directory
        PostgreSQL was configured with])
fi
test_libdir="${test_dir}"


# ----
# Check if our libpq contains several functions
# ----
save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="-I${test_includedir} $CFLAGS"
LIBS="-L${test_libdir} -lpq $LIBS"

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

AC_MSG_CHECKING([for typcache.h])
if test -f "${test_includedir}/server/utils/typcache.h"; then
	have_typcache=yes
	AC_DEFINE(HAVE_TYPCACHE,1,)
else
	have_typcache=no
fi
AC_MSG_RESULT($have_typcache)

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
	src/slonik/Makefile
])

AC_CONFIG_LINKS([
	Makefile.port:makefiles/Makefile.${template}
])


AC_OUTPUT
