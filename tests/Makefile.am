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

EXTRA_DIST = gtkdoccomplete.sh

# make check           -- run all checks
# make (test).check    -- run the given check once
#	make (test).check BT_CHECKS="test5*" -- run the given testcase(s) only
%.check: %
	@$(TESTS_ENVIRONMENT)	\
	./$*

# make (test).gdb      -- start up gdb for the given test
%.gdb: %
	@CK_FORK=no				\
	$(TESTS_ENVIRONMENT)	\
	gdb ./$*

# make (test).refdbg   -- start up refdbg for the given test
%.refdbg: %
	@CK_FORK=no		\
	$(TESTS_ENVIRONMENT)	\
	refdbg -c "btnum=20 ; logobjs=0 ; $(REFDBG_RULE)" \
	./$*

if USE_VALGRIND
VALPREFIX = \
	CK_FORK=no CK_DEFAULT_TIMEOUT=500 \
	LANG=C XDG_CACHE_HOME=$(abs_builddir) \
	G_SLICE=always-malloc G_DEBUG=gc-friendly \
	GLIBCPP_FORCE_NEW=1 GLIBCXX_FORCE_NEW=1
VALDEFAULT = @VALGRIND@/bin/valgrind
VALSUPP = @VALGRIND@/lib/valgrind
VALSUPPRESSIONDEF = --suppressions=$(VALSUPP)/default.supp
VALSUPPRESSIONOWN = --suppressions=$(top_builddir)/buzztard.supp \
	--suppressions=$(top_builddir)/gst.supp \
	--suppressions=$(top_builddir)/gtk.supp
VALSUPPRESSION = $(VALSUPPRESSIONDEF) $(VALSUPPRESSIONOWN)
VALOPTIONS = --trace-children=yes --num-callers=30 --read-var-info=yes \
  --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes
VALCMD = $(VALPREFIX) $(VALDEFAULT) $(VALOPTIONS) $(VALSUPPRESSION)

## for file in /tmp/bt_core.valgrind.pid*; do grep -q "ERROR SUMMARY: 0 errors from 0 contexts" $file ; if test $? == 0; then rm $file; fi; done
## grep -H "ERROR SUMMARY: " /tmp/bt_core.valgrind.pid* | uniq

# make valgrind        -- valgrind all tests
valgrind:: $(TESTS_BIN)
	for i in $^; do \
	  rm -rf /tmp/$$i.valgrind.pid*; \
	  $(VALCMD) --log-file=/tmp/$$i_valgrind.pid%p ./$$i; \
	done

# make (test).valgrind -- valgrind the given test"
%.valgrind: %
	rm -rf /tmp/$*.valgrind.pid*; \
	$(VALCMD) --log-file=/tmp/$*.valgrind.pid%p ./$*;
endif

include $(top_srcdir)/common.mak
