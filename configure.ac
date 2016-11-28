# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# The Automake init of 1.10 requires at least autoconf 2.62
AC_PREREQ(2.63)

#Autoconf versioning is A:C:B
AC_INIT([libsphde], [1.4.0], munroesj@us.ibm.com raji@linux.vnet.ibm.com tulioqm@br.ibm.com)

AC_CONFIG_SRCDIR([src/sassim.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])
AM_MAINTAINER_MODE([disable])

# Update this value for every release: (A:B:C will map to foo.so.(A-C).C.B)
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
SPHDE_SO_VERSION=1:4:0
AC_SUBST(SPHDE_SO_VERSION)

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
LDFLAGS="$LDFLAGS -Wl,--no-as-needed -lrt"

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Tools flags
AC_SUBST([AM_LIBTOOLFLAGS], [--silent])

# Defines host_cpu, host_vendor, and host_os variables.
AC_CANONICAL_HOST

# GCC using -march=i386 does not provide __sync_fetch_and_add_4
# move -march to a high chip level to get the required support
case $host in
x86_64*)
  case $LD in
  *elf_i386*)
    if test "x$GCC" = xyes; then
      CFLAGS="$CFLAGS -march=i686"
    fi
    if test "x$GXX" = xyes; then
      CXXFLAGS="$CXXFLAGS -march=i686"
    fi
  ;;
  esac
;;
*64*)
;;
*)
AC_MSG_WARN([
*** __BIGREGION__ is not defined.
*** Adjust the address limits in sasconf.h as required.])
;;
esac

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stddef.h stdint.h stdlib.h string.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_STRTOD
AC_CHECK_FUNCS([clock_gettime ftruncate getcwd getpagesize gettimeofday memset munmap strerror])

# Pthread check (on m4/acx_pthread.m4)
ACX_PTHREAD

# Check for static libc
if test "$enable_static" = yes ; then
SAVE_LDFLAGS="$LDFLAGS"
LDFLAGS="-static $LDFLAGS"
AC_CHECK_LIB([c], [printf], [],
             AC_MSG_ERROR([*** No static libc found. Try to install glibc-static or libc6-dev.]))
LDFLAGS="$SAVE_LDFLAGS"
fi

AX_CHECK_COMPILE_FLAG([-mhtm],[
        CFLAGS="$CFLAGS -mhtm"
        CXXFLAGS="$CXXFLAGS -mhtm"
        htm=true
])
AM_CONDITIONAL([HTM], [test x$htm = xtrue])

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
  examples/Makefile
  src/Makefile
])

AC_OUTPUT
