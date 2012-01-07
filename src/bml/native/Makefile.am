noinst_LTLIBRARIES = libgstbmln.la

# native
libgstbmln_la_SOURCES = ../gstbml.c ../gstbmlsrc.c ../gstbmltransform.c \
    ../gstbmlv.c ../utils.c
libgstbmln_la_CFLAGS = -DBML_NATIVE \
  -I$(srcdir) -I$(srcdir)/.. -I$(builddir) -I$(builddir)/.. \
  $(GST_PLUGIN_CFLAGS) \
  $(BASE_DEPS_CFLAGS) \
  $(ORC_CFLAGS) \
  $(BML_CFLAGS)

