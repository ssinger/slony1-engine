PGAC_ARG_BOOL(enable, debug, no,
              [  --enable-debug          build with debugging symbols (-g)])
AC_SUBST(enable_debug)

AC_DEFUN([PGAC_PROG_CC_NO_STRICT_ALIASING],
[AC_CACHE_CHECK([how to turn off strict aliasing in $CC],
                pgac_cv_prog_cc_no_strict_aliasing,
[pgac_save_CFLAGS=$CFLAGS
if test "$GCC" = yes; then
  pgac_try="-fno-strict-aliasing"
else
  # Maybe fill in later...
  pgac_try=
fi

for pgac_flag in $pgac_try; do
  CFLAGS="$pgac_save_CFLAGS $pgac_flag"
  _AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
                     [pgac_cv_prog_cc_no_strict_aliasing=$pgac_try
break])
done

CFLAGS=$pgac_save_CFLAGS
])
if test x"$pgac_cv_prog_cc_no_strict_aliasing" != x""; then
  CFLAGS="$CFLAGS $pgac_cv_prog_cc_no_strict_aliasing"
fi])# PGAC_PROG_CC_NO_STRICT_ALIASING

PGAC_PROG_CC_NO_STRICT_ALIASING
if test "$enable_debug" = yes && test "$ac_cv_prog_cc_g" = yes; then
  CFLAGS="$CFLAGS -g"
fi
AC_MSG_NOTICE([using CFLAGS=$CFLAGS])

if test "$GCC" = yes; then
AC_TRY_COMPILE([], [@%:@ifdef __FAST_MATH__
choke me
@%:@endif], [], [AC_MSG_ERROR([do not put -ffast-math in CFLAGS])])
fi

