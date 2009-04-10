/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * common.h: Header for functions shared among all native and wrapped elements
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


#ifndef __GST_BML_COMMON_H__
#define __GST_BML_COMMON_H__

#include "plugin.h"

G_BEGIN_DECLS

//-- helper

extern const gchar *gstbml_get_help_uri(gpointer bm);

//-- preset iface

extern gchar** gstbml_preset_get_preset_names(GstBML *bml, GstBMLClass *klass);
extern gboolean gstbml_preset_load_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name);
extern gboolean gstbml_preset_save_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name);
extern gboolean gstbml_preset_rename_preset(GstBMLClass *klass, const gchar *old_name, const gchar *new_name);
extern gboolean gstbml_preset_delete_preset(GstBMLClass *klass, const gchar *name);
extern gboolean gstbml_preset_set_meta(GstBMLClass *klass, const gchar *name ,const gchar *tag, const gchar *value);
extern gboolean gstbml_preset_get_meta(GstBMLClass *klass, const gchar *name, const gchar *tag, gchar **value);
extern void gstbml_preset_finalize(GstBMLClass *klass);

//-- common class functions

extern void gstbml_convert_names(GObjectClass *klass, gchar *tmp_name, gchar *tmp_desc, gchar **_name, gchar **nick, gchar **desc);
extern GParamSpec *gstbml_register_param(GObjectClass *klass,gint prop_id, gint type, GType enum_type, gchar *name, gchar *nick, gchar *desc, gint flags, gint min_val, gint max_val, gint no_val, gint def_val);

//-- common element functions

extern void gstbml_set_param(gint type,gpointer addr,const GValue *value);
extern void gstbml_get_param(gint type,gpointer addr,GValue *value);
extern void gstbml_calculate_buffer_frames(GstBML *bml);
extern void gstbml_sync_values(GstBML *bml);

extern void gstbml_dispose(GstBML *bml);

G_END_DECLS

#endif /* __GST_BML_COMMON_H__ */
