/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavereplay.c: wavetable player
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
 * SECTION:gstwavereplay
 * @title: GstBtWaveReplay
 * @short_description: wavetable player
 *
 * Plays wavetable assets pre-loaded by the application. Unlike in tracker
 * machines, the wave is implicitly triggered at the start and one can seek in
 * the song without loosing the audio.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "wavereplay.h"

#define GST_CAT_DEFAULT wave_replay_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  // dynamic class properties
  PROP_WAVE,
  PROP_WAVE_LEVEL
};

//-- the class

G_DEFINE_TYPE (GstBtWaveReplay, gstbt_wave_replay, GSTBT_TYPE_AUDIO_SYNTH);

//-- audiosynth vmethods

static gboolean
gstbt_wave_replay_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtWaveReplay *src = ((GstBtWaveReplay *) base);
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  gstbt_osc_wave_setup (src->osc);

  /* set channels to 1 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels",
          src->osc->channels))
    return FALSE;

  return TRUE;
}

static void
gstbt_wave_replay_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtWaveReplay *src = ((GstBtWaveReplay *) base);
  gint16 *d = (gint16 *) GST_BUFFER_DATA (data);
  guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
  guint64 off = gst_util_uint64_scale_round (GST_BUFFER_TIMESTAMP (data),
      base->samplerate, GST_SECOND);

  if (!src->osc->process || !src->osc->process (src->osc, off, ct, d)) {
    gint ch = ((GstBtAudioSynth *) src)->channels;

    memset (d, 0, ct * ch * sizeof (gint16));
    GST_BUFFER_FLAG_SET (data, GST_BUFFER_FLAG_GAP);
  }
}

//-- gobject vmethods

static void
gstbt_wave_replay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      g_object_set_property ((GObject *) (src->osc), "wave-callbacks", value);
      break;
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_WAVE_LEVEL:
      g_object_set_property ((GObject *) (src->osc), "wave-level", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_replay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      g_object_get_property ((GObject *) (src->osc), "wave-callbacks", value);
      break;
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_WAVE_LEVEL:
      g_object_get_property ((GObject *) (src->osc), "wave-level", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_replay_dispose (GObject * object)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  if (src->osc)
    g_object_unref (src->osc);

  G_OBJECT_CLASS (gstbt_wave_replay_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_wave_replay_init (GstBtWaveReplay * src)
{
  /* synth components */
  src->osc = gstbt_osc_wave_new ();
}

static void
gstbt_wave_replay_class_init (GstBtWaveReplayClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;

  audio_synth_class->process = gstbt_wave_replay_process;
  audio_synth_class->setup = gstbt_wave_replay_setup;

  gobject_class->set_property = gstbt_wave_replay_set_property;
  gobject_class->get_property = gstbt_wave_replay_get_property;
  gobject_class->dispose = gstbt_wave_replay_dispose;

  // describe us
  gst_element_class_set_details_simple (element_class,
      "Wave Replay",
      "Source/Audio",
      "Wavetable player", "Stefan Sauer <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
  gst_element_class_set_documentation_uri (element_class,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtWaveReplay.html");
#endif

  // register own properties
  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks", "Wavetable Callbacks",
          "The wave-table access callbacks",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_uint ("wave", "Wave", "Wave index", 1, 200, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE_LEVEL,
      g_param_spec_uint ("wave-level", "Wavelevel", "Wave level index",
          0, 100, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

//-- plugin

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "wavereplay",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "wavetable player");

  return gst_element_register (plugin, "wavereplay", GST_RANK_NONE,
      GSTBT_TYPE_WAVE_REPLAY);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "wavereplay",
    "Wavetable player",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
