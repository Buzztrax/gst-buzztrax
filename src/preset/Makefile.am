libgstpresetincludedir = \
	$(includedir)/gstreamer-@GST_MAJORMINOR@/gst/preset

libgstpresetinclude_HEADERS = preset.h

noinst_LTLIBRARIES = libgstpreset.la
libgstpreset_la_SOURCES = preset.c

INCLUDES = -I@srcdir@ -I$(top_srcdir)/src \
	$(BASE_DEPS_CFLAGS)
