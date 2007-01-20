/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * childbin.c: helper interface for multi child gstreamer elements
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
 * SECTION:gstchildbin
 * @short_description: helper interface for multi child gstreamer elements
 *
 * This interface provides an extension to the #GstChildProxy interface, which
 * is useful for classes that have identical children.
 *
 * The interface provides a children-count property as well as two methods to
 * add and remove children.
 */
 
#include "childbin.h"

/**
 * gst_child_bin_add_child:
 * @self: a #GObject that implements #GstChildBin
 * @child: the #GstObject to add as a child
 *
 * Add the given child to the list of children.
 *
 * Returns: %TRUE for success
 */
gboolean
gst_child_bin_add_child (GstChildBin * self, GstObject *child)
{
  g_return_val_if_fail (GST_IS_CHILD_BIN (self), FALSE);

  return (GST_CHILD_BIN_GET_INTERFACE (self)->add_child (self, child));
}

/**
 * gst_child_bin_remove_child:
 * @self: a #GObject that implements #GstChildBin
 * @child: the #GstObject to remove from the children
 *
 * Remove the given child from the list of children.
 *
 * Returns: %TRUE for success
 */
gboolean
gst_child_bin_remove_child (GstChildBin * self, GstObject *child)
{
  g_return_val_if_fail (GST_IS_CHILD_BIN (self), FALSE);

  return (GST_CHILD_BIN_GET_INTERFACE (self)->remove_child (self, child));
}

static void
gst_child_bin_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */
    g_object_interface_install_property (g_class,
      g_param_spec_ulong ("children",
      "children count property",
      "the number of children this element uses",
      1,
      G_MAXULONG,
      1,
      G_PARAM_READWRITE));
    initialized = TRUE;
  }
}

GType
gst_child_bin_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GstChildBinInterface),
      (GBaseInitFunc) gst_child_bin_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstChildBin",&info,0);
    g_type_interface_add_prerequisite (type, GST_TYPE_CHILD_PROXY);
  }
  return type;
}
