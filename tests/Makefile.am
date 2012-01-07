if BUILD_CHECK_TESTS
  TESTS_BIN = gst_buzztard
else
  TESTS_BIN =
endif

TESTS = \
	$(TESTS_BIN)

# prevent endless loops, this requires bash, see idea in bt-check.c
#   ulimit -St20;
# 
TESTS_ENVIRONMENT = \
	CK_DEFAULT_TIMEOUT=20 \
	LANG=C \
	$(LIBTOOL) --mode=execute

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

gst_buzztard_LDADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	$(BASE_DEPS_LIBS) -lcheck
gst_buzztard_SOURCES= m-gst-buzztard.c m-gst-buzztard.h \
	s-gst-note2frequency.c e-gst-note2frequency.c t-gst-note2frequency.c
gst_buzztard_CFLAGS = \
	-I$(srcdir) -I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"gst-buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	@VALGRIND_CFLAGS@

endif

# run any given test by running make <test>.check
%.check: %
	@$(TESTS_ENVIRONMENT)	\
	./$*

# gdb any given test by running make <test>.gdb
%.gdb: %
	CK_FORK=no				\
	CK_DEFAULT_TIMEOUT=20	\
	$(LIBTOOL) --mode=execute	\
	gdb ./$*

EXTRA_DIST = gtkdoccomplete.sh

help::
	@echo "make check                   -- run all checks"
	@echo "make (test).check            -- run the given check once"
	@echo
	@echo "make (test).gdb              -- start up gdb for the given test"

