/* 
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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
 
#ifndef __GST_AUDIODELAY_H__
#define __GST_AUDIODELAY_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_AUDIODELAY            (gst_audiodelay_get_type())
#define GST_AUDIODELAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIODELAY,GstAudiodelay))
#define GST_IS_AUDIODELAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIODELAY))
#define GST_AUDIODELAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIODELAY,GstAudiodelayClass))
#define GST_IS_AUDIODELAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIODELAY))
#define GST_AUDIODELAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIODELAY,GstAudiodelayClass))

typedef struct _GstAudiodelay      GstAudiodelay;
typedef struct _GstAudiodelayClass GstAudiodelayClass;

struct _GstAudiodelay {
  GstBaseTransform element;

  guint drywet;
  guint delaytime;
  guint feedback;
  
  /* < private > */
  gint samplerate;
  gint16 *ring_buffer;
  guint max_delaytime;
  guint rb_ptr;
};

struct _GstAudiodelayClass {
  GstBaseTransformClass parent_class;
};

GType gst_audiodelay_get_type (void);

G_END_DECLS

#endif /* __GST_AUDIODELAY_H__ */
