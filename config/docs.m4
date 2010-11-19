
AC_DEFUN([SLON_AC_PROG_GROFF],
[AC_CHECK_PROGS([GROFF], [groff])])

if test "x$GROFF" != x; then
  GROFF="$GROFF -U -t -p -ms -mpspic"
fi
)

AC_DEFUN([SLON_AC_PROG_PS2PDF],
[AC_CHECK_PROGS([PS2PDF], [ps2pdf eps2pdf])])

AC_DEFUN([SLON_AC_PROG_DJPEG],
[AC_CHECK_PROGS([DJPEG], [djpeg])])

AC_DEFUN([SLON_AC_PROG_PNMTOPS],
[AC_CHECK_PROGS([PNMTOPS], [pnmtops])])

AC_DEFUN([SLON_AC_PROG_CONVERT],
[AC_CHECK_PROGS([CONVERT], [convert])])

AC_DEFUN([SLON_AC_PROG_PGAUTODOC],
[AC_CHECK_PROGS([PGAUTODOC],[postgresql_autodoc pgautodoc])])

AC_DEFUN([ACX_SLONYDOCS], [
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_LANG_SAVE
  AC_LANG_C
  LOCATION=""
  if test -n "${with_docdir}" ; then
    if test ${with_docdir} != "yes"; then
      LOCATION="${with_docdir}"
    else
      if test ${prefix} = "NONE"; then
        LOCATION="${ac_default_prefix}/doc"
      else
        LOCATION="${prefix}/doc"
      fi
    fi
  fi
  if test x"${LOCATION}" != x; then
   docdir="${LOCATION}"
  fi
  AC_LANG_RESTORE
]) dnl ACX_SLONYDOCS

AC_DEFUN([SLON_AC_DOCS])

