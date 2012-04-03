# plugindir is set in configure

plugin_LTLIBRARIES = libgstfluidsynth.la

# sources used to compile this plug-in
libgstfluidsynth_la_SOURCES = gstfluidsynth.c gstfluidsynth.h

# flags used to compile this plugin
libgstfluidsynth_la_CFLAGS = -I$(srcdir) -I$(top_srcdir) \
  -DDATADIR=\"$(datadir)\" \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS) \
	$(FLUIDSYNTH_CFLAGS)
libgstfluidsynth_la_LIBADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la  \
	$(BASE_DEPS_LIBS) $(FLUIDSYNTH_LIBS) $(GST_PLUGIN_LIBS)
libgstfluidsynth_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstfluidsynth_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstfluidsynth.h

include $(top_srcdir)/common.mak
