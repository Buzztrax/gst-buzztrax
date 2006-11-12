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
 * This interface offers a methods to query and manipulate parameter preset
 * sets.
 */

#include "preset.h"

/**
 * gst_preset_get_preset_names:
 * @self: a #GObject that implements #GstPreset
 *
 * Get a copy of the preset list names. Free list when done.
 *
 * Returns: list with names
 */
GList*
gst_preset_get_preset_names (GstPreset *self) {
  g_return_if_fail (GST_IS_PRESET (self));

  return (GST_PRESET_GET_INTERFACE (self)->get_preset_names (self));  
}

/**
 * gst_preset_load_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to load
 *
 * Load the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that name
 */
gboolean
gst_preset_load_preset (GstPreset *self, const gchar *name) {
  g_return_if_fail (GST_IS_PRESET (self));

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
gst_preset_save_preset (GstPreset *self, const gchar *name) {
  g_return_if_fail (GST_IS_PRESET (self));

  return (GST_PRESET_GET_INTERFACE (self)->save_preset (self, name));  
}

/**
 * gst_preset_rename_preset:
 * @self: a #GObject that implements #GstPreset
 * @old_name: current preset name
 * @new_name: new preset name
 *
 * Renames a preset. If there is already a preset by thr @new_name it will be
 * overwritten.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with @old_name
 */
gboolean
gst_preset_rename_preset (GstPreset *self, const gchar *old_name, const gchar *new_name) {
  g_return_if_fail (GST_IS_PRESET (self));

  return (GST_PRESET_GET_INTERFACE (self)->rename_preset (self,old_name,new_name));  
}

/**
 * gst_preset_delete_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to remove
 *
 * Delete the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that name
 */
gboolean
gst_preset_delete_preset (GstPreset *self, const gchar *name) {
  g_return_if_fail (GST_IS_PRESET (self));

  return (GST_PRESET_GET_INTERFACE (self)->delete_preset (self,name));  
}

/**
 * gst_preset_create_preset:
 * @self: a #GObject that implements #GstPreset
 *
 * Create a new randomized preset. This method is optional. If not implemented
 * true randomization will be applied.
 */
void
gst_preset_create_preset (GstPreset *self) {
  g_return_if_fail (GST_IS_PRESET (self));

  if(GST_PRESET_GET_INTERFACE (self)->create_preset)
	GST_PRESET_GET_INTERFACE (self)->create_preset (self);
  else {
	/* @todo: implementation default randomization */
  }
}

static void
gst_preset_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */

    initialized = TRUE;
  }
}

GType
gst_preset_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GstPresetInterface),
      gst_preset_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstPreset",&info,0);
  }
  return type;
}
