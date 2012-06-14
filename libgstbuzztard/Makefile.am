noinst_HEADERS = compat.h

lib_LTLIBRARIES = libgstbuzztard.la

libgstbuzztard_la_SOURCES = \
	audiosynth.c \	
	childbin.c \
	envelope.c \
	help.c \
	musicenums.c \
	toneconversion.c \
	propertymeta.c \
	tempo.c 

libgstbuzztard_la_CFLAGS = \
	-I$(srcdir) \
	$(GST_LIB_CFLAGS) \
	$(BASE_DEPS_CFLAGS)

libgstbuzztardincludedir = $(includedir)/libgstbuzztard
libgstbuzztardinclude_HEADERS = \
	audiosynth.h \
	childbin.h \
	envelope.h \
	help.h \
	musicenums.h \
	toneconversion.h \
	propertymeta.h \
	tempo.h

libgstbuzztard_la_LIBADD = $(BASE_DEPS_LIBS)
libgstbuzztard_la_LDFLAGS = -version-info @GSTBT_VERSION_INFO@ \
    $(GST_LIB_LDFLAGS)

include $(top_srcdir)/common.mak
