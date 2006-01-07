## $Id: Makefile.am,v 1.1 2006-01-07 16:50:05 ensonic Exp $

if BUILD_CHECK_TESTS
  TESTS_BIN = gst_buzztard
else
  TESTS_BIN =
endif

TESTS = \
  $(TESTS_BIN) \
  gtkdoccomplete.sh

# prevent endless loops
TESTS_ENVIRONMENT = ulimit -St20;

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

gst_buzztard_LDADD = \
	$(top_builddir)/src/libgstbuzztard.la \
    $(BASE_DEPS_LIBS) -lcheck
gst_buzztard_SOURCES= m-gst-buzztard.c m-gst-buzztard.h \
	s-gst-note2frequency.c e-gst-note2frequency.c
	
INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src \
	-DG_LOG_DOMAIN=\"gst-buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	@VALGRIND_CFLAGS@
endif
