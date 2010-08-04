dnl
AC_DEFUN([ACX_SLONYTOOLS], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
LOCATION=""
if test -n "${with_perltools}" ; then
  if test ${with_perltools} != "yes"; then
    LOCATION="${with_perltools}"
  else
    if test ${prefix} = "NONE"; then
      LOCATION="${ac_default_prefix}/bin"
    else
      LOCATION="${prefix}/bin"
    fi
  fi
  if test -n "$PERL"; then
    AC_MSG_CHECKING(for DBD::Pg)
    HAVE_SLONYTOOLS=1
  else
    dnl Specify the commandline options here.
    AC_MSG_ERROR(Cannot find DBD:Pg. 
    You require DBD::Pg to install the slony tools
    )
  fi

 if test $HAVE_SLONYTOOLS; then
   TOOLSBIN=${LOCATION}
 fi
fi
AC_LANG_RESTORE
])dnl ACX_SLONYTOOLS

