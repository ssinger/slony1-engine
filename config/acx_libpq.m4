dnl Checking for libpq and postgres backend features
dnl
AC_DEFUN([ACX_LIBPQ], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C


AC_MSG_CHECKING(for pg_config)

dnl Checking for pg_config in a list of possible locations.
PGCONFIG_POSSIBLE_LOCATIONS="${with_pgconfigdir} /usr/local/pgsql/bin /usr/local/bin /usr/bin /bin /usr/lib/postgresql/bin /opt/local/pgsql/bin /opt/pgsql/bin/"
for i in $PGCONFIG_POSSIBLE_LOCATIONS; do
    if test -x $i/pg_config; then
	PG_CONFIG_LOCATION="$i/pg_config"
	break;
    fi
done

if test -n "$PG_CONFIG_LOCATION"; then
    AC_MSG_RESULT([$PG_CONFIG_LOCATION])
    PG_BINDIR=`$PG_CONFIG_LOCATION --bindir`/
    PG_LIBDIR=`$PG_CONFIG_LOCATION --libdir`/
    PG_INCLUDEDIR=`$PG_CONFIG_LOCATION --includedir`/
    PG_PKGLIBDIR=`$PG_CONFIG_LOCATION --pkglibdir`/
    PG_INCLUDESERVERDIR=`$PG_CONFIG_LOCATION --includedir-server`/
    PG_CONFIGURE=`$PG_CONFIG_LOCATION --configure`
    PG_VERSION=`$PG_CONFIG_LOCATION --version|cut -f2- -d' '|cut -f1 -d'd'|cut -f-2 -d'.'`

    AC_MSG_CHECKING(for correct version of PostgreSQL)
    PG_MAJOR_VERSION=`echo $PG_VERSION |\
           sed 's/\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    PG_MINOR_VERSION=`echo $PG_VERSION |\
           sed 's/\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    if test "$PG_MAJOR_VERSION" = "7"; then
	if test $PG_MINOR_VERSION -gt 2; then
	    AC_MSG_RESULT($PG_VERSION)
	    AC_DEFINE(PG_VERSION_OK,1,[PostgreSQL 7.3.3 or later])
	else
	    AC_MSG_RESULT("error")
	    AC_MSG_ERROR(Your version of PostgreSQL ($PG_VERSION) is lower 
	    than the required 7.3.3. Slony-I needs functions included in
	    a newer version.)
	fi
    fi
    if test "$PG_MAJOR_VERSION" = "8"; then
      AC_MSG_RESULT($PG_VERSION)
      AC_DEFINE(PG_VERSION_OK,1,[PostgreSQL 7.3.3 or later])
    fi
    case ${host_os} in
	aix*|*solaris*)
		AC_MSG_CHECKING(PostgreSQL for enable-thread-safety as required on ${host_os})
		PG_ETS=`echo $PG_CONFIGURE | grep -c enable-thread-safety`
		if test $PG_ETS -eq 0; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR(PostgreSQL needs to be compiled with --enable-thread-safety on ${host_os})
		else
			AC_MSG_RESULT(yes)
		fi
	;;
	*)
	;;
    esac
else
    dnl Specify the commandline options here.
    AC_MSG_ERROR(Cannot find pg_config. 
    Please specify correct installation path for pg_config 
    with --with-pgconfigdir=<dir>
    )
fi

dnl -----------------------------------------
dnl Override the detected values with commandline options
dnl -----------------------------------------



if test -n "${with_pgbindir}" ; then
_pwd=`pwd`
cd ${with_pgbindir}
with_pgbindir=`pwd`
cd ${_pwd}
echo "Overriding pgbindir with" ${with_pgbindir}
PG_BINDIR=${with_pgbindir}
fi

if test -n "${with_pglibdir}"; then
_pwd=`pwd`
cd ${with_pglibdir}
with_pglibdir=`pwd`
cd ${_pwd}
echo "Overriding pglibdir with" ${with_pglibdir}
PG_LIBDIR=${with_pglibdir}
fi

if test -n "${with_pgincludedir}"; then
_pwd=`pwd`
cd ${with_pgincludedir}
with_pgincludedir=`pwd`
cd ${_pwd}
echo "Overriding pgincludedir with" ${with_pgincludedir}
PG_INCLUDEDIR=${with_pgincludedir}
fi

if test -n "${with_pgpkglibdir}"; then
_pwd=`pwd`
cd ${with_pgpkglibdir}
with_pgpkglibdir=`pwd`
cd ${_pwd}
echo "Overriding pgpkglibdir with" ${with_pgpkglibdir}
PG_PKGLIBDIR=${with_pgpkglibdir}
fi

if test -n "${with_pgincludeserverdir}"; then
_pwd=`pwd`
cd ${with_pgincludeserverdir}
with_pgincludeserverdir=`pwd`
cd ${_pwd}
echo "Overriding pgincludeserverdir with" ${with_pgincludeserverdir}
PG_INCLUDESERVERDIR=${with_pgincludeserverdir}
fi

if test -n "${with_toolsdir}"; then
_pwd=`pwd`
cd ${with_toolsdir}
with_toolsdir=`pwd`
cd ${_pwd}
echo "Overriding toolsdir with" ${with_toolsdir}
TOOLSBINDIR=${with_toolsdir}
fi

dnl -----------------------------------------
dnl Make sure we have found the right values!
dnl -----------------------------------------

dnl -----------------------------------------
dnl Because aix needs pg built with thread 
dnl saftey we force PTHREAD_LIBS to be added 
dnl to the test compile, there are more OS's
dnl that this may/should be done for
dnl


case ${host_os} in
aix*)
        LIBS="$LIBS  $PTHREAD_LIBS"
;;
esac


TEMP_LDFLAGS=$LDFLAGS
LDFLAGS="$TEMP_FLAGS -L$PG_LIBDIR"

AC_CHECK_LIB(pq, PQunescapeBytea, HAVE_PQUNESCAPEBYTEA=1)
if test -n "$HAVE_PQUNESCAPEBYTEA"; then
    AC_DEFINE(PG_VERSION_VERIFIED,1,[PostgreSQL 7.3 or later check])
else
    AC_MSG_ERROR(Your version of libpq doesn't have PQunescapeBytea
     this means that your version of PostgreSQL is lower than 7.3 
     and thus not supported by Slony-I.)
fi

TEMP_CPPFLAGS=$CPPFLAGS

CPPFLAGS="$TEMP_CPPFLAGS -I$PG_INCLUDEDIR"
AC_CHECK_HEADER(libpq-fe.h, HAVE_LIBPQFE=1)
if test -n "HAVE_LIBPQFE" ; then
    AC_DEFINE(PG_LIBPQ_VERIFIED,1,[PostgreSQL header check])
else
    AC_MSG_ERROR(Headers for libpq are not found in the includedir.
    Please specify the includedir with --with-pgincludedir=<dir>
    )
fi

CPPFLAGS="$TEMP_CPPFLAGS -I$PG_INCLUDEDIR -I$PG_INCLUDESERVERDIR"

AC_CHECK_HEADER(postgres.h, HAVE_LIBPQSERVER=1)
AC_CHECK_HEADER(utils/typcache.h, AC_DEFINE(HAVE_TYPCACHE,1,[PostgreSQL typcache]),[],[#include "postgres.h"])
if test -n "$HAVE_LIBPQSERVER" -a "$HAVE_TYPCACHE" != "yes"; then
    AC_DEFINE(PG_LIBPQSERVER_VERIFIED,1,[PostgreSQL header check])
else
    AC_MSG_ERROR(Headers for libpqserver are not found in the includeserverdir.
    This is the path to postgres.h. Please specify the includeserverdir with 
    --with-pgincludeserverdir=<dir>
    )
fi

LDFLAGS="$TEMP_FLAGS -L$PG_PKGLIBDIR"
AC_MSG_CHECKING(for plpgsql.so)
if test -s $PG_PKGLIBDIR"/plpgsql.so"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(PG_PKGLIBDIR_VERIFIED,1,[PostgreSQL pkglibdir])
elif test -s $PG_PKGLIBDIR"plpgsql.sl"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(PG_PKGLIBDIR_VERIFIED,1,[PostgreSQL pkglibdir])
elif test -s $PG_PKGLIBDIR"plpgsql.dll"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(PG_PKGLIBDIR_VERIFIED,1,[PostgreSQL pkglibdir])
else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR($PG_PKGLIBDIR/plpgsql.[so|sl|dll] is not found in the pkglibdir.
    Please specify the pkglibdir with --with-pgpkglibdir=<dir>
    )
fi

AC_MSG_CHECKING(for postgresql.conf.sample)
PGSHARE_POSSIBLE_LOCATIONS="${with_pgsharedir} /usr/local/pgsql/share /usr/local/share/postgresql /usr/share/postgresql /usr/local/share/pgsql /usr/share/pgsql /opt/local/pgsql/share /opt/pgsql/share ${PG_BINDIR}/../share"
for i in $PGSHARE_POSSIBLE_LOCATIONS; do
    if test -s "$i/postgresql.conf.sample" ; then
	PG_SHAREDIR=$i/
	break;
    fi
done

if test -n "$PG_SHAREDIR" ; then
    AC_MSG_RESULT(${PG_SHAREDIR}postgresql.conf.sample)
    AC_DEFINE(PG_SHAREDIR_VERIFIED,1,[PostgreSQL sharedir])
else
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(
	postgresql.conf.sample not found! Please specify the sharedir
	with --with-pgsharedir=<dir>
	)
fi

LDFLAGS="$TEMP_FLAGS -L$PG_LIBDIR"

have_pqputcopydata=no
AC_CHECK_LIB(pq, [PQputCopyData], [have_pqputcopydata=yes])
if test $have_pqputcopydata = yes ; then
        AC_DEFINE(HAVE_PQPUTCOPYDATA,1,[Postgresql PQputCopyData()])
fi

have_pqsetnoticereceiver=no
AC_CHECK_LIB(pq, [PQsetNoticeReceiver], [have_pqsetnoticereceiver=yes])
if test $have_pqsetnoticereceiver = yes; then
	 AC_DEFINE(HAVE_PQSETNOTICERECEIVER,1,[Postgresql PQsetNoticeReceiver()])
fi

have_pqfreemem=no
AC_CHECK_LIB(pq, [PQfreemem], [have_pqfreemem=yes])
if test $have_pqfreemem = yes; then
	AC_DEFINE(HAVE_PQFREEMEM,1,[Postgresql PQfreemem()])
fi

AC_CHECK_DECLS([GetTopTransactionId],[],[],[
#include "postgres.h"
#include "access/xact.h"
])

AC_LANG_RESTORE
])dnl ACX_LIBPQ
