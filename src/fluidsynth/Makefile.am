# plugindir is set in configure

plugin_LTLIBRARIES = libgstfluidsynth.la

# sources used to compile this plug-in
libgstfluidsynth_la_SOURCES = gstfluidsynth.c gstfluidsynth.h

# flags used to compile this plugin
libgstfluidsynth_la_CFLAGS = -I$(srcdir) -I$(top_srcdir) \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS) \
	$(FLUIDSYNTH_CFLAGS)
libgstfluidsynth_la_LIBADD = $(BASE_DEPS_LIBS) $(FLUIDSYNTH_LIBS) \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la
libgstfluidsynth_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstfluidsynth_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstfluidsynth.h
