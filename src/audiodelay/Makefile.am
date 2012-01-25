# plugindir is set in configure

plugin_LTLIBRARIES = libgstaudiodelay.la

# sources used to compile this plug-in
libgstaudiodelay_la_SOURCES = audiodelay.c

# flags used to compile this plugin
libgstaudiodelay_la_CFLAGS =  -I$(srcdir) -I$(top_srcdir) \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libgstaudiodelay_la_LIBADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	$(BASE_DEPS_LIBS) $(GST_PLUGIN_LIBS)
libgstaudiodelay_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstaudiodelay_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = audiodelay.h

include $(top_srcdir)/common.mak
