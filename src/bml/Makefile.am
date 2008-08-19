## $Id$

if BUILD_BMLW
SUBDIRS = native wrapped .
BMLW_LA = wrapped/libgstbmlw.la
else
SUBDIRS = native .
BMLW_LA = 
endif

plugin_LTLIBRARIES = libgstbml.la

libgstbml_la_SOURCES = \
  plugin.c common.c
libgstbml_la_LIBADD = $(BASE_DEPS_LIBS) native/libgstbmln.la $(BMLW_LA)
libgstbml_la_LDFLAGS =  $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = \
  gstbmlsrc.h gstbmltransform.h \
  gstbml.h gstbmlv.h \
  plugin.h common.h utils.h

INCLUDES = \
  -I@srcdir@ \
  -DLIBDIR=\"\${libdir}\" \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS)

