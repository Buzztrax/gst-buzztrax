dnl $Id$

dnl
dnl current  : the interface         : MAJOR + MINOR
dnl revision : of the same interface : MICRO
dnl age      : MINOR
dnl
dnl 1: when releasing a changed version : MICRO++
dnl 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
dnl 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
dnl

dnl Initialise the autoconf/automake tool
AC_INIT([gst-buzztard], [0.4.0], [buzztard-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)

GSTBT_MAJOR_VERSION=0
GSTBT_MINOR_VERSION=4
GSTBT_MICRO_VERSION=0
GSTBT_VERSION=$GSTBT_MAJOR_VERSION.$GSTBT_MINOR_VERSION.$GSTBT_MICRO_VERSION
GSTBT_VERSION_INFO=`expr $GSTBT_MAJOR_VERSION + $GSTBT_MINOR_VERSION`:$GSTBT_MICRO_VERSION:$GSTBT_MINOR_VERSION
GSTBT_VERSION_NUMBER=`expr $GSTBT_MAJOR_VERSION \* 10000 + $GSTBT_MINOR_VERSION \* 100 + $GSTBT_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
GSTBT_MAJORMINOR=$GSTBT_MAJOR_VERSION.$GSTBT_MINOR_VERSION
dnl we override it here for release candidates
dnl GSTBT_MAJORMINOR=0.10

dnl that is needed to figure the directory name, where the plugin goes to
GST_MAJORMINOR=0.10
AC_DEFINE_UNQUOTED(GST_MAJORMINOR, "$GST_MAJORMINOR", [gstreamer series])
AC_SUBST(GST_MAJORMINOR)


AC_SUBST(GSTBT_MAJOR_VERSION)
AC_SUBST(GSTBT_MINOR_VERSION)
AC_SUBST(GSTBT_MICRO_VERSION)
AC_SUBST(GSTBT_VERSION)
AC_SUBST(GSTBT_VERSION_INFO)
AC_SUBST(GSTBT_VERSION_NUMBER)
AC_SUBST(GSTBT_MAJORMINOR)

VERSION=${GSTBT_VERSION}

dnl Initialise the automake tool
AM_INIT_AUTOMAKE([check-news])

dnl m4 - everyone who develops should have aclocal installed
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

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_SED
AC_ISC_POSIX
AC_PROG_LIBTOOL
AM_PROG_CC_STDC
PKG_PROG_PKG_CONFIG

dnl enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE(
  debug,
  AC_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
  AC_DEFINE(USE_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
fi


dnl compile with disable-deprecated switches
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


dnl enable coverage analysis
AC_MSG_CHECKING(whether to enable coverage analysis)
AC_ARG_ENABLE(
  coverage,
  AC_HELP_STRING([--enable-coverage],[enable coverage analysis (default=no)]),
  ,
  [enable_coverage="no"])
AC_MSG_RESULT($enable_coverage)
if test "$enable_coverage" = "yes"; then
  COVERAGE_CFLAGS="-O0 -g -fprofile-arcs -ftest-coverage"
  COVERAGE_LIBS="-lgcov"

  AC_PATH_PROG(locv_path, [lcov], [no])
  if test "x$lcov_path" != "xno" ; then
    LCOV_VERSION=`lcov --version|head -n 1| $SED 's/^.* version //'`
    LCOV_MAJOR=`echo $LCOV_VERSION | cut -d. -f1 | $SED s/[a-zA-Z\-].*//g`
    LCOV_MINOR=`echo $LCOV_VERSION | cut -d. -f2 | $SED s/[a-zA-Z\-].*//g`
    if [[ "$LCOV_MAJOR" -lt "1" ]]; then
      AC_MSG_WARN([lcov >= 1.6 is required, you have $LCOV_VERSION])
      lcov_path="no"
    elif [[ "$LCOV_MAJOR" -eq "1" ]] && \
         [[ "$LCOV_MINOR" -lt "6" ]]; then
      AC_MSG_WARN([lcov >= 1.6 is required, you have $LCOV_VERSION])
      lcov_path="no"
    fi
  fi
else
  COVERAGE_CFLAGS=""
  COVERAGE_LIBS=""
fi
AM_CONDITIONAL(USE_LCOV, test ! x$lcov_path = xyes)


dnl valgrind integration for tests
AC_MSG_CHECKING(where Valgrind is installed on your system)
AC_ARG_WITH([valgrind],
        AC_HELP_STRING([--with-valgrind],
        [where Valgrind is installed on your system (default is no)]),
        [ac_cv_use_valgrind=$withval], [ac_cv_use_valgrind=no])dnl
AC_MSG_RESULT($ac_cv_use_valgrind)
VALGRIND=$ac_cv_use_valgrind
AC_SUBST(VALGRIND)dnl
AM_CONDITIONAL(USE_VALGRIND, test x$VALGRIND != xno)dnl

VALGRIND_CPPFLAGS=
if test $ac_cv_use_valgrind != no; then
   AC_DEFINE(HAVE_VALGRIND, 1, [Define to 1 if valgrind is to be used.])
   if test -f $ac_cv_use_valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind
   elif test -f $ac_cv_use_valgrind/include/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/include
   elif test -f $ac_cv_use_valgrind/valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/valgrind
   elif test -f $ac_cv_use_valgrind/include/valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/include/valgrind
   elif test -f $ac_cv_use_valgrind/valgrind/include/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/valgrind/include
   fi
fi
AC_SUBST(VALGRIND_CFLAGS)dnl


dnl Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.2.0 gobject-2.0  >= 2.6.0 gthread-2.0 >= 2.2.0 gmodule-2.0 >= 2.2.0 \
  gstreamer-0.10 >= 0.10.0 gstreamer-base-0.10 >= 0.10.0 \
  gstreamer-controller-0.10 >= 0.10.0 \
)

dnl GStreamer plugin build
plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
AC_SUBST(plugindir)
GST_PLUGIN_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS"
AC_SUBST(GST_PLUGIN_CFLAGS)
GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '[_]*(gst_|Gst|GST_).*' $COVERAGE_LIBS"
AC_SUBST(GST_PLUGIN_LDFLAGS)


#dnl - Check for modules already commited upstream
#ac_cflags_save="$CFLAGS"
#ac_cppflags_save="$CPPFLAGS"
#ac_ldflags_save="$LDFLAGS"
#CFLAGS="$CFLAGS $BASE_DEPS_CFLAGS"
#CPPFLAGS="$CPPFLAGS $BASE_DEPS_CFLAGS"
#LDFLAGS="$LDFLAGS $BASE_DEPS_LIBS"
#AC_CHECK_HEADERS(gst/gstpreset.h,[HAVE_GST_GSTPRESET_H="1"],[HAVE_GST_GSTPRESET_H="0"])
#AM_CONDITIONAL(BUILD_PRESET_IFACE, test "$HAVE_GST_GSTPRESET_H" = "0")
#if test "x$HAVE_GST_GSTPRESET_H" = "x1"
#then
#  PRESET_DOC_TYPES=
#else
#  PRESET_DOC_TYPES="%"
#fi
#AC_SUBST(PRESET_DOC_TYPES)
#CFLAGS="$ac_cflags_save"
#CPPFLAGS="$ac_cppflags_save"
#LDFLAGS="$ac_ldflags_save"


AC_MSG_CHECKING([gstreamer version >= 0.10.20])
if pkg-config --atleast-version=0.10.20 gstreamer-0.10; then
  HAVE_GST_GSTPRESET=1
  PRESET_DOC_TYPES="%"
  AC_MSG_RESULT(yes)
else
  HAVE_GST_GSTPRESET=0
  PRESET_DOC_TYPES=
  AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(BUILD_PRESET_IFACE, test "$HAVE_GST_GSTPRESET" = "0")
AC_SUBST(PRESET_DOC_TYPES)


dnl - Check for buzzmachineloader (bml)
PKG_CHECK_MODULES(BML, libbml >= 0.4.0 liboil-0.3 >= 0.3.8,
    HAVE_BML=1, HAVE_BML=0)
AC_SUBST(BML_CFLAGS)
AC_SUBST(BML_LIBS)
AM_CONDITIONAL(BML_SUPPORT, test "$HAVE_BML" = "1")
if test "x$HAVE_BML" = "x1"
then
  BML_DOC_TYPES=
else
  BML_DOC_TYPES="%"
fi
AC_SUBST(BML_DOC_TYPES)
dnl check for bml variant (wrapped/native)
AC_CHECK_LIB( bml, bmlw_init, [
    HAVE_BMLW=1
    AC_DEFINE(HAVE_BMLW, [1], [Define to 1 if bml has been build with wrapper]) \
  ]
)
AM_CONDITIONAL(BUILD_BMLW, test $HAVE_BMLW == "1")


dnl - Check for FluidSynth
PKG_CHECK_MODULES(FLUIDSYNTH, fluidsynth >= 1.0,
    HAVE_FLUIDSYNTH=1, HAVE_FLUIDSYNTH=0)
AC_SUBST(FLUIDSYNTH_CFLAGS)
AC_SUBST(FLUIDSYNTH_LIBS)
AM_CONDITIONAL(FLUIDSYNTH_SUPPORT, test "$HAVE_FLUIDSYNTH" = "1")
if test "x$HAVE_FLUIDSYNTH" = "x1"
then
  FLUIDSYNTH_DOC_TYPES=
else
  FLUIDSYNTH_DOC_TYPES="%"
fi
AC_SUBST(FLUIDSYNTH_DOC_TYPES)


dnl set license and copyright notice
AC_DEFINE(GST_PACKAGE_ORIGIN, "http://www.buzztard.org", [Plugin package origin])
AC_SUBST(GST_PACKAGE_ORIGIN)
AC_DEFINE(GST_PACKAGE_NAME, "gst-buzztard", [Plugin package name])
AC_SUBST(GST_PACKAGE_NAME)


dnl variable from gtkdoc-fixxref
glib_prefix="`pkg-config --variable=prefix glib-2.0`"
gst_prefix="`pkg-config --variable=prefix gstreamer-0.10`"
AC_SUBST(glib_prefix)
AC_SUBST(gst_prefix)


dnl test for availability of check unittest package (with a relative new function)
dnl @todo: if we configure without --enable-debug most tests wont work
PKG_CHECK_MODULES(CHECK, check >= 0.9.2,
  AM_CONDITIONAL(BUILD_CHECK_TESTS, true),
  AM_CONDITIONAL(BUILD_CHECK_TESTS, false)
)

dnl Check for gtk-doc.
GTK_DOC_CHECK([1.4])

dnl Substitutes
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)

# needed for preset iface
full_var=${datadir}
while true; do
    new_full_var="`eval echo $full_var`"
    if test "x$new_full_var" = "x$full_var"; then break; fi
    full_var=$new_full_var
done
AC_DEFINE_UNQUOTED(DATADIR, "$full_var", [data dir])
AC_SUBST(DATADIR)

AC_CONFIG_FILES( \
	libgstbuzztard.pc \
	Makefile \
	docs/Makefile \
	docs/reference/Makefile \
	docs/reference/gst-buzztard.types \
    libgstbuzztard/Makefile \
	src/Makefile \
	src/audiodelay/Makefile \
	src/bml/Makefile \
	src/bml/native/Makefile \
	src/bml/wrapped/Makefile \
	src/fluidsynth/Makefile \
	src/simsyn/Makefile \
	tests/Makefile
)
AC_OUTPUT
