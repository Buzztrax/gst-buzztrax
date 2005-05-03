SUBDIRS = childbin

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
        childbin/libgstchildbin.la \
        $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgstbuzztard.pc

EXTRA_DIST = \
	libgstbuzztard.pc.in
