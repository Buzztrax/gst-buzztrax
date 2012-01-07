noinst_LTLIBRARIES = libgstbmlw.la

# wrapped
libgstbmlw_la_SOURCES = ../gstbml.c ../gstbmlsrc.c ../gstbmltransform.c \
    ../gstbmlv.c ../utils.c
libgstbmlw_la_CFLAGS = -DBML_WRAPPED \
  -I$(srcdir) -I$(srcdir)/.. -I$(builddir) -I$(builddir)/.. \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS) \
  $(ORC_CFLAGS) \
  $(BML_CFLAGS)

