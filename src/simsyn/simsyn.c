/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * simsyn.c: simple audio synthesizer for gstreamer
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
 * SECTION:gstsimsyn
 * @title: GstBtSimSyn
 * @short_description: simple monophonic audio synthesizer
 *
 * Simple monophonic audio synthesizer with a decay envelope and a
 * state-variable filter.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch simsyn num-buffers=1000 note="c-4" ! autoaudiosink
 * ]| Render a sine wave tone.
 * </refsect2>
 */
/* TODO(ensonic): improvements
 * - implement property-meta iface (see gstbml)
 * - cut-off is now relative to samplerate, needs change
 * - we should do a linear fade down in the last inner_loop block as a anticlick
 *   if(note_count+INNER_LOOP>=note_length) {
 *     ac_f=1.0;
 *     ac_s=1.0/INNER_LOOP;
 *   }
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "libgstbuzztard/propertymeta.h"
#include "simsyn.h"

#define GST_CAT_DEFAULT sim_syn_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_TUNING = 1,
  // dynamic class properties
  PROP_NOTE,
  PROP_WAVE,
  PROP_VOLUME,
  PROP_DECAY,
  PROP_FILTER,
  PROP_CUTOFF,
  PROP_RESONANCE
};

static GstBtAudioSynthClass *parent_class = NULL;

static void gst_sim_syn_base_init (gpointer klass);
static void gst_sim_syn_class_init (GstBtSimSynClass * klass);
static void gst_sim_syn_init (GstBtSimSyn * object, GstBtSimSynClass * klass);

static void gst_sim_syn_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_sim_syn_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_sim_syn_dispose (GObject * object);
static void gst_sim_syn_process (GstBtAudioSynth * base, GstBuffer * data);
static gboolean gst_sim_syn_setup (GstBtAudioSynth * base, GstPad * pad,
    GstCaps * caps);

//-- simsyn implementation

static void
gst_sim_syn_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "simsyn",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "simple audio synthesizer");

  gst_element_class_set_details_simple (element_class,
      "Simple Synth",
      "Source/Audio",
      "Simple audio synthesizer", "Stefan Kost <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
  gst_element_class_set_documentation_uri (element_class,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtSimSyn.html");
#endif
}

static void
gst_sim_syn_class_init (GstBtSimSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;

  parent_class = (GstBtAudioSynthClass *) g_type_class_peek_parent (klass);

  audio_synth_class->process = gst_sim_syn_process;
  audio_synth_class->setup = gst_sim_syn_setup;

  gobject_class->set_property = gst_sim_syn_set_property;
  gobject_class->get_property = gst_sim_syn_get_property;
  gobject_class->dispose = gst_sim_syn_dispose;

  // register own properties
  g_object_class_install_property (gobject_class, PROP_TUNING,
      g_param_spec_enum ("tuning", "Tuning",
          "Harmonic tuning", GSTBT_TYPE_TONE_CONVERSION_TUNING,
          GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_OSC_SYNTH_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of tone",
          0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_double ("decay", "Decay",
          "Volume decay of the tone in seconds", 0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FILTER,
      g_param_spec_enum ("filter", "Filtertype", "Type of audio filter",
          GSTBT_TYPE_FILTER_SVF_TYPE, GSTBT_FILTER_SVF_LOWPASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CUTOFF,
      g_param_spec_double ("cut-off", "Cut-Off",
          "Audio filter cut-off frequency", 0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RESONANCE,
      g_param_spec_double ("resonance", "Resonance", "Audio filter resonance",
          0.7, 25.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

static void
gst_sim_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_TUNING:
      src->tuning = g_value_get_enum (value);
      g_object_set (src->n2f, "tuning", src->tuning, NULL);
      break;
    case PROP_NOTE:
      if ((src->note = g_value_get_enum (value))) {
        GST_DEBUG ("new note -> '%d'", src->note);
        gdouble freq =
            gstbt_tone_conversion_translate_from_number (src->n2f, src->note);
        gstbt_envelope_d_setup (src->volenv,
            ((GstBtAudioSynth *) src)->samplerate, src->decay, src->volume);
        g_object_set (src->osc, "frequency", freq, NULL);
      }
      break;
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_double (value);
      break;
    case PROP_DECAY:
      src->decay = g_value_get_double (value);
      break;
    case PROP_FILTER:
      g_object_set_property ((GObject *) (src->filter), "type", value);
      break;
    case PROP_CUTOFF:
      g_object_set_property ((GObject *) (src->filter), "cut-off", value);
      break;
    case PROP_RESONANCE:
      g_object_set_property ((GObject *) (src->filter), "resonance", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sim_syn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_TUNING:
      g_value_set_enum (value, src->tuning);
      break;
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    case PROP_DECAY:
      g_value_set_double (value, src->decay);
      break;
    case PROP_FILTER:
      g_object_get_property ((GObject *) (src->filter), "type", value);
      break;
    case PROP_CUTOFF:
      g_object_get_property ((GObject *) (src->filter), "cut-off", value);
      break;
    case PROP_RESONANCE:
      g_object_get_property ((GObject *) (src->filter), "resonance", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sim_syn_init (GstBtSimSyn * src, GstBtSimSynClass * g_class)
{
  /* set base parameters */
  src->volume = 0.8;
  src->decay = 0.5;

  src->tuning = GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT;
  src->n2f = gstbt_tone_conversion_new (src->tuning);

  /* synth components */
  src->osc = gstbt_osc_synth_new ();
  src->volenv = gstbt_envelope_d_new ();
  src->filter = gstbt_filter_svf_new ();
  g_object_set (src->osc, "volume-envelope", src->volenv, NULL);
}

static gboolean
gst_sim_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  /* set channels to 1 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels", 1))
    return FALSE;

  return TRUE;
}

static void
gst_sim_syn_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtSimSyn *src = ((GstBtSimSyn *) base);
  gint16 *d = (gint16 *) GST_BUFFER_DATA (data);
  guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;

  if ((src->note != GSTBT_NOTE_NONE)
      && gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv)) {
    src->osc->process (src->osc, ct, d);
    if (src->filter->process)
      src->filter->process (src->filter, ct, d);
  } else {
    memset (d, 0, ct * sizeof (gint16));
    GST_BUFFER_FLAG_SET (data, GST_BUFFER_FLAG_GAP);
  }
}

static void
gst_sim_syn_dispose (GObject * object)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  if (src->n2f)
    g_object_unref (src->n2f);
  if (src->volenv)
    g_object_unref (src->volenv);
  if (src->osc)
    g_object_unref (src->osc);
  if (src->filter)
    g_object_unref (src->filter);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GType
gstbt_sim_syn_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtSimSynClass),
      (GBaseInitFunc) gst_sim_syn_base_init,
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_sim_syn_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstBtSimSyn),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gst_sim_syn_init
    };
    const GInterfaceInfo property_meta_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    type =
        g_type_register_static (GSTBT_TYPE_AUDIO_SYNTH, "GstBtSimSyn",
        &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static (type, GSTBT_TYPE_PROPERTY_META,
        &property_meta_interface_info);
  }
  return type;
}
