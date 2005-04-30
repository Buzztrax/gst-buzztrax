# $Id: configure.ac,v 1.1.1.1 2005-04-30 13:25:45 ensonic Exp $

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
AC_INIT([gstbml], [0.0.1], [buzztard-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)

GSTBML_MAJOR_VERSION=0
GSTBML_MINOR_VERSION=0
GSTBML_MICRO_VERSION=1
GSTBML_VERSION=$GSTBML_MAJOR_VERSION.$GSTBML_MINOR_VERSION.$GSTBML_MICRO_VERSION
GSTBML_VERSION_INFO=`expr $GSTBML_MAJOR_VERSION + $GSTBML_MINOR_VERSION`:$GSTBML_MICRO_VERSION:$GSTBML_MINOR_VERSION
GSTBML_VERSION_NUMBER=`expr $GSTBML_MAJOR_VERSION \* 10000 + $GSTBML_MINOR_VERSION \* 100 + $GSTBML_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
GSTBML_MAJORMINOR=$GSTBML_MAJOR_VERSION.$GSTBML_MINOR_VERSION
dnl we override it here for release candidates
#GSTBML_MAJORMINOR=0.8

dnl that is needed to figure the directory name, where the plugin goes to
GST_MAJORMINOR=0.8
AC_SUBST(GST_MAJORMINOR)

AC_SUBST(GSTBML_MAJOR_VERSION)
AC_SUBST(GSTBML_MINOR_VERSION)
AC_SUBST(GSTBML_MICRO_VERSION)
AC_SUBST(GSTBML_VERSION)
AC_SUBST(GSTBML_VERSION_INFO)
AC_SUBST(GSTBML_VERSION_NUMBER)
AC_SUBST(GSTBML_MAJORMINOR)

VERSION=${GSTBML_VERSION}

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

# Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.2.0 gthread-2.0 >= 2.2.0 gmodule-2.0 >= 2.2.0 \
  gstreamer-0.8 >= 0.8 gstreamer-interfaces-0.8 >= 0.8 \
	gstreamer-control-0.8 >= 0.8 \
	gstreamer-controller >= 0.0.1 \
  libbml >= 0.1 \
	)

plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
AC_SUBST(plugindir)

GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '[_]*(gst_|Gst|GST_).*'"
AC_SUBST(GST_PLUGIN_LDFLAGS)

# forward some setting to config.h
#AC_DEFINE_UNQUOTED(GSTBML_MAJOR_VERSION, $GSTBML_MAJOR_VERSION, [BuzzMachine loader major version])
#AC_DEFINE_UNQUOTED(GSTBML_MINOR_VERSION, $GSTBML_MINOR_VERSION, [BuzzMachine loader minor version])

dnl set license and copyright notice
AC_DEFINE(GSTBML_LICENSE, "LGPL", [BuzzMachine loader license])
AC_SUBST(GSTBML_LICENSE)

dnl set license and copyright notice
AC_DEFINE(GSTBML_ORIGIN, "http://www.buzztard.org", [BuzzMachine loader origin])
AC_SUBST(GSTBML_ORIGIN)

# Substitutes
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
