SUBDIRS = childbin note2frequency propertymeta simsyn tempo

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    note2frequency/libgstnote2frequency.la \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@
