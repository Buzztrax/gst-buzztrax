# plugindir is set in configure

plugin_LTLIBRARIES = libgstaudiodelay.la

# sources used to compile this plug-in
libgstaudiodelay_la_SOURCES = audiodelay.c

# flags used to compile this plugin
libgstaudiodelay_la_CFLAGS =  -I@srcdir@ -I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libgstaudiodelay_la_LIBADD = $(GST_BASE_LIBS) \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la
libgstaudiodelay_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = audiodelay.h
