
AC_DEFUN([SLON_AC_PROG_GROFF],
[AC_CHECK_PROGS([GROFF], [groff])], 

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

AC_DEFUN([SLON_AC_PROG_PGAUTODOC],
[AC_CHECK_PROGS([PGAUTODOC],[postgresql_autodoc pgautodoc])])
