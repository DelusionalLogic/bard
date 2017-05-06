AC_INIT([bard], [1.0], [https://github.com/DelusionalLogic/bard/issues])
AC_CONFIG_MACRO_DIRS([m4])
AM_PROG_AR()
LT_INIT()
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_CONFIG_SRCDIR([bard_src/xlib_color.c])
AC_CONFIG_SRCDIR([sharedlib_src/myerror.c])
AC_PROG_CC
AX_CHECK_COMPILE_FLAG([-std=gnu11],,[AC_MSG_FAILURE([no acceptable C11 compiler found.])])
AC_PROG_CC_STDC
AC_CHECK_HEADERS([iniparser.h])
AC_CONFIG_HEADERS([config.h])

# Check for glib
PKG_CHECK_MODULES([GIO], [gio-unix-2.0 >= 2.0])

# Check for asciidoc
AC_PATH_PROG(A2X, a2x, no)
AM_CONDITIONAL(HAVE_A2X, test "x$A2X" != "xno")
if test x"$A2X" = x"no"; then
	AC_MSG_ERROR([Asciidoc is required to build documentation])
fi

AC_CONFIG_FILES([
Makefile
bard_src/Makefile
sharedlib_src/Makefile
docs/Makefile
docs/bard.txt
])
CFLAGS="$CFLAGS -std=gnu11"
AC_OUTPUT
