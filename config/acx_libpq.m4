dnl Checking for libpq.
dnl
AC_DEFUN([ACX_LIBPQ], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C


AC_MSG_CHECKING(for pg_config)

dnl Checking for pg_config in a list of possible locations.
PGCONFIG_POSSIBLE_LOCATIONS="/usr/local/pgsql/bin /usr/local/bin /usr/bin /bin /usr/lib/postgresql/bin /opt/local/pgsql/bin /opt/pgsql/bin/ ${with_pgconfigdir}"
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
	    AC_DEFINE(PG_VERSION_OK,1,[PostgreSQL 7.3 or later])
	else
	    AC_MSG_RESULT("error")
	    AC_MSG_ERROR(Your version of PostgreSQL ($PG_VERSION) is lower 
	    than the required 7.3.x. Slony-I needs functions included in
	    a newer version.)
	fi
    fi
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
echo "Overriding pgbindir with" ${with_pgbindir}
PG_BINDIR=${with_pgbindir}
fi
if test -n "${with_pglibdir}"; then
echo "Overriding pglibdir with" ${with_pglibdir}
PG_LIBDIR=${with_pglibdir}
fi
if test -n "${with_pgincludedir}"; then
echo "Overriding pgincludedir with" ${with_pgincludedir}
PG_INCLUDEDIR=${with_pgincludedir}
fi
if test -n "${with_pgpkglibdir}"; then
echo "Overriding pgpkglibdir with" ${with_pgpkglibdir}
PG_PKGLIBDIR=${with_pgpkglibdir}
fi
if test -n "${with_pgincludeserverdir}"; then
echo "Overriding pgincludeserverdir with" ${with_pgincludeserverdir}
PG_INCLUDESERVERDIR=${with_pgincludeserverdir}
fi

dnl -----------------------------------------
dnl Make sure we have found the right values!
dnl -----------------------------------------

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
if test -n "$HAVE_LIBPQSERVER" ; then
    AC_DEFINE(PG_LIBPQSERVER_VERIFIED,1,[PostgreSQL header check])
else
    AC_MSG_ERROR(Headers for libpqserver are not found in the includeserverdir.
    This is the path to postgres.h. Please specify the includeserverdir with 
    --with-pgincludeserverdir=<dir>
    )
fi

LDFLAGS="$TEMP_FLAGS -L$PG_PKGLIBDIR"
AC_MSG_CHECKING(for plpgsql.so)
if test -s $PG_PKGLIBDIR"plpgsql.so"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(PG_PKGLIBDIR_VERIFIED,1,[PostgreSQL pkglibdir])
else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR($PG_PKGLIBDIR/plpgsql.so is not found in the pkglibdir.
    Please specify the pkglibdir with --with-pgpkglibdir=<dir>
    )
fi

AC_MSG_CHECKING(for postgresql.conf.sample)
PGSHARE_POSSIBLE_LOCATIONS="/usr/local/pgsql/share /usr/local/share/postgresql /usr/share/postgresql /usr/local/share/pgsql /usr/share/pgsql /opt/local/pgsql/share /opt/pgsql/share ${with_pgsharedir} $(PG_BINDIR)/../share"
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

AC_LANG_RESTORE
])dnl ACX_LIBPQ
