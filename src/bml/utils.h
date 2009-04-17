/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * utils.h: Header for functions shared among source and transform elements
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


#ifndef __GST_BML_UTILS_H__
#define __GST_BML_UTILS_H__

#include "plugin.h"

G_BEGIN_DECLS

//-- helper

extern gboolean bml(gstbml_is_polyphonic(gpointer bm));

//-- common iface functions

extern gchar *bml(gstbml_property_meta_describe_property(gpointer bm, glong index, GValue *event));
extern void bml(gstbml_tempo_change_tempo(GObject *gstbml, GstBML *bml, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick));

//-- common class functions

extern gpointer bml(gstbml_class_base_init(GstBMLClass *klass, GType type, gint numsrcpads, gint numsinkpads));
extern void bml(gstbml_class_set_details(GstElementClass *element_class, gpointer bm, const gchar *category));
extern void bml(gstbml_class_prepare_properties(GObjectClass *klass, GstBMLClass *bml_class));
extern GType bml(gstbml_register_track_enum_type(GObjectClass *klass, gpointer bm, gint i, gchar *name, gint min_val, gint max_val, gint no_val));

//-- common element functions

extern void bml(gstbml_init(GstBML *bml,GstBMLClass *klass,GstElement *element));
extern void bml(gstbml_init_pads(GstElement *element, GstBML *bml, GstPadLinkFunction *gst_bml_link));

extern void bml(gstbml_finalize(GstBML *bml));

extern void bml(gstbml_set_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, const GValue *value, GParamSpec * pspec));
extern void bml(gstbml_get_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, GValue *value, GParamSpec * pspec));

extern void bml(gstbml_sync_values(GstBML *bml, GstBMLClass *bml_class));
extern void bml(gstbml_reset_triggers(GstBML *bml, GstBMLClass *bml_class));

G_END_DECLS

#endif /* __GST_BML_UTILS_H__ */
