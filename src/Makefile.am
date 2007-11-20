if FLUIDSYNTH_SUPPORT
FLUIDSYNTH = fluidsynth
else
FLUIDSYNTH =
endif

if BUILD_PRESET_IFACE
PRESET_IFACE_MOD = preset
PRESET_FFACE_LIB = preset/libgstpreset.la
else
PRESET_IFACE_MOD =
PRESET_FFACE_LIB =
endif

SUBDIRS = childbin envelope help musicenums note2frequency \
    $(PRESET_IFACE_MOD) propertymeta tempo . \
    audiodelay $(FLUIDSYNTH) simsyn

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES =

libgstbuzztard_la_LIBADD = \
    childbin/libgstchildbin.la \
    musicenums/libbtmusicenums.la \
    envelope/libgstenvelope.la \
    help/libgsthelp.la \
    note2frequency/libgstnote2frequency.la \
    $(PRESET_IFACE_LIB) \
    propertymeta/libgstpropertymeta.la \
    tempo/libgsttempo.la \
    $(BASE_DEPS_LIBS)

libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@
