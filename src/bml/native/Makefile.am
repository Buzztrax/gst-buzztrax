## $Id$

noinst_LTLIBRARIES = libgstbmln.la

# native
libgstbmln_la_SOURCES = ../gstbml.c ../gstbmlsrc.c ../gstbmltransform.c \
    ../gstbmlv.c ../utils.c
libgstbmln_la_CFLAGS = -DBML_NATIVE

INCLUDES = \
  -I@srcdir@ \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS)

