SUBDIRS = childbin envelope note2frequency propertymeta tempo . audiodelay simsyn

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    envelope/libgstenvelope.la \
    note2frequency/libgstnote2frequency.la \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@
