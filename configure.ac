dnl Autoconf file for building bayes_opt codels library.

AC_PREREQ(2.59)

AC_INIT([bayes_opt-genom3],[0.8],[])
AC_CONFIG_MACRO_DIR([autoconf])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_HEADERS([autoconf/acbayes_opt.h])
AM_INIT_AUTOMAKE([foreign no-define])

dnl Compilers
dnl
LT_INIT([disable-static])
AC_PROG_CC


dnl Require GNU make
AC_CACHE_CHECK([for GNU make], [ac_cv_path_MAKE],
  [AC_PATH_PROGS_FEATURE_CHECK([MAKE], [make gmake],
    [case `$ac_path_MAKE --version 2>/dev/null` in
       *GNU*) ac_cv_path_MAKE=$ac_path_MAKE; ac_path_MAKE_found=:;;
     esac],
    [AC_MSG_ERROR([could not find GNU make])])])
AC_SUBST([MAKE], [$ac_cv_path_MAKE])


dnl External packages
AC_SUBST(requires_SPEC, ['m4_normalize([
  openrobots2-idl >= 2.0,
  genom3 >= 2.99.26
])'])
AC_SUBST(codels_requires_SPEC)
AC_SUBST(codels_requires_CFLAGS)
AC_SUBST(codels_requires_LIBS)

PKG_CHECK_MODULES(requires, [$requires_SPEC])

AC_PATH_PROG(GENOM3, [genom3], [no])
if test "$GENOM3" = "no"; then
  AC_MSG_ERROR([genom3 tool not found], 2)
fi

dnl --with-templates option
AG_OPT_TEMPLATES([$GENOM3 ],
    [bayes_opt.gen])

dnl Doc
AM_MISSING_PROG([ASCIIDOCTOR], [asciidoctor])

dnl Output
AC_CONFIG_FILES([
	bayes_opt-genom3.pc
	bayes_opt-genom3-uninstalled.pc
	Makefile
	codels/Makefile
])
AC_OUTPUT
AG_OUTPUT_TEMPLATES
