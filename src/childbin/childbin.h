/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * childbin.h: helper interface header for multi child gstreamer elements
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

#ifndef __GST_CHILD_BIN_H__
#define __GST_CHILD_BIN_H__

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/gstchildproxy.h>

G_BEGIN_DECLS

#define GST_TYPE_CHILD_BIN               (gst_child_bin_get_type())
#define GST_CHILD_BIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_CHILD_BIN, GstChildBin))
#define GST_IS_CHILD_BIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_CHILD_BIN))
#define GST_CHILD_BIN_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GST_TYPE_CHILD_BIN, GstChildBinInterface))


typedef struct _GstChildBin GstChildBin; /* dummy object */
typedef struct _GstChildBinInterface GstChildBinInterface;

struct _GstChildBinInterface
{
  GTypeInterface parent;

  gboolean (*add_child) (GstChildBin *self, GstObject *child);
  gboolean (*remove_child) (GstChildBin *self, GstObject *child);
};

GType gst_child_bin_get_type(void);

gboolean gst_child_bin_add_child (GstChildBin *self, GstObject *child);
gboolean gst_child_bin_remove_child (GstChildBin *self, GstObject *child);

G_END_DECLS

#endif /* __GST_CHILD_BIN_H__ */
