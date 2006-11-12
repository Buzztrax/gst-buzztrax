libgsthelpincludedir = \
	$(includedir)/gstreamer-@GST_MAJORMINOR@/gst/help

libgsthelpinclude_HEADERS = help.h

noinst_LTLIBRARIES = libgsthelp.la
libgsthelp_la_SOURCES = help.c

INCLUDES = \
	-I@srcdir@ \
	$(BASE_DEPS_CFLAGS)
