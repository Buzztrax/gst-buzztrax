dnl Initialise autoconf/automake/libtool
AC_PREREQ(2.61)
AC_INIT([gst-buzztrax],[0.8.0],[buzztrax-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall check-news std-options])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)
LT_PREREQ([2.2])
LT_INIT

# Support silent build rules, requires at least automake-1.11. Disable by
# either passing --disable-silent-rules to configure or passing V=1 to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


dnl dependencies
REQ_GLIB=2.32.0
REQ_GST=1.1.0


dnl
dnl current  : the interface         : MAJOR + MINOR
dnl revision : of the same interface : MICRO
dnl age      : MINOR
dnl
dnl 1: when releasing a changed version : MICRO++
dnl 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
dnl 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
dnl
GSTBT_MAJOR_VERSION=1
GSTBT_MINOR_VERSION=0
GSTBT_MICRO_VERSION=0
GSTBT_VERSION_INFO=`expr $GSTBT_MAJOR_VERSION + $GSTBT_MINOR_VERSION`:$GSTBT_MICRO_VERSION:$GSTBT_MINOR_VERSION

dnl that is needed to figure the directory name, where the plugin goes to
GST_MAJORMINOR=1.0
AC_DEFINE_UNQUOTED(GST_MAJORMINOR, "$GST_MAJORMINOR", [gstreamer series])
AC_SUBST(GST_MAJORMINOR)

AC_SUBST(GSTBT_MAJOR_VERSION)
AC_SUBST(GSTBT_MINOR_VERSION)
AC_SUBST(GSTBT_MICRO_VERSION)
AC_SUBST(GSTBT_VERSION_INFO)


dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_SED
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl common switches for all buzztrax modules

dnl enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug=no])
AC_MSG_RESULT($enable_debug)
if test "x$enable_debug" = "xyes"; then
  AC_DEFINE(USE_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
  DEBUG_LDFLAGS="-Wl,--warn-common"
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
  DEBUG_LDFLAGS=""
fi


dnl compile with disable-deprecated switches
AC_MSG_CHECKING([whether to disable deprecated glib/gtk+/gst/etc. features])
AC_ARG_ENABLE([deprecated],
	AS_HELP_STRING([--disable-deprecated],[disable deprecated glib/gtk+/gst/etc. features]),
	,
	[
		if test -f $ac_top_srcdir/autogen.sh; then
			enable_deprecated=no
		else
			enable_deprecated=yes
		fi
	]
)
AC_MSG_RESULT($enable_deprecated)
if test "x$enable_deprecated" == "xno"; then
	BT_DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED"
else
	BT_DISABLE_DEPRECATED=""
fi
AC_SUBST(BT_DISABLE_DEPRECATED)


dnl enable coverage analysis
AC_MSG_CHECKING(whether to enable coverage analysis)
AC_ARG_ENABLE([coverage],
  AS_HELP_STRING([--enable-coverage],[enable coverage analysis (default=no)]),
  ,
  [enable_coverage=no])
AC_MSG_RESULT($enable_coverage)
if test "x$enable_coverage" = "xyes"; then
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
AC_PATH_PROG(valgrind_path, [valgrind], [no])
if test "x$valgrind_path" != "xno"; then
  valgrind_path=`dirname $valgrind_path`
  VALGRIND=`dirname $valgrind_path`
  have_valgrind="yes"
else
  VALGRIND="no"
  have_valgrind="no"
fi
AC_SUBST(VALGRIND)
AM_CONDITIONAL(USE_VALGRIND, test x$VALGRIND != xno)


dnl Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= $REQ_GLIB gobject-2.0  >= $REQ_GLIB gthread-2.0 >= $REQ_GLIB \
  gmodule-2.0 >= $REQ_GLIB \
  gstreamer-1.0 >= $REQ_GST gstreamer-base-1.0 >= $REQ_GST \
  gstreamer-controller-1.0 >= $REQ_GST \
  gstreamer-plugins-base-1.0 >= $REQ_GST \
  gstreamer-audio-1.0 >= $REQ_GST \
)
GST_LIB_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS"
AC_SUBST(GST_LIB_CFLAGS)
GST_LIB_LDFLAGS="$DEBUG_LDFLAGS -export-symbols-regex \^[_]*\(gstbt_\|GstBt\|GSTBT_\|gst_\|Gst\|GST_\).*"
AC_SUBST(GST_LIB_LDFLAGS)

dnl GStreamer plugin build
plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
AC_SUBST(plugindir)
GST_PLUGIN_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS"
AC_SUBST(GST_PLUGIN_CFLAGS)
GST_PLUGIN_LDFLAGS="$DEBUG_LDFLAGS -module -avoid-version -export-symbols-regex '[_]*(gst_|bt_|Bt|BT_).*'"
AC_SUBST(GST_PLUGIN_LDFLAGS)
GST_PLUGIN_LIBS="$COVERAGE_LIBS"
AC_SUBST(GST_PLUGIN_LIBS)


#dnl - Check for parts not yet in upstream
#ac_cflags_save="$CFLAGS"
#ac_cppflags_save="$CPPFLAGS"
#ac_ldflags_save="$LDFLAGS"
#CFLAGS="$CFLAGS $BASE_DEPS_CFLAGS"
#CPPFLAGS="$CPPFLAGS $BASE_DEPS_CFLAGS"
#LDFLAGS="$LDFLAGS $BASE_DEPS_LIBS"
#AC_CHECK_HEADERS(gst/gstbufferpool.h,[HAVE_GST_BUFFERPOOL_H="1"],[HAVE_GST_BUFFERPOOL_H="0"])
#AM_CONDITIONAL(BUILD_BUFFERPOOL, test "$HAVE_GST_BUFFERPOOL_H" = "0")
#CFLAGS="$ac_cflags_save"
#CPPFLAGS="$ac_cppflags_save"
#LDFLAGS="$ac_ldflags_save"


dnl - Check for buzzmachineloader (bml)
PKG_CHECK_MODULES(BML, libbml >= 0.8.0, have_bml=yes, have_bml=no)
AC_SUBST(BML_CFLAGS)
AC_SUBST(BML_LIBS)
AM_CONDITIONAL(BML_SUPPORT, test "x$have_bml" = "xyes")
if test "x$have_bml" = "xyes"
then
  BML_DOC_TYPES=
else
  BML_DOC_TYPES="%"
fi
AC_SUBST(BML_DOC_TYPES)

dnl check for bml variant (wrapped/native)
ac_cflags_save="$CFLAGS"
ac_ldflags_save="$LDFLAGS"
CFLAGS="$CFLAGS $BML_CFLAGS"
LDFLAGS="$LDFLAGS $BML_LIBS"
AC_CHECK_LIB( bml, bmlw_init, [
    have_bmlw=yes
    AC_DEFINE(HAVE_BMLW, [1], [Define to 1 if bml has been build with wrapper])
    bml_types="native"
  ],[
    have_bmlw=no
    bml_types="native and wrapped"
  ]
)
AM_CONDITIONAL(BUILD_BMLW, test "x$have_bmlw" = "xyes")
CFLAGS="$ac_cflags_save"
LDFLAGS="$ac_ldflags_save"

dnl check for oil runtime compiler
ORC_CHECK([0.4.6])

dnl check for FluidSynth
PKG_CHECK_MODULES(FLUIDSYNTH, fluidsynth >= 1.0,
    have_fluidsynth=yes, have_fluidsynth=no)
AC_SUBST(FLUIDSYNTH_CFLAGS)
AC_SUBST(FLUIDSYNTH_LIBS)
AM_CONDITIONAL(FLUIDSYNTH_SUPPORT, test "x$have_fluidsynth" = "xyes")
if test "x$have_fluidsynth" = "xyes"
then
  FLUIDSYNTH_DOC_TYPES=
else
  FLUIDSYNTH_DOC_TYPES="%"
fi
AC_SUBST(FLUIDSYNTH_DOC_TYPES)


dnl set license and copyright notice
AC_DEFINE(GST_PACKAGE_ORIGIN, "http://www.buzztrax.org", [Plugin package origin])
AC_SUBST(GST_PACKAGE_ORIGIN)
AC_DEFINE(GST_PACKAGE_NAME, "gst-buzztrax", [Plugin package name])
AC_SUBST(GST_PACKAGE_NAME)


dnl variable from gtkdoc-fixxref
glib_prefix="`pkg-config --variable=prefix glib-2.0`"
gst_prefix="`pkg-config --variable=prefix gstreamer-1.0`"
AC_SUBST(glib_prefix)
AC_SUBST(gst_prefix)


dnl test for availability of check unittest package (with a relative new function)
dnl @todo: if we configure without --enable-debug most tests wont work
PKG_CHECK_MODULES(CHECK, check >= 0.9.2,
  [
    have_check=yes
    AM_CONDITIONAL(BUILD_CHECK_TESTS, true)
  ],[
    have_check=no
    AM_CONDITIONAL(BUILD_CHECK_TESTS, false)
  ]
)

dnl check for libraries
AC_CHECK_LIBM
AC_SUBST(LIBM)


dnl Check for gtk-doc.
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.10],[--flavour no-tmpl])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])


dnl Substitutes
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)


AC_CONFIG_FILES(Makefile \
	libgstbuzztrax.pc \
	docs/reference/Makefile \
	docs/reference/gst-buzztrax.types
)
AC_OUTPUT

echo "
Configuration
	Version                    : ${PACKAGE_VERSION}
	Source code location       : ${srcdir}
	Prefix                     : ${prefix}
	Compiler                   : ${CC}

	Documentation (API)        : ${enable_gtk_doc}
	Buzzmachine support        : ${have_bml} (${bml_types})
	FluidSynth support         : ${have_fluidsynth}

	Debug                      : ${enable_debug}
	Coverage profiling         : ${enable_coverage}
	Use of deprecated API      : ${enable_deprecated}
	Check based unit tests     : ${have_check}
	Use valgrind on the tests  : ${have_valgrind}
"

if test "x${have_bml}" = "xno" -o "x${have_fluidsynth}" = "xno" ; then
echo "
Some features are not built. If you like to have them built, please check that
you have the required -devel packages installed and that they can be found in
your PKG_CONFIG_PATH ($PKG_CONFIG_PATH).
"
fi

if test ! `echo "$PKG_CONFIG_PATH:/usr" | tr \: \\n | grep $prefix` ; then
echo "
You are installing buzztrax libraries into a location that is not yet known to
system. In order for other buzztrax modules to find them and the application to
work with all features please register the location (see README for details):
export LD_LIBRARY_PATH=$prefix/lib:\$LD_LIBRARY_PATH
export DEVHELP_SEARCH_PATH="\$DEVHELP_SEARCH_PATH:$prefix/share/gtk-doc/html"
export PKG_CONFIG_PATH="\$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"
export GST_PLUGIN_PATH_1_0="\$GST_PLUGIN_PATH:$prefix/lib/gstreamer-1.0"
"
fi

ORC_OUTPUT

