if BUILD_PRESET_IFACE
PRESET_C = preset.c
PRESET_H = preset.h
else
PRESET_C =
PRESET_H =
endif

if BUILD_BUFFERPOOL
BUFFERPOOL_C = gstbufferpool.c gstpooledbuffer.c gstlfqueue.c
BUFFERPOOL_H = gstbufferpool.h gstpooledbuffer.h gstlfqueue.h
else
BUFFERPOOL_C =
BUFFERPOOL_H =
endif

noinst_HEADERS = compat.h

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES = \
	childbin.c \
	envelope.c \
	help.c \
	musicenums.c \
	toneconversion.c \
	$(PRESET_C) \
	propertymeta.c \
	tempo.c \
	$(BUFFERPOOL_C)

libgstbuzztard_la_CFLAGS = \
	-I$(srcdir) \
	$(BASE_DEPS_CFLAGS)

libgstbuzztardincludedir = $(includedir)/libgstbuzztard
libgstbuzztardinclude_HEADERS = \
	childbin.h \
	envelope.h \
	help.h \
	musicenums.h \
	toneconversion.h \
	$(PRESET_H) \
	propertymeta.h \
	tempo.h \
	$(BUFFERPOOL_H)

libgstbuzztard_la_LIBADD = $(BASE_DEPS_LIBS)
libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@

