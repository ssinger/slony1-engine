dnl Checking for net-snmp.
dnl
AC_DEFUN([ACX_LIBSNMP], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C

if test -n "${with_netsnmp}" ; then
  AC_MSG_CHECKING(for net-snmp-config)

  dnl Checking for net-snmp-config in a list of possible locations.
  if test ${with_netsnmp} != "yes"; then
    POSSIBLE_LOCATIONS="${with_netsnmp}"
  else
    POSSIBLE_LOCATIONS="${with_netsnmp} ${PATH}"
  fi
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for i in $POSSIBLE_LOCATIONS; do
    if test -x "$i" -a -f "$i"; then
      NET_CONFIG_LOCATION="$i"
      break;
    fi
    if test -x "$i/net-snmp-config" -a -f "$i/net-snmp-config"; then
      NETSNMP_CONFIG_LOCATION="$i/net-snmp-config"
      break;
    fi
  done
  IFS=${ac_save_ifs}
#CFLAGS+=-I. `net-snmp-config --cflags`
#BUILDLIBS+=`net-snmp-config --libs`
#BUILDAGENTLIBS+=`net-snmp-config --agent-libs`


  if test -n "$NETSNMP_CONFIG_LOCATION"; then
    AC_MSG_RESULT([$NETSNMP_CONFIG_LOCATION])
    NETSNMP_CFLAGS=`$NETSNMP_CONFIG_LOCATION --cflags`
    NETSNMP_LIBS=`$NETSNMP_CONFIG_LOCATION --libs`
    NETSNMP_AGENTLIBS=`$NETSNMP_CONFIG_LOCATION --agent-libs`
    NETSNMP_VERSION=`$NETSNMP_CONFIG_LOCATION --version|cut -f2- -d' '|cut -f1 -d'd'|cut -f-2 -d'.'`

    AC_MSG_CHECKING(for correct version of net-snmp)
    NETSNMP_MAJOR_VERSION=`echo $NETSNMP_VERSION |\
           sed 's/\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    NETSNMP_MINOR_VERSION=`echo $NETSNMP_VERSION |\
           sed 's/\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    if test "$NETSNMP_MAJOR_VERSION" = "5"; then
      if test $NETSNMP_MINOR_VERSION -gt 0; then
        AC_MSG_RESULT($NETSNMP_VERSION)
	AC_DEFINE(NETSNMP_VERSION_OK,1,[net-snmp 5.1 or later])
        HAVE_NETSNMP=1
      else
	AC_MSG_RESULT("error")
	AC_MSG_ERROR(Your version of net-snmp ($NETSNMP_VERSION) is lower 
	  than the required 5.1 Slony-I needs functions included in
	  a newer version.)
      fi
    fi
  else
    dnl Specify the commandline options here.
    AC_MSG_ERROR(Cannot find net-snmp-config. 
    Please specify correct installation path for net-snmp-config 
    with --with-netsnmp=<dir>
    )
  fi

 if test $HAVE_NETSNMP; then
   TEMP_LDFLAGS=$LDFLAGS
   LDFLAGS="${LDFLAGS} ${NETSNMP_LIBS}"
   TEMP_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$TEMP_CPPFLAGS $NETSNMP_CFLAGS"
   HAVE_STDARG_H=0
   AC_CHECK_HEADERS(stdarg.h)
   AC_CHECK_SIZEOF(short)
   AC_CHECK_SIZEOF(int)
   AC_CHECK_SIZEOF(long)
   AC_CHECK_SIZEOF(long long)
   AC_CHECK_HEADERS(net-snmp/net-snmp-config.h, HAVE_SNMP_INCLUDES=1)
   AC_CHECK_HEADERS(net-snmp/agent/mib_module_config.h net-snmp/agent/agent_module_config.h, HAVE_SNMP_AGENT_INCLUDES=1)
   CPPFLAGS="$TEMP_CPPFLAGS"
   LDFLAGS="${TEMP_LDFLAGS}"
 fi
fi
AC_LANG_RESTORE
])dnl ACX_LIBSNMP

