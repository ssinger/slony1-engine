
AC_PATH_PROG(GROFF,groff)
if test "x$GROFF" != x; then
  GROFF="$GROFF -U -t -p -ms -mpspic"
fi

AC_PATH_PROG(PS2PDF, ps2pdf)
AC_PATH_PROG(DJPEG, djpeg)
AC_PATH_PROG(PNMTOPS, pnmtops)

