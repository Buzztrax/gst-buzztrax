/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * propertymeta.h: helper interface header for extendet gstreamer element meta data
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

#ifndef __GST_PROPERTY_META_H__
#define __GST_PROPERTY_META_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_PROPERTY_META               (gst_property_meta_get_type())
#define GST_PROPERTY_META(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_PROPERTY_META, GstPropertyMeta))
#define GST_IS_PROPERTY_META(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_PROPERTY_META))
#define GST_PROPERTY_META_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GST_TYPE_PROPERTY_META, GstPropertyMetaInterface))


typedef struct _GstPropertyMeta GstPropertyMeta; /* dummy object */
typedef struct _GstPropertyMetaInterface GstPropertyMetaInterface;

struct _GstPropertyMetaInterface
{
  GTypeInterface parent;

  gchar *(*describe_property) (GstPropertyMeta *self, glong index, GValue *value);
};

GType gst_property_meta_get_type(void);

gchar *gst_property_meta_describe_property (GstPropertyMeta *self, glong index, GValue *value);

extern GQuark gst_property_meta_quark_min_val;
extern GQuark gst_property_meta_quark_max_val;
extern GQuark gst_property_meta_quark_def_val;
extern GQuark gst_property_meta_quark_no_val;
extern GQuark gst_property_meta_quark_flags;

G_END_DECLS

#endif /* __GST_PROPERTY_META_H__ */
