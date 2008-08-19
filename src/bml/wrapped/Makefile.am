## $Id$

noinst_LTLIBRARIES = libgstbmlw.la

# wrapped
libgstbmlw_la_SOURCES = ../gstbml.c ../gstbmlsrc.c ../gstbmltransform.c \
    ../gstbmlv.c ../utils.c
libgstbmlw_la_CFLAGS = -DBML_WRAPPED

INCLUDES = \
  -I@srcdir@ \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS)

