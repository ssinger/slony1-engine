# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.3 2003-12-03 20:08:54 wieck Exp $
#
# Process this file with autoconf to produce a configure script.
# ----------
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_PREFIX_DEFAULT(/usr/local/slony1)

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

AC_ARG_WITH(pghome, [  --with-pghome=<dir> Location of postgres home directory], AC_SUBST(PGHOME, $with_pghome))

if test "x$with_pghome" = x ; then
  AC_MSG_ERROR([
                ********************************************
                Location of postgres home directory required
                
                       Use --with-pghome option                    
                ********************************************])
else
  if test ! -f "$with_pghome/include/server/executor/spi.h" ; then
    AC_MSG_ERROR([
           $with_pghome/include/server/executor/spi.h NOT found
           Was PostgreSQL installed including 'make install-all-headers'?])
  fi
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_CHECK_FUNCS([strchr strpbrk])

AC_CONFIG_FILES([
	Makefile.global 
])

AC_CONFIG_LINKS([
  Makefile.port:makefiles/Makefile.${template}
])


AC_OUTPUT
