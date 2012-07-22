/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsynv.cc: c64 sid synthesizer voice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libgstbuzztard/propertymeta.h"
#include "libgstbuzztard/tempo.h"

#include "sidsynv.h"

#define GST_CAT_DEFAULT sid_syn_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  PROP_NOTE = 1,
  PROP_SYNC,
  PROP_RING_MOD,
	PROP_WAVE,
	PROP_PULSE_WIDTH,
	PROP_FILTER_VOICE,
	PROP_ATTACK,
	PROP_DECAY,
	PROP_SUSTAIN,
	PROP_RELEASE  
};

#define GSTBT_TYPE_SID_SYN_WAVE (gst_sid_syn_wave_get_type())
static GType
gst_sid_syn_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SID_SYN_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GSTBT_SID_SYN_WAVE_SAW, "Saw", "saw"},
    {GSTBT_SID_SYN_WAVE_SQUARE, "Square", "square"},
    {GSTBT_SID_SYN_WAVE_NOISE, "Noise", "noise"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSidSynWave", enums);
  }
  return type;
}


G_DEFINE_TYPE (GstBtSidSynV, gstbt_sid_synv, GST_TYPE_OBJECT);

static void
gst_sid_synv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSidSynV *src = GSTBT_SID_SYNV (object);

  switch (prop_id) {
    case PROP_NOTE:
      src->note = (GstBtNote) g_value_get_enum (value);
      if (src->note) {
        src->note_set = TRUE;
      }
      break;
    case PROP_SYNC:
      src->sync = g_value_get_boolean (value);
      break;
    case PROP_RING_MOD:
      src->ringmod = g_value_get_boolean (value);
      break;
    case PROP_WAVE:
      src->wave = (GstBtSimSynWave) g_value_get_enum (value);
      break;
    case PROP_PULSE_WIDTH:
      src->pulse_width = g_value_get_uint (value);
      break;
    case PROP_FILTER_VOICE:
      src->filter = g_value_get_boolean (value);
      break;
    case PROP_ATTACK:
      src->attack = g_value_get_uint (value);
      break;
    case PROP_DECAY:
      src->decay = g_value_get_uint (value);
      break;
    case PROP_SUSTAIN:
      src->sustain = g_value_get_uint (value);
      break;
    case PROP_RELEASE:
      src->release = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sid_synv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtSidSynV *src = GSTBT_SID_SYNV (object);

  switch (prop_id) {
    case PROP_SYNC:
      g_value_set_boolean (value, src->sync);
      break;
    case PROP_RING_MOD:
      g_value_set_boolean (value, src->ringmod);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, src->wave);
      break;
    case PROP_PULSE_WIDTH:
      g_value_set_uint (value, src->pulse_width);
      break;
    case PROP_FILTER_VOICE:
      g_value_set_boolean (value, src->filter);
      break;
    case PROP_ATTACK:
      g_value_set_uint (value, src->attack);
      break;
    case PROP_DECAY:
      g_value_set_uint (value, src->decay);
      break;
    case PROP_SUSTAIN:
      g_value_set_uint (value, src->sustain);
      break;
    case PROP_RELEASE:
      g_value_set_uint (value, src->release);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_sid_synv_init (GstBtSidSynV * self)
{
  self->wave = GSTBT_SID_SYN_WAVE_TRIANGLE;
  self->pulse_width = 2048;
  self->attack = 2;
  self->decay = 2;
  self->sustain = 10;
  self->release = 5;
}

static void
gstbt_sid_synv_class_init (GstBtSidSynVClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  const GParamFlags pflags1 = (GParamFlags) 
      (G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  const GParamFlags pflags2 = (GParamFlags) 
      (G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  gobject_class->set_property = gst_sid_synv_set_property;
  gobject_class->get_property = gst_sid_synv_get_property;

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          pflags1));

  g_object_class_install_property (gobject_class, PROP_SYNC,
      g_param_spec_boolean ("sync", "Sync", "Sync with voice 3", FALSE,
          pflags2));

  g_object_class_install_property (gobject_class, PROP_RING_MOD,
      g_param_spec_boolean ("ringmod", "Ringmod", "Ringmod with voice 3", FALSE, 
          pflags2));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_SID_SYN_WAVE, GSTBT_SID_SYN_WAVE_TRIANGLE, 
          pflags2));

  g_object_class_install_property (gobject_class, PROP_PULSE_WIDTH,
      g_param_spec_uint ("pulse-width", "Pulse Width", "Pulse Width", 0, 4095,
          2048, pflags2));

  g_object_class_install_property (gobject_class, PROP_FILTER_VOICE,
      g_param_spec_boolean ("fiter-voice", "Filter Voice", "Filter Voice",
          FALSE, pflags2));

  g_object_class_install_property (gobject_class, PROP_ATTACK,
      g_param_spec_uint ("attack", "Attack", "Attack", 0, 15, 2, pflags2));

  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_uint ("decay", "Decay", "Decay", 0, 15, 2, pflags2));
  
  g_object_class_install_property (gobject_class, PROP_SUSTAIN,
      g_param_spec_uint ("sustain", "Sustain", "Sustain", 0, 15, 10, pflags2));
  
  g_object_class_install_property (gobject_class, PROP_RELEASE,
      g_param_spec_uint ("release", "Release", "Release", 0, 15, 5, pflags2));
}
