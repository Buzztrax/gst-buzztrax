/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * help.h: helper interface header for telement user docs
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

#ifndef __GSTBT_HELP_H__
#define __GSTBT_HELP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_HELP               (gstbt_help_get_type())
#define GSTBT_HELP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_HELP, GstBtHelp))
#define GSTBT_IS_HELP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_HELP))
#define GSTBT_HELP_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSTBT_TYPE_HELP, GstBtHelpInterface))


typedef struct _GstBtHelp GstBtHelp; /* dummy object */
typedef struct _GstBtHelpInterface GstBtHelpInterface;

/**
 * GstBtHelp:
 *
 * Opaque interface structure.
 */
struct _GstBtHelpInterface
{
  GTypeInterface parent;
};

GType gstbt_help_get_type(void);

G_END_DECLS

#endif /* __GSTBT_HELP_H__ */
