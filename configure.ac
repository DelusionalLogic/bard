AC_INIT([bard], [1.0], [bug-report@address])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/main.c])
: ${CFLAGS="-g -O0 -Wall"}
AC_PROG_CC
AC_PROG_CC_STDC
AC_CHECK_HEADERS([iniparser.h])
AC_CONFIG_HEADERS([config.h])

# Check for asciidoc
AC_PATH_PROG(A2X, a2x, no)
AM_CONDITIONAL(HAVE_A2X, test "x$A2X" != "xno")
if test x"$A2X" = x"no"; then
  AC_MSG_ERROR([Test for asciidoc failed. See the file 'INSTALL' for help.])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
docs/Makefile
docs/bard.txt
])
AC_OUTPUT
