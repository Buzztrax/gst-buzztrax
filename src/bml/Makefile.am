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

libgstbml_la_CFLAGS = -I@srcdir@ \
  -DLIBDIR=\"\${libdir}\" \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS) \
  $(BML_CFLAGS)
libgstbml_la_LIBADD = $(GST_BASE_LIBS)  $(BML_LIBS) \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	native/libgstbmln.la $(BMLW_LA)
libgstbml_la_LDFLAGS =  $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = \
  gstbmlsrc.h gstbmltransform.h \
  gstbml.h gstbmlv.h \
  plugin.h common.h utils.h

