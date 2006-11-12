/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * help.c: helper interface for element user docs
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
 * SECTION:gsthelp
 * @short_description: helper interface for element user docs
 *
 * This interface offers a method to query the URL of the user documentation.
 */

#include "help.h"

/**
 * gst_help_get_documentation_uri:
 * @self: a #GObject that implements #GstHelp
 *
 * Get URI of the user documentation. Can be shown with e.g. gnome_url_show().
 *
 * Returns: The documentation uri. Do not modify!
 */
const gchar *
gst_help_get_documentation_uri (GstHelp *self)
{
  g_return_if_fail (GST_IS_HELP (self));

  return (GST_HELP_GET_INTERFACE (self)->get_documentation_uri (self));
}

static void
gst_help_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */

    initialized = TRUE;
  }
}

GType
gst_help_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GstHelpInterface),
      gst_help_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstHelp",&info,0);
  }
  return type;
}
