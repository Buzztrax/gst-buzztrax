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
 * SECTION:gstbthelp
 * @short_description: helper interface for element user docs
 *
 * This interface offers a method to query the URL of the user documentation.
 */

#include "help.h"
#include "compat.h"

//-- the iface

G_DEFINE_INTERFACE (GstBtHelp, gstbt_help, 0);


static void
gstbt_help_default_init(GstBtHelpInterface *iface)
{
  g_object_interface_install_property (iface,
    g_param_spec_string ("documentation-uri",
      "documentation uri help property",
      "uri of the user documentation",
      NULL,
      G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

