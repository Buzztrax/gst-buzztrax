# plugindir is set in configure

plugin_LTLIBRARIES = libgstsimsyn.la

# sources used to compile this plug-in
libgstsimsyn_la_SOURCES = simsyn.c

# flags used to compile this plugin
libgstsimsyn_la_CFLAGS = -I$(srcdir) -I$(top_srcdir) \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libgstsimsyn_la_LIBADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	$(GST_BASE_LIBS) $(GST_PLUGIN_LIBS)
libgstsimsyn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsimsyn_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = simsyn.h
