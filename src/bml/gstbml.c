/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:gstbml
 * @title: GstBml
 * @short_description: buzzmachine wrapper
 *
 * Wrapper for buzzmachine sound generators and effects.
 */

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

extern GstStructure *bml_meta_all;
extern GstPlugin *bml_plugin;
extern GHashTable *bml_category_by_machine_name;

static void
remove_double_def_chars (gchar * name)
{
  gchar *ptr1, *ptr2;

  ptr1 = ptr2 = name;
  // remove double '-'
  while (*ptr2) {
    if (*ptr2 == '-') {
      while (ptr2[1] == '-')
        ptr2++;
    }
    if (ptr1 != ptr2)
      *ptr1 = *ptr2;
    ptr1++;
    ptr2++;
  }
  if (ptr1 != ptr2)
    *ptr1 = '\0';
  // remove trailing '-'
  ptr1--;
  while (*ptr1 == '-')
    *ptr1++ = '\0';
}

gboolean
bml (describe_plugin (gchar * pathname, gpointer bmh))
{
  gchar *dllname;
  gint type, flags;
  gboolean res = FALSE;

  GST_INFO ("describing %p : %s", bmh, pathname);

  // use dllname to register
  // 'M3.dll' and 'M3 Pro.dll' both use the name M3 :(
  // BM_PROP_DLL_NAME instead of BM_PROP_SHORT_NAME

  g_assert (bmh);
  if (bml (get_machine_info (bmh, BM_PROP_DLL_NAME, (void *) &dllname)) &&
      bml (get_machine_info (bmh, BM_PROP_TYPE, (void *) &type)) &&
      bml (get_machine_info (bmh, BM_PROP_FLAGS, (void *) &flags))
      ) {
    gchar *name, *filename, *ext;
    gchar *element_type_name, *voice_type_name = NULL;
    gchar *help_filename, *preset_filename;
    gchar *data_pathname = NULL;
    gboolean type_names_ok = TRUE;
    GstStructure *bml_meta;
    GError *error = NULL;
    if ((filename = strrchr (dllname, '/'))) {
      filename++;
    } else {
      filename = dllname;
    }
    GST_INFO ("  dll-name: '%s', flags are: %x", filename, flags);
    if (flags & 0xFC) {
      // we only support, MONO_TO_STEREO(1), PLAYS_WAVES(2) yet
      GST_WARNING ("  machine is not yet fully supported, flags: %x", flags);
    }
    // get basename
    ext = g_strrstr (filename, ".");
    *ext = '\0';                // temporarily terminate
    if (!(name =
            g_convert_with_fallback (filename, -1, "ASCII", "WINDOWS-1252", "-",
                NULL, NULL, &error))) {
      GST_WARNING ("trouble converting filename: '%s': %s", filename,
          error->message);
      g_error_free (error);
      error = NULL;
    }
    if (name) {
      g_strstrip (name);
    }
    *ext = '.';                 // restore
    GST_INFO ("  name is '%s'", name);

    // construct the type name
    element_type_name = g_strdup_printf ("bml-%s", name);
    if (bml (gstbml_is_polyphonic (bmh))) {
      voice_type_name = g_strdup_printf ("bmlv-%s", name);
    }
    g_free (name);
    // if it's already registered, skip (mean e.g. native element has been registered)
    g_strcanon (element_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+",
        '-');
    remove_double_def_chars (element_type_name);
    if (g_type_from_name (element_type_name)) {
      GST_WARNING ("already registered type : \"%s\"", element_type_name);
      g_free (element_type_name);
      element_type_name = NULL;
      type_names_ok = FALSE;
    }
    if (voice_type_name) {
      g_strcanon (voice_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+",
          '-');
      remove_double_def_chars (voice_type_name);
      if (g_type_from_name (voice_type_name)) {
        GST_WARNING ("already registered type : \"%s\"", voice_type_name);
        g_free (voice_type_name);
        voice_type_name = NULL;
        type_names_ok = FALSE;
      }
    }
    if (!type_names_ok) {
      g_free (element_type_name);
      g_free (voice_type_name);
      return (TRUE);
    }
    // create the metadata store
#ifdef BML_WRAPPED
    bml_meta = gst_structure_new_empty ("bmlw");
#else
    bml_meta = gst_structure_new_empty ("bmln");
#endif

    // use right path for emulated and native machines
#ifdef BML_WRAPPED
    data_pathname = g_strdup (pathname);
#else
    {
      gchar *pos;
      // replace lib in path-name wth share (should we use DATADIR/???)
      pos = strstr (pathname, "/lib/");
      if (pos) {
        *pos = '\0';
        data_pathname = g_strdup_printf ("%s/share/%s", pathname, &pos[5]);
        *pos = '/';
      } else {
        GST_WARNING ("failed to map plugin lib path '%s' to a datadir",
            pathname);
        data_pathname = g_strdup (pathname);
      }
    }
#endif
    // data files have same basename as plugin, but different extension
    ext = strrchr (data_pathname, '.');
    *ext = '\0';
    // try various help uris
    help_filename = g_strdup_printf ("%s.html", data_pathname);
    if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
      GST_INFO ("no user docs at '%s'", help_filename);
      g_free (help_filename);
      help_filename = g_strdup_printf ("%s.htm", data_pathname);
      if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
        GST_INFO ("no user docs at '%s'", help_filename);
        g_free (help_filename);
        help_filename = g_strdup_printf ("%s.txt", data_pathname);
        if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
          GST_INFO ("no user docs at '%s'", help_filename);
          g_free (help_filename);
          help_filename = NULL;
        }
      }
    }
    // generate preset name
    // we need a fallback to g_get_user_config_dir()/??? in any case
    preset_filename = g_strdup_printf ("%s.prs", data_pathname);
    g_free (data_pathname);

    // store help uri
    if (help_filename) {
      gchar *help_uri;
      help_uri = g_strdup_printf ("file://%s", help_filename);
      GST_INFO ("machine %p has user docs at '%s'", bmh, help_uri);
      g_free (help_filename);
      gst_structure_set (bml_meta, "help-filename", G_TYPE_STRING, help_uri,
          NULL);
    }
    // store preset path
    if (preset_filename) {
      GST_INFO ("machine %p preset path '%s'", bmh, preset_filename);
      gst_structure_set (bml_meta, "preset-filename", G_TYPE_STRING,
          preset_filename, NULL);
    }
    if (voice_type_name) {
      gst_structure_set (bml_meta, "voice-type-name", G_TYPE_STRING,
          voice_type_name, NULL);
    }

    gst_structure_set (bml_meta,
        "plugin-filename", G_TYPE_STRING, pathname,
        "machine-type", G_TYPE_INT, type,
        "element-type-name", G_TYPE_STRING, element_type_name, NULL);

    {
      gchar *str, *extra_categories;

      /* this does not yet match all machines, e.g. Elak SVF
       * we could try ("%s %s",author,longname) in addition? */
      bml (get_machine_info (bmh, BM_PROP_NAME, (void *) &str));
      str = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
      extra_categories =
          g_hash_table_lookup (bml_category_by_machine_name, str);
      g_free (str);
      if (extra_categories) {
        gst_structure_set (bml_meta, "categories", G_TYPE_STRING,
            extra_categories, NULL);
      }
    }

    // store metadata cache
    {
      GValue value = { 0, };

      GST_INFO ("caching data: type_name=%s, file_name=%s", element_type_name,
          pathname);
      g_value_init (&value, GST_TYPE_STRUCTURE);
      g_value_set_boxed (&value, bml_meta);
      gst_structure_set_value (bml_meta_all, element_type_name, &value);
      g_value_unset (&value);
    }
    res = TRUE;
  }
  return (res);
}
