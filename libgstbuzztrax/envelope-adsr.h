/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-adsr.h: attack-decay-sustain-release envelope generator
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GSTBT_ENVELOPE_ADSR_H__
#define __GSTBT_ENVELOPE_ADSR_H__

#include <gst/gst.h>
#include <libgstbuzztrax/envelope.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_ENVELOPE_ADSR            (gstbt_envelope_adsr_get_type())
#define GSTBT_ENVELOPE_ADSR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_ENVELOPE_ADSR,GstBtEnvelopeADSR))
#define GSTBT_IS_ENVELOPE_ADSR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_ENVELOPE_ADSR))
#define GSTBT_ENVELOPE_ADSR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_ENVELOPE_ADSR,GstBtEnvelopeADSRClass))
#define GSTBT_IS_ENVELOPE_ADSR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_ENVELOPE_ADSR))
#define GSTBT_ENVELOPE_ADSR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_ENVELOPE_ADSR,GstBtEnvelopeADSRClass))

typedef struct _GstBtEnvelopeADSR GstBtEnvelopeADSR;
typedef struct _GstBtEnvelopeADSRClass GstBtEnvelopeADSRClass;

/**
 * GstBtEnvelopeADSR:
 *
 * Class instance data.
 */
struct _GstBtEnvelopeADSR {
  GstBtEnvelope parent;
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
};

struct _GstBtEnvelopeADSRClass {
  GstBtEnvelopeClass parent_class;
};

GType gstbt_envelope_adsr_get_type (void);

GstBtEnvelopeADSR *gstbt_envelope_adsr_new (void);
void gstbt_envelope_adsr_setup (GstBtEnvelopeADSR * self, gint samplerate,
                                gdouble attack_time, gdouble decay_time, gdouble note_time, 
                                gdouble release_time, gdouble peak_level, gdouble sustain_level);

G_END_DECLS

#endif /* __GSTBT_ENVELOPE_ADSR_H__ */
