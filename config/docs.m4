dnl Checking for documentation tools.
dnl

AC_DEFUN([SLON_DOCS], [
AC_LANG_SAVE
AC_PATH_PROG(GROFF,groff)
if test "x$GROFF" != x; then
  GROFF="$GROFF -U -t -p -ms -mpspic"
fi

AC_PATH_PROG(PS2PDF, ps2pdf)
AC_PATH_PROG(DJPEG, djpeg)
AC_PATH_PROG(PNMTOPS, pnmtops)

AC_LANG_RESTORE

])

