AC_DEFUN([SLON_AC_COMPILER],
[SLON_AC_ARG_BOOL(enable, debug, no,
              [  --enable-debug          build with debugging symbols (-g)])
AC_SUBST(enable_debug)

AC_DEFUN([SLON_AC_PROG_CC_NO_STRICT_ALIASING],
[AC_CACHE_CHECK([how to turn off strict aliasing in $CC],
                slon_ac_cv_prog_cc_no_strict_aliasing,
[slon_ac_save_CFLAGS=$CFLAGS
if test "$GCC" = yes; then
  slon_ac_try="-fno-strict-aliasing"
else
  # Maybe fill in later...
  slon_ac_try=
fi

for slon_ac_flag in $slon_ac_try; do
  CFLAGS="$slon_ac_save_CFLAGS $slon_ac_flag"
  _AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
                     [slon_ac_cv_prog_cc_no_strict_aliasing=$slon_ac_try
break])
done

CFLAGS=$slon_ac_save_CFLAGS
])
if test x"$slon_ac_cv_prog_cc_no_strict_aliasing" != x""; then
  CFLAGS="$CFLAGS $slon_ac_cv_prog_cc_no_strict_aliasing"
fi])# SLOn_AC_PROG_CC_NO_STRICT_ALIASING

if test "$enable_debug" = yes && test "$ac_cv_prog_cc_g" = yes; then
  CFLAGS="$CFLAGS -g"
fi
AC_MSG_NOTICE([using CFLAGS=$CFLAGS])])

