## $Id$

if BUILD_CHECK_TESTS
  TESTS_BIN = gst_buzztard
else
  TESTS_BIN =
endif

TESTS = \
	$(TESTS_BIN) \
	gtkdoccomplete.sh

# prevent endless loops
TESTS_ENVIRONMENT = ulimit -St20; \
	$(LIBTOOL) --mode=execute

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

gst_buzztard_LDADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	$(BASE_DEPS_LIBS) -lcheck
gst_buzztard_SOURCES= m-gst-buzztard.c m-gst-buzztard.h \
	s-gst-note2frequency.c e-gst-note2frequency.c t-gst-note2frequency.c
	
INCLUDES = \
	-I@srcdir@ -I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"gst-buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	@VALGRIND_CFLAGS@
endif

EXTRA_DIST = gtkdoccomplete.sh

