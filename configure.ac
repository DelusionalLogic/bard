AC_INIT([bard], [1.0], [https://github.com/DelusionalLogic/bard/issues])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/xlib_color.c])
: ${CFLAGS="-g -O0 -Wall"}
AC_PROG_CC
AX_CHECK_COMPILE_FLAG([-std=gnu11],,[AC_MSG_FAILURE([no acceptable C11 compiler found.])])
AC_PROG_CC_STDC
AC_CHECK_HEADERS([iniparser.h])
AC_CONFIG_HEADERS([config.h])

# Check for asciidoc
AC_PATH_PROG(A2X, a2x, no)
AM_CONDITIONAL(HAVE_A2X, test "x$A2X" != "xno")
if test x"$A2X" = x"no"; then
	AC_MSG_ERROR([Asciidoc is required to build documentation])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
docs/Makefile
docs/bard.txt
])
CFLAGS="$CFLAGS -std=gnu11"
AC_OUTPUT
