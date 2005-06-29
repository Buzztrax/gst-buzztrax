# $Id: configure.ac,v 1.7 2005-06-29 09:20:17 ensonic Exp $

#
# current  : the interface         : MAJOR + MINOR
# revision : of the same interface : MICRO
# age      : MINOR
#
# 1: when releasing a changed version : MICRO++
# 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
# 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
#

# Initialise the autoconf/automake tool
AC_INIT([gst-buzztard], [0.0.1], [buzztard-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)

GSTBT_MAJOR_VERSION=0
GSTBT_MINOR_VERSION=0
GSTBT_MICRO_VERSION=1
GSTBT_VERSION=$GSTBT_MAJOR_VERSION.$GSTBT_MINOR_VERSION.$GSTBT_MICRO_VERSION
GSTBT_VERSION_INFO=`expr $GSTBT_MAJOR_VERSION + $GSTBT_MINOR_VERSION`:$GSTBT_MICRO_VERSION:$GSTBT_MINOR_VERSION
GSTBT_VERSION_NUMBER=`expr $GSTBT_MAJOR_VERSION \* 10000 + $GSTBT_MINOR_VERSION \* 100 + $GSTBT_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
GSTBT_MAJORMINOR=$GSTBT_MAJOR_VERSION.$GSTBT_MINOR_VERSION
dnl we override it here for release candidates
#GSTBT_MAJORMINOR=0.8

dnl that is needed to figure the directory name, where the plugin goes to
GST_MAJORMINOR=0.8
AC_SUBST(GST_MAJORMINOR)

AC_SUBST(GSTBT_MAJOR_VERSION)
AC_SUBST(GSTBT_MINOR_VERSION)
AC_SUBST(GSTBT_MICRO_VERSION)
AC_SUBST(GSTBT_VERSION)
AC_SUBST(GSTBT_VERSION_INFO)
AC_SUBST(GSTBT_VERSION_NUMBER)
AC_SUBST(GSTBT_MAJORMINOR)

VERSION=${GSTBT_VERSION}

# Initialise the automake tool
AM_INIT_AUTOMAKE([check-news])

# m4 - everyone who develops should have aclocal installed
AC_PATH_PROG(PROG_ACLOCAL, aclocal, no)
AC_MSG_CHECKING(where to install .m4 files to)
if test "$PROG_ACLOCAL" = "no" ; then
	acdir=${datadir}/aclocal
else
	acdir=`$PROG_ACLOCAL --print-ac-dir`
	if test ! -w "$acdir" ; then
		acdir=${datadir}/aclocal
	fi
fi
AC_MSG_RESULT($acdir)
AC_SUBST(acdir)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_ISC_POSIX
AC_PROG_LIBTOOL
AM_PROG_CC_STDC

# enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE(
  debug,
  AC_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
  AC_DEFINE(APP_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
fi


# compile with disable-deprecated switches
AC_MSG_CHECKING([whether to disable deprecated glib/gtk+/gst/etc. features])
AC_ARG_ENABLE(deprecated,
	AC_HELP_STRING([--disable-deprecated],[disable deprecated glib/gtk+/gst/etc. features]),
	set_enable_deprecated="$enableval",[
		if test -f $srcdir/autogen.sh; then
			is_cvs_version=true
			set_enable_deprecated=no
		else
			set_enable_deprecated=yes
		fi
	]
)
if test "$set_enable_deprecated" != "yes"; then
	AC_MSG_RESULT(yes)
	BT_DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED"
else
	AC_MSG_RESULT(no)
	BT_DISABLE_DEPRECATED=""
fi
AC_SUBST(BT_DISABLE_DEPRECATED)


# enable coverage analysis
AC_MSG_CHECKING(whether to enable coverage analysis)
AC_ARG_ENABLE(
  coverage,
  AC_HELP_STRING([--enable-coverage],[enable coverage analysis (default=no)]),
  ,
  [enable_coverage="no"])
AC_MSG_RESULT($enable_coverage)
if test "$enable_coverage" = "yes"; then
  #AC_DEFINE(APP_COVERAGE, [1], [enable coverage analysis])
  AM_CONDITIONAL(USE_COVERAGE, true)
  COVERAGE_CFLAGS="-O0 -g -fprofile-arcs -ftest-coverage"
else
  AM_CONDITIONAL(USE_COVERAGE, false)
  COVERAGE_CFLAGS=""
fi


# Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.2.0 gthread-2.0 >= 2.2.0 gmodule-2.0 >= 2.2.0 \
  gstreamer-0.8 >= 0.8.10 gstreamer-interfaces-0.8 >= 0.8 \
)

plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
AC_SUBST(plugindir)

GST_PLUGIN_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS $BT_DISABLE_DEPRECATED"
AC_SUBST(GST_PLUGIN_CFLAGS)

GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '[_]*(gst_|Gst|GST_).*'"
AC_SUBST(GST_PLUGIN_LDFLAGS)

# forward some setting to config.h
#AC_DEFINE_UNQUOTED(GSTBT_MAJOR_VERSION, $GSTBT_MAJOR_VERSION, [BuzzMachine loader major version])
#AC_DEFINE_UNQUOTED(GSTBT_MINOR_VERSION, $GSTBT_MINOR_VERSION, [BuzzMachine loader minor version])

dnl set license and copyright notice
AC_DEFINE(GSTBT_LICENSE, "LGPL", [Buzztard gstreamer extension module license])
AC_SUBST(GSTBT_LICENSE)

dnl set license and copyright notice
AC_DEFINE(GSTBT_ORIGIN, "http://www.buzztard.org", [Buzztard gstreamer extension module origin])
AC_SUBST(GSTBT_ORIGIN)

# Substitutes
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)

AC_CONFIG_FILES( \
	libgstbuzztard.pc \
	Makefile \
	childbin/Makefile \
	propertymeta/Makefile \
	tempo/Makefile \
)
AC_OUTPUT
