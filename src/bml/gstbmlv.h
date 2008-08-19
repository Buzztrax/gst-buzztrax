/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbml.h: Header for BML plugin
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


#ifndef __GST_BMLV_H__
#define __GST_BMLV_H__

#include "plugin.h"

G_BEGIN_DECLS

#define GST_BMLV(obj) ((GstBMLV *)obj)
#define GST_BMLV_CLASS(klass) ((GstBMLVClass *)klass)

typedef struct _GstBMLV GstBMLV;
typedef struct _GstBMLVClass GstBMLVClass;

struct _GstBMLV {
  GstObject object;

  gboolean dispose_has_run;

  // the buzz machine handle (to use with libbml API)
  void *bm;

  // the parent gst-element
  //GstElement *parent;
  // the voice number
  guint voice;
};

struct _GstBMLVClass {
  GstObjectClass parent_class;

  // the buzz machine handle (to use with libbml API)
  void *bm;
  
  gint numtrackparams;
};

extern GType bml(v_get_type(gchar *voice_type_name));

G_END_DECLS

#endif /* __GST_BMLV_H__ */
