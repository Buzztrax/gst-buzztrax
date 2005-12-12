SUBDIRS = childbin propertymeta tempo docs

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgstbuzztard.pc

EXTRA_DIST = \
	libgstbuzztard.pc.in
