/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavetabsyn.c: wavetable synthesizer
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
 * SECTION:gstwavetab_syn
 * @title: GstBtWaveTabSyn
 * @short_description: wavetable synthesizer
 *
 * A synth that uses the wavetable osc. I picks a cycle from the selected
 * wavetable entry and repeats it as a osc. The offset parameter allows scanning
 * though the waveform.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "wavetabsyn.h"

#define GST_CAT_DEFAULT wave_tab_syn_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  PROP_TUNING,
  // dynamic class properties
  PROP_NOTE,
  PROP_WAVE,
  PROP_OFFSET
};

//-- the class

G_DEFINE_TYPE (GstBtWaveTabSyn, gstbt_wave_tab_syn, GSTBT_TYPE_AUDIO_SYNTH);

//-- audiosynth vmethods

static gboolean
gstbt_wave_tab_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  gstbt_osc_wave_setup (src->osc);

  /* set channels to 1 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels",
          src->osc->channels))
    return FALSE;

  return TRUE;
}

static void
gstbt_wave_tab_syn_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);
  gint16 *d = (gint16 *) GST_BUFFER_DATA (data);
  guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
  gint ch = ((GstBtAudioSynth *) src)->channels;
  guint sz = src->cycle_size;
  guint pos = src->cycle_pos;
  guint p = 0;
  guint64 off = src->offset * (src->duration - src->cycle_size) / 0xFFFF;

  if (src->osc->process) {
    // do we have a unfinished cycle?
    if (pos > 0) {
      guint new_pos;
      p = sz - pos;
      if (p > ct) {
        p = ct;
        new_pos = pos + ct;
      } else {
        new_pos = 0;
      }
      src->osc->process (src->osc, off + pos, p, &d[0]);
      ct -= p;
      pos = new_pos;
    }
    // do full cycles
    while (ct >= sz) {
      src->osc->process (src->osc, off, sz, &d[p * ch]);
      ct -= sz;
      p += sz;
    }
    // fill buffer with partial cycle
    if (ct > 0) {
      src->osc->process (src->osc, off, ct, &d[p * ch]);
      pos += ct;
    }
    src->cycle_pos = pos;
  } else {
    memset (d, 0, ct * sizeof (gint16));
    GST_BUFFER_FLAG_SET (data, GST_BUFFER_FLAG_GAP);
  }
}

//-- gobject vmethods

static void
gstbt_wave_tab_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      g_object_set_property ((GObject *) (src->osc), "wave-callbacks", value);
      break;
    case PROP_TUNING:
      g_object_set_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_NOTE:
      if ((src->note = g_value_get_enum (value))) {
        GST_DEBUG ("new note -> '%d'", src->note);
        gdouble freq =
            gstbt_tone_conversion_translate_from_number (src->n2f, src->note);
        g_object_set (src->osc, "frequency", freq, NULL);
        g_object_get (src->osc, "duration", &src->duration, NULL);

        // this is the chunk that we need to repeat for the selected tone
        src->cycle_size = ((GstBtAudioSynth *) src)->samplerate / freq;
        src->cycle_pos = 0;
      }
      break;
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_OFFSET:
      src->offset = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_tab_syn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      g_object_get_property ((GObject *) (src->osc), "wave-callbacks", value);
      break;
    case PROP_TUNING:
      g_object_get_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_OFFSET:
      g_value_set_uint (value, src->offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_tab_syn_dispose (GObject * object)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  if (src->n2f)
    g_object_unref (src->n2f);
  if (src->osc)
    g_object_unref (src->osc);

  G_OBJECT_CLASS (gstbt_wave_tab_syn_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_wave_tab_syn_init (GstBtWaveTabSyn * src)
{
  src->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);

  /* synth components */
  src->osc = gstbt_osc_wave_new ();
}

static void
gstbt_wave_tab_syn_class_init (GstBtWaveTabSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;

  audio_synth_class->process = gstbt_wave_tab_syn_process;
  audio_synth_class->setup = gstbt_wave_tab_syn_setup;

  gobject_class->set_property = gstbt_wave_tab_syn_set_property;
  gobject_class->get_property = gstbt_wave_tab_syn_get_property;
  gobject_class->dispose = gstbt_wave_tab_syn_dispose;

  // describe us
  gst_element_class_set_details_simple (element_class,
      "WaveTabSyn",
      "Source/Audio",
      "Wavetable synthesizer", "Stefan Sauer <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
  gst_element_class_set_documentation_uri (element_class,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtWaveTabSyn.html");
#endif

  // register own properties
  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks", "Wavetable Callbacks",
          "The wave-table access callbacks",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TUNING,
      g_param_spec_enum ("tuning", "Tuning", "Harmonic tuning",
          GSTBT_TYPE_TONE_CONVERSION_TUNING,
          GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_uint ("wave", "Wave", "Wave index", 1, 200, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OFFSET,
      g_param_spec_uint ("offset", "Offset", "Wave table offset", 0, 0xFFFF, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

//-- plugin

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "wavetabsyn",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "wavetable synthesizer");

  return gst_element_register (plugin, "wavetabsyn", GST_RANK_NONE,
      GSTBT_TYPE_WAVE_TAB_SYN);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "wavetabsyn",
    "Wavetable synthesizer",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
