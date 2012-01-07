if BUILD_BMLW
SUBDIRS = native wrapped .
BMLW_LA = wrapped/libgstbmlw.la
else
SUBDIRS = native .
BMLW_LA = 
endif

ORC_SOURCE=gstbmlorc
include $(top_srcdir)/orc.mak

plugin_LTLIBRARIES = libgstbml.la

libgstbml_la_SOURCES = \
  plugin.c common.c

nodist_libgstbml_la_SOURCES = $(ORC_NODIST_SOURCES)

libgstbml_la_CFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(builddir) \
  -DLIBDIR=\"\${libdir}\" \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS) \
  $(ORC_CFLAGS) \
  $(BML_CFLAGS)
libgstbml_la_LIBADD = \
	$(top_builddir)/libgstbuzztard/libgstbuzztard.la \
	native/libgstbmln.la $(BMLW_LA) \
	$(BASE_DEPS_LIBS) $(ORC_LIBS) $(BML_LIBS) $(GST_PLUGIN_LIBS) $(LIBM) 
libgstbml_la_LDFLAGS =  $(GST_PLUGIN_LDFLAGS)
libgstbml_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = \
  gstbmlsrc.h gstbmltransform.h \
  gstbml.h gstbmlv.h \
  plugin.h common.h utils.h

