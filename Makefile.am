SUBDIRS = childbin

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
        childbin/libgstchildbin.la \
        $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -version-info @GST_PLUGINS_LIBVERSION@
