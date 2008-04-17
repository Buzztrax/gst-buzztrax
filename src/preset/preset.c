/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * preset.c: helper interface for element presets
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:gstpreset
 * @short_description: helper interface for element presets
 *
 * This interface offers methods to query and manipulate parameter preset sets.
 * A preset is a bunch of property settings, together with meta data and a name.
 * The name of a preset serves as key for subsequent method calls to manipulate
 * single presets.
 * All instances of one type will share the list of presets. The list is created
 * on demand, if presets are not used, the list is not created.
 *
 * The interface comes with a default implementation that serves most plugins.
 * Wrapper plugins will override most methods to implement support for the
 * native preset format of those wrapped plugins.
 * One method that is useful to be overridden is gst_preset_get_property_names().
 * With that one can control which properties are saved and in which order.
 */
/* @todo:
 * - we need locks to avoid two instances writing the preset file
 *   -> flock(fileno()), http://www.ecst.csuchico.edu/~beej/guide/ipc/flock.html
 *   -> better save the new file to a tempfile and then rename?
 *
 * - need to add support for GstChildProxy
 *   we can do this in a next iteration, the format is flexible enough
 *   http://www.buzztard.org/index.php/Preset_handling_interface
 *
 * - should there be a 'preset-list' property to get the preset list
 *   (and to connect a notify:: to to listen for changes)
 *   we could use gnome_vfs_monitor_add() to monitor the user preset_file.
 *
 * - should there be a 'preset-name' property so that we can set a preset via
 *   gst-launch, or should we handle this with special syntax in gst-launch:
 *   gst-lanunch element preset:<preset-name> property=value ...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "preset.h"
//#include "propertymeta/propertymeta.h"

#include "stdlib.h"
#include <unistd.h>
#include <glib/gstdio.h>

#define GST_CAT_DEFAULT preset_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* defines for keyfile usage */
#define PRESET_HEADER "_presets_"

static GQuark preset_user_path_quark = 0;
static GQuark preset_system_path_quark = 0;
static GQuark preset_quark = 0;
static GQuark instance_list_quark = 0;
/*static GQuark property_list_quark = 0;*/

/* default iface implementation */

/* max character per line */
#define LINE_LEN 200

static gboolean gst_preset_default_save_presets_file (GstPreset * self);

#define preset_get_storage(self) \
  (GKeyFile *) g_type_get_qdata (G_TYPE_FROM_INSTANCE (self), preset_quark)

/*
 * preset_get_paths:
 * @self: a #GObject that implements #GstPreset
 * @preset_user_path: location for path or %NULL
 * @preset_system_path: location for path or %NULL
 *
 * Fetch the preset_path for user local and system wide settings. Don't free
 * after use.
 */
static void
preset_get_paths (GstPreset *self, const gchar **preset_user_path, const gchar **preset_system_path)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  gchar *preset_path;

  preset_path = (gchar *) g_type_get_qdata (type, preset_user_path_quark);
  if (!preset_path) {
    const gchar *element_name, *plugin_name, *file_name;
    gchar *preset_dir;
    GstElementFactory *factory;
    GstPlugin *plugin;

    element_name = G_OBJECT_TYPE_NAME (self);
    GST_INFO ("element_name: '%s'", element_name);

    factory = GST_ELEMENT_GET_CLASS (self)->elementfactory;
    if (factory) {
      GST_INFO ("factory: %p", factory);
      plugin_name = GST_PLUGIN_FEATURE (factory)->plugin_name;
      GST_INFO ("plugin_name: '%s'", plugin_name);
      plugin = gst_default_registry_find_plugin (plugin_name);
      GST_INFO ("plugin: %p", plugin);
      file_name = gst_plugin_get_filename (plugin);
      GST_INFO ("file_name: '%s'", file_name);
    }
    else {
      GST_WARNING ("no factory ??");
      return;
    }

    /*
     '/home/ensonic/buzztard/lib/gstreamer-0.10/libgstsimsyn.so'
     -> '/home/ensonic/buzztard/share/gstreamer-0.10/presets/GstSimSyn.prs'
     -> '$HOME/.gstreamer-0.10/presets/GstSimSyn.prs'
     */
    if (preset_user_path) {
      preset_dir =
          g_build_filename (g_get_home_dir (),
            ".gstreamer-" GST_MAJORMINOR, "presets", NULL);
      GST_INFO ("user_preset_dir: '%s'", preset_dir);
      *preset_user_path =
          g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.prs", preset_dir,
          element_name);
      GST_INFO ("user_preset_path: '%s'", *preset_user_path);
      g_mkdir_with_parents (preset_dir, 0755);
      g_free (preset_dir);
      /* attach the preset path to the type */
      g_type_set_qdata (type, preset_user_path_quark, (gpointer) *preset_user_path);
    }
    /*
     '/usr/lib/gstreamer-0.10/libgstaudiofx.so'
     -> '/usr/share/gstreamer-0.10/presets/GstAudioPanorama.prs'
     -> '$HOME/.gstreamer-0.10/presets/GstAudioPanorama.prs'
     */
    if (preset_system_path) {
      preset_dir =
          g_build_filename (DATADIR,
            "gstreamer-" GST_MAJORMINOR, "presets", NULL);
      GST_INFO ("system_preset_dir: '%s'", preset_dir);
      *preset_system_path =
          g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.prs", preset_dir,
          element_name);
      GST_INFO ("system_preset_path: '%s'", *preset_system_path);
      g_mkdir_with_parents (preset_dir, 0755);
      g_free (preset_dir);
      /* attach the preset path to the type */
      g_type_set_qdata (type, preset_system_path_quark, (gpointer) *preset_system_path);
    }
  }
  else {
    if (preset_user_path) {
      *preset_user_path = preset_path;
    }
    if (preset_system_path) {
      *preset_system_path = (gchar *) g_type_get_qdata (type, preset_system_path_quark);
    }
  }
}

static gboolean
preset_skip_property (GParamSpec *property)
{
  if (((property->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE) ||
    (property->flags & G_PARAM_CONSTRUCT_ONLY))
        return TRUE;
  /* FIXME: skip GST_PARAM_NOT_PRESETABLE, see #522205 */
  return FALSE;
}

static void
preset_cleanup (gpointer user_data, GObject * self)
{
  GType type = (GType) user_data;
  GList *instances;

  /* remove instance from instance list (if not yet there) */
  instances = (GList *) g_type_get_qdata (type, instance_list_quark);
  if (instances != NULL) {
    instances = g_list_remove (instances, self);
    GST_INFO ("old instance removed");
    g_type_set_qdata (type, instance_list_quark, (gpointer) instances);
  }
}

static GKeyFile *
preset_open_and_parse_header(GstPreset * self, const gchar *preset_path, gchar **preset_version)
{
  GKeyFile *in = g_key_file_new();
  GError *error = NULL;
  
  if (g_key_file_load_from_file (in, preset_path, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
    const gchar *element_name = G_OBJECT_TYPE_NAME (self);
    gchar *name = g_key_file_get_value (in, PRESET_HEADER, "element", NULL);
    
    if (!name || strcmp (name, element_name)) {
      GST_WARNING ("Wrong element name in preset file %s", preset_path);
      goto Error;
    }
    if (preset_version)
      *preset_version = g_key_file_get_value (in, PRESET_HEADER, "version", NULL);
  }
  else {
    if(error->domain == G_KEY_FILE_ERROR) {
      GST_WARNING ("Unable to read preset file %s: %s\n", preset_path, error->message);
    }
    g_error_free (error);
    goto Error;
  }
  return in;
Error:
  g_key_file_free (in);
  return NULL;
}

static guint64
preset_parse_version (const gchar *str_version) {
  guint64 version = 0;
  gchar *str = (gchar *)str_version, *nstr;
  guint i = 0;

  /* parse version (e.g. 0.10.15.1) to guint64 */
  for (i = 0; i < 4; i++) {
    if (str) {
      version |= g_ascii_strtoull (str, &nstr, 10);
      str = (nstr && (nstr!=str)) ? (nstr + 1) : NULL;
    }
    if (i < 3)
      version <<= 8;
  }
  GST_DEBUG ("version %s -> %" G_GUINT64_FORMAT, str_version, version);
  return version;
}

static void
preset_merge (GKeyFile *system, GKeyFile *user) {
  gchar *str;
  gchar **groups, **keys;
  gsize i, j, num_groups, num_keys;
  
  /* copy file comment if there is any */
  if ((str = g_key_file_get_comment (user, NULL, NULL, NULL))) {
    g_key_file_set_comment (system, NULL, NULL, str, NULL);
    g_free(str);
  }

  /* get groups in user and copy into system */
  groups = g_key_file_get_groups (user, &num_groups);
  for (i = 0; i < num_groups; i++) {
    /* copy group comment if there is any */
    if ((str = g_key_file_get_comment (user, groups[i], NULL, NULL))) {
      g_key_file_set_comment (system, groups[i], NULL, str, NULL);
      g_free(str);
    }

    if (groups[i][0]=='_') continue;

    /* if group already exists in system, remove and re-add keys from user */
    if (g_key_file_has_group (system, groups[i])) {
      g_key_file_remove_group (system, groups[i], NULL);
    }
    
    keys = g_key_file_get_keys (user, groups[i], &num_keys, NULL);
    for (j = 0; j < num_keys; j++) {
      /* copy key comment if there is any */
      if ((str = g_key_file_get_comment (user, groups[i], keys[j], NULL))) {
        g_key_file_set_comment (system, groups[i], keys[j], str, NULL);
        g_free(str);
      }
      str = g_key_file_get_value (user, groups[i], keys[j], NULL);
      g_key_file_set_value (system, groups[i], keys[j], str);
      g_free(str);
    }
    g_strfreev(keys);
  }
  g_strfreev(groups);
}


static gchar **
gst_preset_default_get_preset_names (GstPreset * self)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GKeyFile *presets;

  /* get the presets from the type */
  if (!(presets = preset_get_storage (self))) {
    const gchar *preset_user_path, *preset_system_path;
    gchar *str_version_user = NULL, *str_version_system = NULL;
    gboolean updated_from_system = FALSE;
    GKeyFile *in_user, *in_system;

    preset_get_paths (self, &preset_user_path, &preset_system_path);

    /* test presets */
    in_user = preset_open_and_parse_header (self, preset_user_path, &str_version_user);
    in_system = preset_open_and_parse_header (self, preset_system_path, &str_version_system);
    /* compare version to check for merge */
    if (in_system) {
      if (!in_user || (in_user && (preset_parse_version (str_version_system) > 
          preset_parse_version (str_version_user)))) {
        /* keep system presets */
        presets = in_system;
        updated_from_system = TRUE;
      }
    }
    if (in_user) {
      if (updated_from_system) {
        /* merge user on top of system presets */
        preset_merge (presets, in_user);
        g_key_file_free (in_user);
      }
      else {
        /* keep user presets */
        presets = in_user;
      }
    }

    g_free (str_version_user);
    g_free (str_version_system);

    /* attach the preset to the type */
    g_type_set_qdata (type, preset_quark, (gpointer) presets);

    if (updated_from_system) {
      gst_preset_default_save_presets_file (self);
    }
  }
  
  if (presets) {
    GList *instances;
    gboolean found = FALSE;
    gsize i, num_groups;
    gchar **groups = g_key_file_get_groups (presets, &num_groups);
    gchar **preset_names = g_new (gchar*, num_groups);
    GSList *list=NULL,*node;
    
    /* insert instance in instance list (if not yet there) */
    instances = (GList *) g_type_get_qdata (type, instance_list_quark);
    if (instances != NULL) {
      if (g_list_find (instances, self))
        found = TRUE;
    }
    if (!found) {
      GST_INFO ("new instance added");
      /* register a weak ref, to clean up when the object gets destroyed */
      g_object_weak_ref (G_OBJECT (self), preset_cleanup, (gpointer) type);
      instances = g_list_prepend (instances, self);
      g_type_set_qdata (type, instance_list_quark, (gpointer) instances);
    }

    /* copy group-names to skip private groups */
    for (i = 0; i<num_groups; i++) {
      if (groups[i][0]=='_') continue;
      list = g_slist_prepend (list, g_strdup (groups[i]));
    }
    g_strfreev(groups);

    /* sort and copy back to strvector */
    list = g_slist_sort(list, (GCompareFunc)strcmp);
    for(i = 0, node = list; node; node=g_slist_next (node)) {
      preset_names[i++] = node->data;
    }
    preset_names[i]=NULL;
    g_slist_free(list);

    return preset_names;
  }
  return NULL;
}

static GList *
gst_preset_default_get_property_names (GstPreset * self)
{
  GParamSpec **properties, *property;
  GList *names = NULL;
  guint i, number_of_properties;

  if ((properties = g_object_class_list_properties (G_OBJECT_CLASS
            (GST_ELEMENT_GET_CLASS (self)), &number_of_properties))) {
    GST_DEBUG_OBJECT (self, "  filtering properties: %u", number_of_properties);
    for (i = 0; i < number_of_properties; i++) {
      property = properties[i];
      if (preset_skip_property (property))
        continue;

      names = g_list_prepend (names, property->name);
    }
    g_free (properties);
  }
  else {
    GST_INFO ("no properties");
  }
  return names;
}

static gboolean
gst_preset_default_load_preset (GstPreset * self, const gchar * name)
{
  GKeyFile *presets;

  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {
    if (g_key_file_has_group (presets,name)) {
      GList *properties;

      GST_DEBUG ("loading preset : '%s'", name);

      /* preset found, now set values */
      if ((properties = gst_preset_get_property_names (self))) {
        GParamSpec *property;
        GList *node;
        GValue gvalue={0,};
        gchar *str = NULL;

        for (node = properties; node; node = g_list_next (node)) {
          property = g_object_class_find_property (G_OBJECT_CLASS
              (GST_ELEMENT_GET_CLASS (self)), node->data);

	      /* @todo:
          if(GST_IS_PROPERTY_META(self)) {
            flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
          }
	      else if(!(flags&GST_PROPERTY_META_STATE)) continue;
	      else if(voice_class && g_object_class_find_property(voice_class,property->name)) continue;
	      */
          /* check if we have a settings for this property */
          if ((str = g_key_file_get_value (presets, name, property->name, NULL))) {
            GST_DEBUG ("setting value '%s' for property '%s'", str,
                property->name);
            
            g_value_init(&gvalue, property->value_type);
            if (gst_value_deserialize (&gvalue, str)) {
              g_object_set_property (G_OBJECT (self), property->name, &gvalue);
            } else {
              GST_INFO_OBJECT (self, "deserialization of value '%s' for property '%s' failed", str,
                property->name);
            }
            g_value_unset(&gvalue);
            g_free(str);
          } else {
            GST_INFO ("parameter '%s' not in preset", property->name);
          }
        }
        /* FIXME: handle childproxy properties as well
         * (properties with '/' in the name)
         */
        g_list_free (properties);
        return TRUE;
      } else {
        GST_INFO ("no properties");
      }
    }
    else {
      GST_WARNING ("no preset named %s", name);
    }
  } else {
    GST_WARNING ("no presets");
  }
  return FALSE;
}

static gboolean
gst_preset_default_save_presets_file (GstPreset * self)
{
  GKeyFile *presets;
  const gchar *preset_path;

  preset_get_paths (self, &preset_path, NULL);
  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {
    GError *error = NULL;
    gchar *bak_file_name;
    gboolean backup = TRUE;
    gchar *data;
    gsize data_size;

    GST_DEBUG ("saving preset file: '%s'", preset_path);

    /* create backup if possible */
    bak_file_name = g_strdup_printf ("%s.bak", preset_path);
    if (g_file_test (bak_file_name, G_FILE_TEST_EXISTS)) {
      if (g_unlink (bak_file_name)) {
        backup = FALSE;
        GST_INFO ("cannot remove old backup file : %s", bak_file_name);
      }
    }
    if (backup) {
      if (g_rename (preset_path, bak_file_name)) {
        GST_INFO ("cannot backup file : %s -> %s", preset_path, bak_file_name);
      }
    }
    g_free (bak_file_name);

    /* update gstreamer version */
    g_key_file_set_string (presets, PRESET_HEADER, "version", PACKAGE_VERSION);

    /* write presets */
    if ((data = g_key_file_to_data (presets, &data_size, NULL))) {
      gboolean res = TRUE;

      if(!g_file_set_contents (preset_path, data, data_size, &error)) {
        GST_WARNING ("Unable to store preset file %s: %s\n", preset_path, error->message);
        g_error_free (error);
        res=FALSE;
      }
      g_free (data);
      return res;
    }
  } else {
    GST_DEBUG
        ("no presets, trying to unlink possibly existing preset file: '%s'",
        preset_path);
    unlink (preset_path);
  }
  return FALSE;
}

static gboolean
gst_preset_default_save_preset (GstPreset * self, const gchar * name)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GKeyFile *presets;
  GList *properties;

  GST_INFO ("saving new preset: %s", name);

  /* get the presets from the type */
  if (!(presets = preset_get_storage (self))) {
    /* create a new one */
    presets = g_key_file_new();
    g_key_file_set_string (presets, PRESET_HEADER, "name", G_OBJECT_TYPE_NAME (self));
    /* attach the preset to the type */
    g_type_set_qdata (type, preset_quark, (gpointer) presets);
  }

  /* take copies of current gobject properties from self */
  if ((properties = gst_preset_get_property_names (self))) {
    GParamSpec *property;
    GList *node;
    GValue gvalue={0,};
    gchar *str = NULL;

    for (node = properties; node; node = g_list_next (node)) {
      property = g_object_class_find_property (G_OBJECT_CLASS
          (GST_ELEMENT_GET_CLASS (self)), node->data);

      g_value_init(&gvalue, property->value_type);
      g_object_get_property (G_OBJECT (self), property->name, &gvalue);
      if (!(str = gst_value_serialize(&gvalue))) {
        GST_INFO_OBJECT (self, "serialization for property '%s' failed",
            property->name);
      } else {
        g_key_file_set_string (presets, name, property->name, (gpointer) str);
        str = NULL;
      }
      g_value_unset(&gvalue);
    }
    /* FIXME: handle childproxy properties as well
     * (properties with '/' in the name)
     */
    GST_INFO ("  saved");
    g_list_free (properties);
  } else {
    GST_INFO ("no properties");
  }

  GST_INFO ("done");

  return gst_preset_default_save_presets_file (self);
}

static gboolean
gst_preset_default_rename_preset (GstPreset * self, const gchar * old_name,
    const gchar * new_name)
{
  GKeyFile *presets;
  gchar *str;
  gchar **keys;
  gsize j, num_keys;

  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {
    if (g_key_file_has_group (presets, old_name)) {
      /* copy group comment if there is any */
      if ((str = g_key_file_get_comment (presets, old_name, NULL, NULL))) {
        g_key_file_set_comment (presets, new_name, NULL, str, NULL);
        g_free(str);
      }
  
      keys = g_key_file_get_keys (presets, old_name, &num_keys, NULL);
      for (j = 0; j < num_keys; j++) {
        /* copy key comment if there is any */
        if ((str = g_key_file_get_comment (presets, old_name, keys[j], NULL))) {
          g_key_file_set_comment (presets, new_name, keys[j], str, NULL);
          g_free(str);
        }
        str = g_key_file_get_value (presets, old_name, keys[j], NULL);
        g_key_file_set_value (presets, new_name, keys[j], str);
        g_free(str);
      }
      g_strfreev(keys);
  
      g_key_file_remove_group (presets, old_name, NULL);
  
      return gst_preset_default_save_presets_file (self);
    } else {
      GST_WARNING ("no preset named %s", old_name);
    }
  } else {
    GST_WARNING ("no presets");
  }
  return FALSE;
}

static gboolean
gst_preset_default_delete_preset (GstPreset * self, const gchar * name)
{
  GKeyFile *presets;

  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {

    if (g_key_file_has_group (presets, name)) {
      g_key_file_remove_group (presets, name, NULL);
      return gst_preset_default_save_presets_file (self);
    } else {
      GST_WARNING ("no preset named %s", name);
    }
  } else {
    GST_WARNING ("no presets");
  }
  return FALSE;
}

static gboolean
gst_preset_default_set_meta (GstPreset * self, const gchar * name,
    const gchar * tag, const gchar * value)
{
  GKeyFile *presets;

  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {
    gchar *key = g_strdup_printf ("_meta/%s",tag);

    if(value && *value) {
      g_key_file_set_value (presets, name, key, value);
    }
    else {
      g_key_file_remove_key (presets, name, key, NULL);
    }
    g_free (key);

    return gst_preset_default_save_presets_file (self);
  } else {
    GST_WARNING ("no presets");
  }
  return FALSE;
}

static gboolean
gst_preset_default_get_meta (GstPreset * self, const gchar * name,
    const gchar * tag, gchar ** value)
{
  GKeyFile *presets;
  
  g_assert (value);

  /* get the presets from the type */
  if ((presets = preset_get_storage (self))) {
    gchar *key = g_strdup_printf ("_meta/%s",tag);
    
    *value = g_key_file_get_value(presets, name, key, NULL);
    g_free (key);
    return TRUE;
  } else {
    GST_WARNING ("no presets");
  }
  *value = NULL;
  return FALSE;
}

static void
gst_preset_default_randomize (GstPreset * self)
{
  GList *properties;
  GType base, parent;

  if ((properties = gst_preset_get_property_names (self))) {
    GParamSpec *property;
    GList *node;
    gdouble rnd;

    for (node = properties; node; node = g_list_next (node)) {
      property = g_object_class_find_property (G_OBJECT_CLASS
          (GST_ELEMENT_GET_CLASS (self)), node->data);

      rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);

      /* get base type */
      base = property->value_type;
      while ((parent = g_type_parent (base)))
        base = parent;
      GST_INFO ("set random value for property: %s (type is %s)", property->name,
          g_type_name (base));

      switch (base) {
        case G_TYPE_BOOLEAN:{
          g_object_set (self, property->name, (gboolean) (2.0 * rnd), NULL);
        } break;
        case G_TYPE_INT:{
          const GParamSpecInt *int_property = G_PARAM_SPEC_INT (property);

          g_object_set (self, property->name,
              (gint) (int_property->minimum + ((int_property->maximum -
                          int_property->minimum) * rnd)), NULL);
        } break;
        case G_TYPE_UINT:{
          const GParamSpecUInt *uint_property = G_PARAM_SPEC_UINT (property);

          g_object_set (self, property->name,
              (guint) (uint_property->minimum + ((uint_property->maximum -
                          uint_property->minimum) * rnd)), NULL);
        } break;
        case G_TYPE_DOUBLE:{
          const GParamSpecDouble *double_property =
              G_PARAM_SPEC_DOUBLE (property);

          g_object_set (self, property->name,
              (gdouble) (double_property->minimum + ((double_property->maximum -
                          double_property->minimum) * rnd)), NULL);
        } break;
        case G_TYPE_ENUM:{
          const GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (property);
          const GEnumClass *enum_class = enum_property->enum_class;

          g_object_set (self, property->name,
              (gulong) (enum_class->minimum + ((enum_class->maximum -
                          enum_class->minimum) * rnd)), NULL);
        } break;
        default:
          GST_WARNING ("incomplete implementation for GParamSpec type '%s'",
              G_PARAM_SPEC_TYPE_NAME (property));
      }
    }
    /* FIXME: handle childproxy properties as well */
  }
}

static void
gst_preset_default_reset (GstPreset * self)
{
  GList *properties;

  if ((properties = gst_preset_get_property_names (self))) {
    GParamSpec *property;
    GList *node;
    GValue gvalue={0,};

    for (node = properties; node; node = g_list_next (node)) {
      property = g_object_class_find_property (G_OBJECT_CLASS
          (GST_ELEMENT_GET_CLASS (self)), node->data);
      
      g_value_init(&gvalue, property->value_type);
      g_param_value_set_default (property, &gvalue);
      g_object_set_property (G_OBJECT (self), property->name, &gvalue);
      g_value_unset(&gvalue);
    }
    /* FIXME: handle childproxy properties as well */
  }
}

/* wrapper */

/**
 * gst_preset_get_preset_names:
 * @self: a #GObject that implements #GstPreset
 *
 * Get a copy of preset names as a NULL terminated string array. Free with
 * g_strfreev() wen done.
 *
 * Returns: list with names
 */
gchar **
gst_preset_get_preset_names (GstPreset * self)
{
  g_return_val_if_fail (GST_IS_PRESET (self), NULL);

  return (GST_PRESET_GET_INTERFACE (self)->get_preset_names (self));
}

/**
 * gst_preset_get_property_names:
 * @self: a #GObject that implements #GstPreset
 *
 * Get a the gobject property names to use for presets.
 *
 * Returns: list with names
 */
GList *
gst_preset_get_property_names (GstPreset * self)
{
  g_return_val_if_fail (GST_IS_PRESET (self), NULL);

  return (GST_PRESET_GET_INTERFACE (self)->get_property_names (self));
}

/**
 * gst_preset_load_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to load
 *
 * Load the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_load_preset (GstPreset * self, const gchar * name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->load_preset (self, name));
}

/**
 * gst_preset_save_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to save
 *
 * Save the current preset under the given name. If there is already a preset by
 * this @name it will be overwritten.
 *
 * Returns: %TRUE for success, %FALSE
 */
gboolean
gst_preset_save_preset (GstPreset * self, const gchar * name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->save_preset (self, name));
}

/**
 * gst_preset_rename_preset:
 * @self: a #GObject that implements #GstPreset
 * @old_name: current preset name
 * @new_name: new preset name
 *
 * Renames a preset. If there is already a preset by the @new_name it will be
 * overwritten.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with @old_name
 */
gboolean
gst_preset_rename_preset (GstPreset * self, const gchar * old_name,
    const gchar * new_name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (old_name, FALSE);
  g_return_val_if_fail (new_name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->rename_preset (self, old_name,
          new_name));
}

/**
 * gst_preset_delete_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to remove
 *
 * Delete the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_delete_preset (GstPreset * self, const gchar * name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->delete_preset (self, name));
}

/**
 * gst_preset_set_meta:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name
 * @tag: meta data item name
 * @value: new value
 *
 * Sets a new @value for an existing meta data item or adds a new item. Meta
 * data @tag names can be something like e.g. "comment". Supplying %NULL for the
 * @value will unset an existing value.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_set_meta (GstPreset * self, const gchar * name, const gchar * tag,
    const gchar * value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);

  return GST_PRESET_GET_INTERFACE (self)->set_meta (self, name, tag, value);
}

/**
 * gst_preset_get_meta:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name
 * @tag: meta data item name
 * @value: value
 *
 * Gets the @value for an existing meta data @tag. Meta data @tag names can be
 * something like e.g. "comment". Returned values need to be released when done.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 * or no value for the given @tag
 */
gboolean
gst_preset_get_meta (GstPreset * self, const gchar * name, const gchar * tag,
    gchar ** value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);
  g_return_val_if_fail (value, FALSE);

  return GST_PRESET_GET_INTERFACE (self)->get_meta (self, name, tag, value);
}

/**
 * gst_preset_randomize:
 * @self: a #GObject that implements #GstPreset
 *
 * Create a new randomized preset. This method is optional. If not overridden
 * true randomization will be applied. Elements can override this if they can
 * make more meaningful variations or even override with an empty implementation
 * if it does not make sense for them.
 */
void
gst_preset_randomize (GstPreset * self)
{
  g_return_if_fail (GST_IS_PRESET (self));

  GST_PRESET_GET_INTERFACE (self)->randomize (self);
}

/**
 * gst_preset_reset:
 * @self: a #GObject that implements #GstPreset
 *
 * Resets values to defaults. This method is optional. If not overridden
 * default from paramspecs are used.
 */
void
gst_preset_reset (GstPreset * self)
{
  g_return_if_fail (GST_IS_PRESET (self));

  GST_PRESET_GET_INTERFACE (self)->reset (self);
}

/* class internals */

static void
gst_preset_class_init (GstPresetInterface * iface)
{
  iface->get_preset_names = gst_preset_default_get_preset_names;
  iface->get_property_names = gst_preset_default_get_property_names;

  iface->load_preset = gst_preset_default_load_preset;
  iface->save_preset = gst_preset_default_save_preset;
  iface->rename_preset = gst_preset_default_rename_preset;
  iface->delete_preset = gst_preset_default_delete_preset;

  iface->set_meta = gst_preset_default_set_meta;
  iface->get_meta = gst_preset_default_get_meta;

  iface->randomize = gst_preset_default_randomize;
  iface->reset = gst_preset_default_reset;
}

static void
gst_preset_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* init default implementation */
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "preset",
        GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "preset interface");

    /* create quarks for use with g_type_{g,s}et_qdata() */
    preset_quark = g_quark_from_static_string ("GstPreset::presets");
    preset_user_path_quark = g_quark_from_static_string ("GstPreset::user_path");
    preset_system_path_quark = g_quark_from_static_string ("GstPreset::system_path");
    instance_list_quark = g_quark_from_static_string ("GstPreset::instances");

#if 0
    property_list_quark = g_quark_from_static_string ("GstPreset::properties");

    /* create interface properties, each element would need to override this
     *   g_object_class_override_property(gobject_class, PROP_PRESET_NAME, "preset-name");
     * and in _set_property() do
     *   case PROP_PRESET_NAME: {
     *     gchar *name = g_value_get_string (value);
     *     if (name)
     *       gst_preset_load_preset(self, name);
     *   } break;
     */
    g_object_interface_install_property (g_class,
      g_param_spec_string ("preset-name",
      "preset-name property",
      "load given preset",
      NULL,
      G_PARAM_WRITABLE));
#endif

    initialized = TRUE;
  }
}

GType
gst_preset_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstPresetInterface),
      (GBaseInitFunc) gst_preset_base_init,     /* base_init */
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_preset_class_init,   /* class_init */
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      0,
      0,                        /* n_preallocs */
      NULL                      /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE, "GstPreset", &info, 0);
  }
  return type;
}
