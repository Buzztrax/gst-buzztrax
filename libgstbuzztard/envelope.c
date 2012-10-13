/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * envelope.c: decay envelope object
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
 * SECTION:gstbtenvelope
 * @short_description: decay envelope object
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

#include "envelope.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // class properties
  PROP_VALUE = 1,
};

//-- the class

G_DEFINE_TYPE (GstBtEnvelope, gstbt_envelope, G_TYPE_OBJECT);

//-- constructor methods

/**
 * gstbt_envelope_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelope *
gstbt_envelope_new (void)
{
  GstBtEnvelope *self;

  self = GSTBT_ENVELOPE (g_object_new (GSTBT_TYPE_ENVELOPE, NULL));
  return (self);
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 * @decay_time: the decay time in sec
 * @peak_level: peak volume level (0.0 -> 1.0)
 *
 * Initialize the envelope for a new cycle.
 */
void
gstbt_envelope_setup (GstBtEnvelope * self, gint samplerate, gdouble decay_time,
    gdouble peak_level)
{
  GValue val = { 0, };
  GstController *ctrl = self->ctrl;
  guint64 attack, decay;

  /* reset states */
  self->value = 0.001;
  self->offset = G_GUINT64_CONSTANT (0);
  /* samplerate will be one second */
  attack = samplerate / 1000;
  decay = samplerate * decay_time;
  if (attack > decay)
    attack = decay - 10;
  self->length = decay;         // FIXME(ensonic): parent would like to know 
  g_value_init (&val, G_TYPE_DOUBLE);
  gst_controller_unset_all (ctrl, "value");
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", G_GUINT64_CONSTANT (0), &val);
  g_value_set_double (&val, peak_level);
  gst_controller_set (ctrl, "value", attack, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "value", decay, &val);
  g_value_unset (&val);

  /* TODO(ensonic): more advanced envelope
     if(attack_time+decay_time>note_time) note_time=attack_time+decay_time;
     gst_controller_set(ctrl,"value",0,0.0);
     gst_controller_set(ctrl,"value",attack_time,peak_level);
     gst_controller_set(ctrl,"value",attack_time+decay_time,sustain_level);
     gst_controller_set(ctrl,"value",note_time,sustain_level);
     gst_controller_set(ctrl,"value",note_time+release_time,0.0);
   */
}

/**
 * gstbt_envelope_get:
 * @self: the envelope
 * @offset: the time offset to add 
 *
 * Get the currect envelope level and add the time-offset for the next position.
 *
 * Returns: the current level
 */
gdouble
gstbt_envelope_get (GstBtEnvelope * self, guint offset)
{
  gst_controller_sync_values (self->ctrl, self->offset);
  self->offset += offset;
  return self->value;
}

/**
 * gstbt_envelope_is_running:
 * @self: the envelope
 *
 * Checks if the end of the envelop has reached. Can be used to skip audio
 * rendering once the end is reached.
 *
 * Returns: if the envelope is still running
 */
gboolean
gstbt_envelope_is_running (GstBtEnvelope * self)
{
  return self->offset < self->length;
}

//-- virtual methods

static void
gstbt_envelope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_VALUE:
      self->value = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_VALUE:
      // TODO(ensonic): gst_object_sync_values (G_OBJECT (env), self->running_time);
      g_value_set_double (value, self->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_dispose (GObject * object)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;
  self->dispose_has_run = TRUE;

  if (self->ctrl)
    g_object_unref (self->ctrl);

  G_OBJECT_CLASS (gstbt_envelope_parent_class)->dispose (object);
}

static void
gstbt_envelope_init (GstBtEnvelope * self)
{
  self->value = 0.0;
  self->ctrl = gst_controller_new (G_OBJECT (self), "value", NULL);
  gst_controller_set_interpolation_mode (self->ctrl, "value",
      GST_INTERPOLATE_LINEAR);

}

static void
gstbt_envelope_class_init (GstBtEnvelopeClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  gobject_class->set_property = gstbt_envelope_set_property;
  gobject_class->get_property = gstbt_envelope_get_property;
  gobject_class->dispose = gstbt_envelope_dispose;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_VALUE,
      g_param_spec_double ("value", "Value", "Current envelope value",
          0.0, 1.0, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

}
