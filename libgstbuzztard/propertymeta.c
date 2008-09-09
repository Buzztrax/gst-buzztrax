/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * propertymeta.c: helper interface for extended gstreamer element meta data
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
 * SECTION:gstbtpropertymeta
 * @short_description: helper interface for extended gstreamer element meta data
 *
 * This interface standardises some additional meta-data that is attached to
 * #GObject properties.
 *
 * Furthermore it adds the gstbt_property_meta_describe_property() method that
 * builds a string description of a property value.
 */

#include "propertymeta.h"

/**
 * gstbt_property_meta_quark:
 *
 * Only if this is set to TRUE, there is property meta data for this property..
 */
GQuark gstbt_property_meta_quark;
/**
 * gstbt_property_meta_quark_min_val:
 *
 * Minimum property value (excluding default and no-value).
 */
GQuark gstbt_property_meta_quark_min_val=0;
/**
 * gstbt_property_meta_quark_max_val:
 *
 * Maximum property value (excluding default and no-value).
 */
GQuark gstbt_property_meta_quark_max_val=0;
/**
 * gstbt_property_meta_quark_def_val:
 *
 * Default property value (used initialy).
 */
GQuark gstbt_property_meta_quark_def_val=0;
/**
 * gstbt_property_meta_quark_no_val:
 *
 * Property value (used in trigger style properties, when there is no current
 * value)
 */
GQuark gstbt_property_meta_quark_no_val=0;
/**
 * gstbt_property_meta_quark_flags:
 *
 * Application specific flags giving more hints about the property.
 */
GQuark gstbt_property_meta_quark_flags=0;

/**
 * gstbt_property_meta_describe_property:
 * @self: a #GObject that implements #GstBtPropertyMeta
 * @index: the property index
 * @value: the current property value
 *
 * Formats the gives value as a human readable string. The method is useful to
 * display a property value in a user interface.
 * It privides a default implementation.
 *
 * Returns: a string with the value in humand readable form, free memory when
 * done
 */
 // @todo make index generic (property name)
gchar *
gstbt_property_meta_describe_property (GstBtPropertyMeta *self, glong index, GValue *value)
{
  g_return_val_if_fail (GSTBT_IS_PROPERTY_META (self), FALSE);

  if(GSTBT_PROPERTY_META_GET_INTERFACE (self)->describe_property) {
    return (GSTBT_PROPERTY_META_GET_INTERFACE (self)->describe_property (self, index, value));
  }
  else {
    return(g_strdup_value_contents(value));
  }
}

static void
gstbt_property_meta_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create quarks for use with g_param_spec_{g,s}et_qdata() */
    gstbt_property_meta_quark=g_quark_from_string("GstBtPropertyMeta::");
    gstbt_property_meta_quark_min_val=g_quark_from_string("GstBtPropertyMeta::min-val");
    gstbt_property_meta_quark_max_val=g_quark_from_string("GstBtPropertyMeta::max-val");
    gstbt_property_meta_quark_def_val=g_quark_from_string("GstBtPropertyMeta::def-val");
    gstbt_property_meta_quark_no_val=g_quark_from_string("GstBtPropertyMeta::no-val");
    gstbt_property_meta_quark_flags=g_quark_from_string("GstBtPropertyMeta::flags");

    initialized = TRUE;
  }
}

GType
gstbt_property_meta_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstBtPropertyMetaInterface),
      (GBaseInitFunc) gstbt_property_meta_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstBtPropertyMeta",&info,0);
  }
  return type;
}
