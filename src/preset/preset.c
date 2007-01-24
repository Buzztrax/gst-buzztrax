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
 * sets. The name of a presets serves as key for subsequent method calls to 
 * manipulate single presets. All instances of one type will share the list of
 * presets.
 */

#include "preset.h"
#include "propertymeta/propertymeta.h"

#if 0
static GQuark preset_list_quark=0;
static GQuark instance_list_quark=0;
#endif

/* default implementation */

static GList*
gst_preset_default_get_preset_names (GstPreset *self)
{
  GST_WARNING("not yet implemented");
#if 0
  GType *type = G_TYPE_FROM_INSTANCE (self);
  GList *presets;
  GList *instances;
  gboolean found = FALSE;

  // get the preset list from the type
  preset = (GList *) g_type_get_qdata (type, preset_list_quark);
  if (presets == NULL) {
	gchar *element_name, *plugin_name, *file_name;
	GstElementFactory *factory;
    GstPlugin *plugin;
    
    element_name = G_OBJECT_TYPE_NAME(self);
    factory = gst_element_factory_find(element_name);
    plugin_name = GST_PLUGIN_FEATURE (factory)->plugin_name;
    plugin = gst_default_registry_find_plugin (plugin_name);
    file_name = gst_plugin_get_filename (plugin);

	// we need to add DATADIR=$prefix/share/gstreamer-0.10 to config.h
	// and what about plugins not in $GST_PLUGIN_PATH ?
	// $DATADIR/presets/<element_name>.xml
	// $HOME/.gstreamer-0.10/presets/<element_name>.xml
  
	// attach the preset list to the type
	g_type_set_qdata (type, preset_list_quark, (gpointer) presets);
  }
  
  // insert instance in instance list (if not yet there)
  instances = (GList *) g_type_get_qdata (type, instance_list_quark);
  if (instances != NULL) {
	if (g_list_find (instances, self))
	  found = TRUE;
  }
  if (!found) {
	instance = g_list_prepend (instances, self);
	g_type_set_qdata (type, instance_list_quark, (gpointer) instances);
  }
  
#endif
}

static gboolean
gst_preset_default_load_preset (GstPreset *self, const gchar *name)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static gboolean
gst_preset_default_save_preset (GstPreset *self, const gchar *name)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static gboolean
gst_preset_default_rename_preset (GstPreset *self, const gchar *old_name, const gchar *new_name)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static gboolean
gst_preset_default_delete_preset (GstPreset *self, const gchar *name)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static gboolean 
gst_preset_default_set_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar *value)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static gboolean
gst_preset_default_get_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar **value)
{
  GST_WARNING("not yet implemented");
#if 0
#endif
}

static void
gst_preset_default_create_preset (GstPreset *self)
{
  GParamSpec **properties,*property;
  guint i,number_of_properties;
  GType param_type,base_type;

  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_OBJECT_GET_CLASS(self)),&number_of_properties))) {
    gdouble rnd;
    guint flags=0;

    /* @todo: what about voice properties */
    
    GST_INFO("nr of values : %d", number_of_properties);
    for(i=0;i<number_of_properties;i++) {
      property=properties[i];
      if(GST_IS_PROPERTY_META(self)) {
        flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
      }

      /* skip non-controlable, trigger params & voice params */
      if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
      else if(!(flags&GST_PROPERTY_META_STATE)) continue;

      GST_INFO("property '%s' (GType=%d)",property->name,property->value_type);

      param_type=property->value_type;
      while((base_type=g_type_parent(param_type))) param_type=base_type;
      
      rnd=((gdouble)rand()) / (RAND_MAX + 1.0);
      switch(param_type) {
        case G_TYPE_BOOLEAN: {
          g_object_set(self,property->name,(gboolean)(2.0 * rnd),NULL);
        } break;
        case G_TYPE_INT: {
          const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
          
          g_object_set(self,property->name,(gint)(int_property->minimum+((int_property->maximum-int_property->minimum)*rnd)),NULL);
        } break;
        case G_TYPE_UINT: {
          const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
          
          g_object_set(self,property->name,(guint)(uint_property->minimum+((uint_property->maximum-uint_property->minimum)*rnd)),NULL);
        }  break;
        case G_TYPE_DOUBLE: {
          const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);

          g_object_set(self,property->name,(gdouble)(double_property->minimum+((double_property->maximum-double_property->minimum) * rnd)),NULL);
        } break;
        case G_TYPE_ENUM: {
          const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
          const GEnumClass *enum_class=enum_property->enum_class;

          g_object_set(self,property->name,(gulong)(enum_class->minimum+((enum_class->maximum-enum_class->minimum) * rnd)),NULL);
        } break;
        default:
          //GST_WARNING("unhandled GType=%d:'%s'",param_type,G_VALUE_TYPE_NAME(param_type));
          GST_WARNING("unhandled GType=%d",param_type);
      }
    }
  }
}

/* wrapper */

/**
 * gst_preset_get_preset_names:
 * @self: a #GObject that implements #GstPreset
 *
 * Get a copy of the preset list names. Free list when done.
 *
 * Returns: list with names
 */
GList*
gst_preset_get_preset_names (GstPreset *self)
{
  g_return_val_if_fail (GST_IS_PRESET (self), NULL);

  return (GST_PRESET_GET_INTERFACE (self)->get_preset_names (self));  
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
gst_preset_load_preset (GstPreset *self, const gchar *name)
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
gst_preset_save_preset (GstPreset *self, const gchar *name)
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
 * Renames a preset. If there is already a preset by thr @new_name it will be
 * overwritten.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with @old_name
 */
gboolean
gst_preset_rename_preset (GstPreset *self, const gchar *old_name, const gchar *new_name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (old_name, FALSE);
  g_return_val_if_fail (new_name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->rename_preset (self,old_name,new_name));  
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
gst_preset_delete_preset (GstPreset *self, const gchar *name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->delete_preset (self,name));  
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
gst_preset_set_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar *value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);

  GST_PRESET_GET_INTERFACE (self)->set_meta (self,name,tag,value);
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
gst_preset_get_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar **value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);
  g_return_val_if_fail (value, FALSE);

  GST_PRESET_GET_INTERFACE (self)->get_meta (self,name,tag,value);
}

/**
 * gst_preset_create_preset:
 * @self: a #GObject that implements #GstPreset
 *
 * Create a new randomized preset. This method is optional. If not implemented
 * true randomization will be applied.
 */
void
gst_preset_create_preset (GstPreset *self)
{
  g_return_if_fail (GST_IS_PRESET (self));

  GST_PRESET_GET_INTERFACE (self)->create_preset (self);
}

/* class internals */

static void
gst_preset_class_init(GstPresetInterface *iface)
{
  iface->get_preset_names = gst_preset_default_get_preset_names;

  iface->load_preset = gst_preset_default_load_preset;
  iface->save_preset = gst_preset_default_save_preset;
  iface->rename_preset = gst_preset_default_rename_preset;
  iface->delete_preset = gst_preset_default_delete_preset;
  
  iface->set_meta = gst_preset_default_set_meta;
  iface->get_meta = gst_preset_default_get_meta;
  
  iface->create_preset = gst_preset_default_create_preset;
}

static void
gst_preset_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* init default implementation */

#if 0
    /* create quarks for use with g_type_{g,s}et_qdata() */
    preset_list_quark=g_quark_from_string("GstPreset::presets");
    instance_list_quark=g_quark_from_string("GstPreset::instances");
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
      (GBaseInitFunc) gst_preset_base_init,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) gst_preset_class_init,   /* class_init */
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
