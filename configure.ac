# ----------
# configure.ac
#
#	Autoconf configuration file for Slony-I
#
#	Copyright (c) 2003-2004, PostgreSQL Global Development Group
#	Author: Jan Wieck, Afilias USA INC.
#
# $Id: configure.ac,v 1.28 2004-07-21 22:33:31 darcyb Exp $
#
# Process this file with autoconf to produce a configure script.
# ----------
m4_define([SLONREL_VERSION], esyscmd([echo "$Name:  $" | sed -e 's/\: REL_//;s/\$//g;s/_/./g;s/\./\-/3;s/\ //g'| tr -d '\n']))
ifelse([SLONREL_VERSION],`:',m4_define([SLONREL_VERSION], esyscmd([echo `date +HEAD-%Y%m%d`|tr -d '\n'])))
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

##Portability checks
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([sys/socket.h]) 
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])
				

# ----
# Locate PostgreSQL paths
# ----
AC_ARG_WITH(pgconfigdir, [  --with-pgconfigdir=<dir>	     Location of the PostgreSQL pg_config program. ])
AC_ARG_WITH(pgbindir, [  --with-pgbindir=<dir>	     Location of the PostgreSQL postmaster. ])
AC_ARG_WITH(pgincludedir, [  --with-pgincludedir=<dir>          Location of the PostgreSQL headers. ])
AC_ARG_WITH(pgincludeserverdir, [  --with-pgincludeserverdir=<dir>    Location of the PostgreSQL server headers. ])
AC_ARG_WITH(pglibdir, [  --with-pglibdir=<dir>              Location of the PostgreSQL libs. ])
AC_ARG_WITH(pgpkglibdir, [  --with-pgpkglibdir=<dir>           Location of the PostgreSQL pkglibs. E.g. plpgsql.so ])
AC_ARG_WITH(pgsharedir, [  --with-pgsharedir=<dir>            Location of the PostgreSQL share dir. E.g. postgresql.conf.sample ])

#Our current path
SLONYPATH=`pwd`

# ----
# PostgreSQL checks
# ----
ACX_LIBPQ()

AC_SUBST(PGINCLUDEDIR, $PG_INCLUDEDIR)
AC_SUBST(PGINCLUDESERVERDIR, $PG_INCLUDESERVERDIR)
AC_SUBST(PGLIBDIR, $PG_LIBDIR)
AC_SUBST(PGPKGLIBDIR, $PG_PKGLIBDIR)
AC_SUBST(PGSHAREDIR, $PG_SHAREDIR)
AC_SUBST(PGBINDIR, $PG_BINDIR)
AC_SUBST(SLONYPATH, $SLONYPATH)
AC_SUBST(HOST_OS,$host_os)
AC_SUBST(PORTNAME,$template)

AC_CONFIG_FILES([
	Makefile.global GNUmakefile
])

AC_OUTPUT([
    postgresql-slony1-engine.spec
    Makefile.port:makefiles/Makefile.${template}
])
