plugin_LTLIBRARIES = libgstsimsyn.la

libgstsimsyn_la_SOURCES = simsyn.c
libgstsimsyn_la_CFLAGS = $(GST_CFLAGS) $(GST_CONTROLLER_CFLAGS)
libgstsimsyn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsimsyn_la_LIBADD = $(GST_BASE_LIBS) $(GST_CONTROLLER_LIBS) \
	$(top_builddir)/src/libgstbuzztard.la

noinst_HEADERS = simsyn.h

INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
