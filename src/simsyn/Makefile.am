# plugindir is set in configure

plugin_LTLIBRARIES = libgstsimsyn.la

# sources used to compile this plug-in
libgstsimsyn_la_SOURCES = simsyn.c

# flags used to compile this plugin
libgstsimsyn_la_CFLAGS = -I@srcdir@ -I$(top_srcdir) \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libgstsimsyn_la_LIBADD = $(BASE_DEPS_LIBS) \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la
libgstsimsyn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = simsyn.h
