dnl Process this file with autoconf to produce a configure script.

m4_define([xarchiver_version], [0.4.0])

AC_INIT([xarchiver], [xarchiver_version], [])
AC_PREREQ([2.50])


XARCHIVER_VERSION=xarchiver_version
AM_INIT_AUTOMAKE([xarchiver], [$XARCHIVER_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX
AM_CONDITIONAL([HAVE_CYGWIN], [test "`uname | grep \"CYGWIN\"`" != ""])


dnl check for standard header files
AC_PROG_CC
AC_PROG_RANLIB

AC_HEADER_STDC
AC_SYS_LARGEFILE
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES"

dnl Check for i18n support
XDT_I18N([])


XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.2.0])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.2.0])

dnl check for debugging support
XDT_FEATURE_DEBUG

AC_OUTPUT([
po/Makefile.in
Makefile
src/Makefile
libxarchiver/Makefile
])

echo "----------------------------------------"
echo "$PACKAGE $VERSION"
echo ""
echo "The binary will be installed in $prefix/bin"
echo ""
echo "Configure finished, type 'make' to build."
