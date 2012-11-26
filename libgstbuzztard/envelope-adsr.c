/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-adsr.c: attack-decay-sustain-release envelope generator
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
 * SECTION:gstbtenvelopeadsr
 * @short_description: attack-decay-sustain-release envelope generator
 *
 * Classic attack-decay-sustain-release envelope.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "envelope-adsr.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

//-- the class

G_DEFINE_TYPE (GstBtEnvelopeADSR, gstbt_envelope_adsr, GSTBT_TYPE_ENVELOPE);

//-- constructor methods

/**
 * gstbt_envelope_adsr_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelopeADSR *
gstbt_envelope_adsr_new (void)
{
  GstBtEnvelopeADSR *self;

  self = GSTBT_ENVELOPE_ADSR (g_object_new (GSTBT_TYPE_ENVELOPE_ADSR, NULL));
  return (self);
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_adsr_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 * @attack_time: the attack time in sec
 * @decay_time: the decay time in sec
 * @note_time: the duration of the note in sec
 * @release_time: the decay time in sec
 * @peak_level: peak volume level (0.0 -> 1.0)
 * @sustain_level: sustain volume level (0.0 -> 1.0)
 *
 * Initialize the envelope for a new cycle. @note_time is the length of the
 * note. @attack_time + @decay_time must be < @note_time otherwise they get
 * scaled down.
 */
void
gstbt_envelope_adsr_setup (GstBtEnvelopeADSR * self, gint samplerate,
    gdouble attack_time, gdouble decay_time, gdouble note_time,
    gdouble release_time, gdouble peak_level, gdouble sustain_level)
{
  GstBtEnvelope *base = (GstBtEnvelope *) self;
  GValue val = { 0, };
  GstController *ctrl = base->ctrl;
  guint64 attack, decay, sustain, release;

  /* reset states */
  base->value = 0.001;
  base->offset = G_GUINT64_CONSTANT (0);

  /* ensure a+d < s */
  if ((attack_time + decay_time) > note_time) {
    gdouble fc = note_time / (attack_time + decay_time);
    attack_time *= fc;
    decay_time *= fc;
  }

  /* samplerate will be one second */
  attack = samplerate * attack_time;
  decay = attack + samplerate * decay_time;
  sustain = samplerate * note_time;
  release = sustain + samplerate * release_time;
  base->length = release;

  /* configure envelope */
  g_value_init (&val, G_TYPE_DOUBLE);
  gst_controller_unset_all (ctrl, "value");
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", G_GUINT64_CONSTANT (0), &val);
  g_value_set_double (&val, peak_level);
  gst_controller_set (ctrl, "value", attack, &val);
  g_value_set_double (&val, sustain_level);
  gst_controller_set (ctrl, "value", decay, &val);
  gst_controller_set (ctrl, "value", sustain, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", release, &val);
  g_value_unset (&val);
}

//-- virtual methods

static void
gstbt_envelope_adsr_init (GstBtEnvelopeADSR * self)
{
}

static void
gstbt_envelope_adsr_class_init (GstBtEnvelopeADSRClass * klass)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");
}
