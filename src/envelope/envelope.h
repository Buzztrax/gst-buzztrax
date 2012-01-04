/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * envelope.h: envelope generator for gstreamer
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

#ifndef __GST_ENVELOPE_H__
#define __GST_ENVELOPE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GST_TYPE_ENVELOPE            (gst_envelope_get_type())
#define GST_ENVELOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ENVELOPE,GstEnvelope))
#define GST_IS_ENVELOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ENVELOPE))
#define GST_ENVELOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_ENVELOPE,GstEnvelopeClass))
#define GST_IS_ENVELOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_ENVELOPE))
#define GST_ENVELOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_ENVELOPE,GstEnvelopeClass))

typedef struct _GstEnvelope GstEnvelope;
typedef struct _GstEnvelopeClass GstEnvelopeClass;

struct _GstEnvelope {
  GObject parent;

  /* parameters */
  gdouble value;

  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
};

struct _GstEnvelopeClass {
  GObjectClass parent_class;
};

GType gst_envelope_get_type(void);

GstEnvelope *gst_envelope_new(void);

G_END_DECLS

#endif /* __GST_ENVELOPE_H__ */
