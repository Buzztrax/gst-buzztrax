/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-d.c: decay envelope generator
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
 * SECTION:gstbtenveloped
 * @short_description: decay envelope generator
 *
 * Simple decay envelope.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "envelope-d.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

//-- the class

G_DEFINE_TYPE (GstBtEnvelopeD, gstbt_envelope_d, GSTBT_TYPE_ENVELOPE);

//-- constructor methods

/**
 * gstbt_envelope_d_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelopeD *
gstbt_envelope_d_new (void)
{
  GstBtEnvelopeD *self;

  self = GSTBT_ENVELOPE_D (g_object_new (GSTBT_TYPE_ENVELOPE_D, NULL));
  return (self);
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_d_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 * @decay_time: the decay time in sec
 * @peak_level: peak volume level (0.0 -> 1.0)
 *
 * Initialize the envelope for a new cycle. Adds a short attack (~0.001 s) to
 * avoid clicks.
 */
void
gstbt_envelope_d_setup (GstBtEnvelopeD * self, gint samplerate,
    gdouble decay_time, gdouble peak_level)
{
  GstBtEnvelope *base = (GstBtEnvelope *) self;
  GValue val = { 0, };
  GstController *ctrl = base->ctrl;
  gdouble attack_time = 0.001;
  guint64 attack, decay;

  /* reset states */
  base->value = 0.001;
  base->offset = G_GUINT64_CONSTANT (0);

  /* ensure a < d */
  if (attack_time > decay_time) {
    attack_time = decay_time / 2.0;
  }

  /* samplerate will be one second */
  attack = samplerate * attack_time;
  decay = samplerate * decay_time;
  base->length = decay;

  /* configure envelope */
  g_value_init (&val, G_TYPE_DOUBLE);
  gst_controller_unset_all (ctrl, "value");
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", G_GUINT64_CONSTANT (0), &val);
  g_value_set_double (&val, peak_level);
  gst_controller_set (ctrl, "value", attack, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", decay, &val);
  g_value_unset (&val);
}

//-- virtual methods

static void
gstbt_envelope_d_init (GstBtEnvelopeD * self)
{
}

static void
gstbt_envelope_d_class_init (GstBtEnvelopeDClass * klass)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");
}
