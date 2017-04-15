dnl Copyright (c) 2006
dnl         The Xfce development team. All rights reserved.
dnl
dnl Written for Xfce by Benedikt Meurer <benny@xfce.org>,
dnl changed and updated by Ingo Brückl.

dnl ***************
dnl *** Version ***
dnl ***************
m4_define([xarchiver_version], [0.5.4.8])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2006
        The Xfce development team. All rights reserved.

Written for Xfce by Giuseppe Torelli <colossus73@gmail.com>.
Continuation of the Xfce master branch.])
AC_PREREQ([2.60])
AC_INIT([xarchiver], [xarchiver_version], [https://github.com/ib/xarchiver/issues])
: ${CFLAGS="-O2"}
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_HOST

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.11 dist-xz])
AM_CONFIG_HEADER([config.h])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_USE_SYSTEM_EXTENSIONS([_GNU_SOURCE])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC
AC_PROG_INSTALL

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ***********************************************
dnl *** Work-around system-specific limitations ***
dnl ***********************************************
AC_SYS_LARGEFILE

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_HEADER_STDC

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_CHECK_FUNCS([strcasestr mkdtemp socket getc_unlocked mmap])

dnl ********************
dnl *** i18n support ***
dnl ********************
IT_PROG_INTLTOOL([0.35.0], [no-xml])
GETTEXT_PACKAGE=m4_default([], [AC_PACKAGE_NAME])
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AM_GLIB_GNU_GETTEXT

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
AC_ARG_ENABLE([gtk2], AS_HELP_STRING([--enable-gtk2], [compile xarchiver against GTK+ 2]))
GTK_API_VERSION=2.0
AS_IF([test x"$enable_gtk2" != x"yes"], [PKG_CHECK_EXISTS([gtk+-3.0], [GTK_API_VERSION=3.0], [test x"$enable_gtk2" = x"no" && GTK_API_VERSION=3.0])])

PKG_CHECK_MODULES([GTK], [gtk+-$GTK_API_VERSION >= 2.10.0])
GTK_VERSION_FOUND=`$PKG_CONFIG --modversion gtk+-$GTK_API_VERSION`

AS_VERSION_COMPARE([$GTK_VERSION_FOUND], [2.16.0], [], [GTK_V2_16=yes], [GTK_V2_16=yes])
AM_CONDITIONAL([GTK_V2_16], [test x"$GTK_V2_16" = x"yes"])

AS_VERSION_COMPARE([$GTK_VERSION_FOUND], [3.0.0], [], [GTK_V3_0=yes], [GTK_V3_0=yes])
AM_CONDITIONAL([GTK_V3_0], [test x"$GTK_V3_0" = x"yes"])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
AC_ARG_ENABLE(
[debug],
AC_HELP_STRING([--enable-debug[=yes|no|full]], [build with debugging support])
AC_HELP_STRING([--disable-debug], [no debugging support [default]]),
[],
[enable_debug=no]
)

AC_MSG_CHECKING([whether to build with debugging support])

if test x"$enable_debug" != x"no"; then

  AC_DEFINE([DEBUG], [1], [Define for debugging support.])

  if test x"$enable_debug" = x"full"; then
    AC_DEFINE([DEBUG_TRACE], [1], [Define for tracing support.])
    test x"$GCC" = x"yes" && xa_cv_additional_CFLAGS="-Werror -g3"
    AC_MSG_RESULT([full])
  else
    test x"$GCC" = x"yes" && xa_cv_additional_CFLAGS="-g"
    AC_MSG_RESULT([yes])
  fi

  CFLAGS="$CFLAGS $xa_cv_additional_CFLAGS"

else

  AC_MSG_RESULT([no])

fi

dnl ***************
dnl *** Options ***
dnl ***************
AC_ARG_ENABLE([doc], AS_HELP_STRING([--disable-doc], [don't install xarchiver's documentation]))
DOC=doc
AS_IF([test x"$enable_doc" = x"no"], [DOC=],
      [
        AC_PATH_PROG([XSLTPROC], [xsltproc])
        AS_IF([test -z "$XSLTPROC"], AC_MSG_ERROR([xsltproc is required for rebuilding the pre-built documentation; consider --disable-doc]))
      ]
)
AC_SUBST([DOC])

AC_ARG_ENABLE([icons], AS_HELP_STRING([--disable-icons], [don't install xarchiver's icons]))
ICONS=icons
AS_IF([test x"$enable_icons" = x"no"], [ICONS=])
AC_SUBST([ICONS])

AC_ARG_ENABLE([plugin], AS_HELP_STRING([--disable-plugin], [don't install xarchiver's Thunar plugin]))
PLUGINDIR='$(libexecdir)/thunar-archive-plugin/'
PLUGINSCRIPT=xarchiver.tap
AS_IF([test x"$enable_plugin" = x"no"], [PLUGINDIR=; PLUGINSCRIPT=])
AC_SUBST([PLUGINDIR])
AC_SUBST([PLUGINSCRIPT])

AC_ARG_WITH([pixmapdir], AS_HELP_STRING([--with-pixmapdir=DIR], [use directory DIR for xarchiver's internal pixmaps [DATADIR/pixmaps/xarchiver]]))
PIXMAPDIR='$(datadir)/pixmaps/xarchiver'
AS_IF([test x"$with_pixmapdir" != x""], [PIXMAPDIR="$with_pixmapdir"])
AC_SUBST([PIXMAPDIR])

dnl **************
dnl *** Output ***
dnl **************
AC_OUTPUT([
doc/Makefile
icons/16x16/Makefile
icons/24x24/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
icons/Makefile
pixmaps/Makefile
po/Makefile.in
src/Makefile
src/xdgmime/Makefile
Makefile
])

echo "

${PACKAGE} ${PACKAGE_VERSION} configured for ${prefix} using GTK+ $GTK_VERSION_FOUND

"
