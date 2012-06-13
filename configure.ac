# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(libsphde, 0.0.1, munroesj@us.ibm.com)

AC_CONFIG_SRCDIR([src/sassim.cpp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 no-define foreign])

# Debug option
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [Enable debugging information])],
  [],
  [enable_debug=no])

DEBUG=0
AS_IF([test "x$enable_debug" != xno],
  [AC_SUBST([DEBUG], [1])])
if test "x$enable_debug" = "xyes" ; then
  CFLAGS="$CFLAGS -D__SOMDebugPrint__ -Dmylockdebug -DcoherenceCheck -D__SASDebugPrint__"
  CXXFLAGS="$CXXFLAGS -D__SOMDebugPrint__ -Dmylockdebug -DcoherenceCheck -D__SASDebugPrint__"
fi

# This directive is to avoid buggy libtool that don't add the -Wl,--no-as-needed
# directive in the correct position of LDFLAGS
LDFLAGS="-Wl,--no-as-needed -lrt"

# Check for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_LN_S

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

# Tools flags
AC_SUBST([AM_LIBTOOLFLAGS], [--silent])

# Defines host_cpu, host_vendor, and host_os variables.
AC_CANONICAL_HOST

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_MALLOC

# Pthread check (on m4/acx_pthread.m4)
ACX_PTHREAD

# Doxygen support
DX_HTML_FEATURE(ON)
DX_MAN_FEATURE(ON)
DX_PDF_FEATURE(OFF)
DX_INIT_DOXYGEN($PACKAGE_NAME, [ \
  doc/libsphde-doxygen-sasutil.doxy \
  doc/libsphde-doxygen-sph.doxy
])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  examples/Makefile
])

AC_OUTPUT
