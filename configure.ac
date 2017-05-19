AC_INIT([bard], [1.0], [https://github.com/DelusionalLogic/bard/issues])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_CONFIG_SRCDIR([src/xlib_color.c])
AC_PROG_CC
AX_CHECK_COMPILE_FLAG([-std=gnu11],,[AC_MSG_FAILURE([no acceptable C11 compiler found.])])
AC_PROG_CC_STDC
AC_CHECK_HEADERS([iniparser.h])
AC_CONFIG_HEADERS([config.h])

# Check for glib
PKG_CHECK_MODULES([GIO], [gio-unix-2.0 >= 2.0])

AC_ARG_ENABLE(
			  [documentation],
			  AS_HELP_STRING([--disable-documentation], [Disable documentation])
)

AS_IF([test "x$enable_documentation" != "xno"], [
	# Check for asciidoc
	AC_PATH_PROG(A2X, a2x, no)
	AS_IF(
		[test "x$A2X" != xyes],
		[enable_documentation=yes],
		[AC_MSG_ERROR([Asciidoc is required to build documentation])]
	)
])

AM_CONDITIONAL(HAVE_A2X, test "x$A2X" != "xno")
AM_CONDITIONAL(WANT_DOC, test "x$enable_documentation" != "xno")



AC_CONFIG_FILES([
Makefile
src/Makefile
docs/Makefile
docs/bard.txt
])
CFLAGS="$CFLAGS -std=gnu11"
AC_OUTPUT
