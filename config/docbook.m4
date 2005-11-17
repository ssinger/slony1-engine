# $Id: docbook.m4,v 1.4 2005-11-17 16:30:56 xfade Exp $

# SLON_AC_PROG_JADE
# --------------
AC_DEFUN([SLON_AC_PROG_JADE],
[AC_CHECK_PROGS([JADE], [openjade jade])])


# SLON_AC_PROG_NSGMLS
# ----------------
AC_DEFUN([SLON_AC_PROG_NSGMLS],
[AC_CHECK_PROGS([NSGMLS], [onsgmls nsgmls])])


# SLON_AC_CHECK_DOCBOOK(VERSION)
# ---------------------------
AC_DEFUN([SLON_AC_CHECK_DOCBOOK],
[AC_REQUIRE([SLON_AC_PROG_NSGMLS])
AC_CACHE_CHECK([for DocBook V$1], [slonac_cv_check_docbook],
[cat >conftest.sgml <<EOF
<!doctype book PUBLIC "-//OASIS//DTD DocBook V$1//EN">
<book>
 <title>test</title>
 <chapter>
  <title>random</title>
   <sect1>
    <title>testsect</title>
    <para>text</para>
  </sect1>
 </chapter>
</book>
EOF

slonac_cv_check_docbook=no

if test -n "$NSGMLS"; then
  $NSGMLS -s conftest.sgml 1>&AS_MESSAGE_LOG_FD 2>&1
  if test $? -eq 0; then
    slonac_cv_check_docbook=yes
  fi
fi
rm -f conftest.sgml])

have_docbook=$slonac_cv_check_docbook
AC_SUBST([have_docbook])
])# SLON_AC_CHECK_DOCBOOK


# SLON_AC_PATH_DOCBOOK_STYLESHEETS
# -----------------------------
AC_DEFUN([SLON_AC_PATH_DOCBOOK_STYLESHEETS],
[AC_ARG_VAR(DOCBOOKSTYLE, [location of DocBook stylesheets])dnl
AC_MSG_CHECKING([for DocBook stylesheets])
AC_CACHE_VAL([slonac_cv_path_stylesheets],
[if test -n "$DOCBOOKSTYLE"; then
  slonac_cv_path_stylesheets=$DOCBOOKSTYLE
else
  for slonac_prefix in /usr /usr/local /opt; do
    for slonac_infix in share lib; do
      for slonac_postfix in \
        sgml/stylesheets/nwalsh-modular \
        sgml/stylesheets/docbook \
	sgml/docbook-dsssl \
        sgml/docbook/dsssl/modular \
        sgml/docbook/stylesheet/dsssl/modular \
        sgml/docbook/dsssl-stylesheets \
        sgml/stylesheets/dsssl/docbook
      do
        slonac_candidate=$slonac_prefix/$slonac_infix/$slonac_postfix
        if test -r "$slonac_candidate/html/docbook.dsl" \
           && test -r "$slonac_candidate/print/docbook.dsl"
        then
          slonac_cv_path_stylesheets=$slonac_candidate
          break 3
        fi
      done
    done
  done
fi])
DOCBOOKSTYLE=$slonac_cv_path_stylesheets
AC_SUBST([DOCBOOKSTYLE])
if test -n "$DOCBOOKSTYLE"; then
  AC_MSG_RESULT([$DOCBOOKSTYLE])
else
  AC_MSG_RESULT(no)
fi])# SLON_C_PATH_DOCBOOK_STYLESHEETS


# SLON_AC_PATH_COLLATEINDEX
# ----------------------
AC_DEFUN([SLON_AC_PATH_COLLATEINDEX],
[AC_REQUIRE([SLON_AC_PATH_DOCBOOK_STYLESHEETS])dnl
if test -n "$DOCBOOKSTYLE"; then
  AC_PATH_PROGS(COLLATEINDEX, collateindex.pl, [],
                [$DOCBOOKSTYLE/bin $PATH])
else
  AC_PATH_PROGS(COLLATEINDEX, collateindex.pl)
fi])# PGAC_PATH_COLLATEINDEX
