if FLUIDSYNTH_SUPPORT
FLUIDSYNTH = fluidsynth
else
FLUIDSYNTH =
endif

SUBDIRS = childbin envelope help musicenums note2frequency preset propertymeta tempo . \
    audiodelay $(FLUIDSYNTH) simsyn

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    musicenums/libbtmusicenums.la \
    envelope/libgstenvelope.la \
    help/libgsthelp.la \
    note2frequency/libgstnote2frequency.la \
    preset/libgstpreset.la \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@
