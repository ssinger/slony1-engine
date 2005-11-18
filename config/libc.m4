AC_DEFUN([SLON_AC_FUNC_POSIX_SIGNALS],
  [AC_CACHE_CHECK(for POSIX signal interface, slonac_cv_func_posix_signals,
  [AC_TRY_LINK([#include <signal.h>],
    [struct sigaction act, oact;
     sigemptyset(&act.sa_mask);
     act.sa_flags = SA_NODEFER;
     sigaction(0, &act, &oact);],
  [slonac_cv_func_posix_signals=yes],
  [slonac_cv_func_posix_signals=no])])
  if test x"$slonac_cv_func_posix_signals" = xyes ; then
    AC_DEFINE(HAVE_POSIX_SIGNALS,, [Define to 1 if you have the POSIX signal interface.])
    HAVE_POSIX_SIGNALS=$slonac_cv_func_posix_signals  
  else
    if test x"$template" != "xwin32" ; then
      if test x"$template" = "xwin" ; then
        AC_MSG_RESULT(*** Skipping error on Cygwin ***)
      else 
        AC_MSG_ERROR(Slony requires a POSIX compatible signal interface.)
      fi
    fi
  fi
  AC_SUBST(HAVE_POSIX_SIGNALS)]
)# SLON_AC_FUNC_POSIX_SIGNALS
