SUBDIRS = childbin envelope help note2frequency preset propertymeta tempo . \
    audiodelay simsyn

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    envelope/libgstenvelope.la \
    help/libgsthelp.la \
    note2frequency/libgstnote2frequency.la \
    preset/libgstpreset.la \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@
