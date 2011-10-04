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
/* @todo:
 * - implement property-meta iface (see gstbml)
 * - cut-off is now relative to samplerate, needs change
 *
 * - add a more advanced element (polysyn)
 *   - polyphonic
 *   - multiple oscillators per voice with detune
 *   - real envelopes per osczillator
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>
#include <gst/audio/audio.h>

//#include <gst/childbin/childbin.h>
#if !GST_CHECK_VERSION(0,10,31)
#include <libgstbuzztard/help.h>
#endif
#include "libgstbuzztard/propertymeta.h"
#ifndef HAVE_GST_GSTPRESET_H
#include "libgstbuzztard/preset.h"
#endif
#include "libgstbuzztard/tempo.h"

#include "simsyn.h"

#define M_PI_M2 ( M_PI + M_PI )
#define INNER_LOOP 64

#define GST_CAT_DEFAULT sim_syn_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum {
  // tempo iface
  PROP_BPM = 1,
  PROP_TPB,
  PROP_STPT,
#if !GST_CHECK_VERSION(0,10,31)
  // help iface
  PROP_DOCU_URI,
#endif
  // dynamic class properties
  PROP_NOTE,
  PROP_WAVE,
  PROP_VOLUME,
  PROP_DECAY,
  PROP_FILTER,
  PROP_CUTOFF,
  PROP_RESONANCE
};

static GstStaticPadTemplate gst_sim_syn_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16, " "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );

#define GSTBT_TYPE_SIM_SYN_WAVE (gst_sim_syn_wave_get_type())
static GType
gst_sim_syn_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SIM_SYN_WAVE_SINE, "Sine", "sine"},
    {GSTBT_SIM_SYN_WAVE_SQUARE, "Square", "square"},
    {GSTBT_SIM_SYN_WAVE_SAW, "Saw", "saw"},
    {GSTBT_SIM_SYN_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GSTBT_SIM_SYN_WAVE_SILENCE, "Silence", "silence"},
    {GSTBT_SIM_SYN_WAVE_WHITE_NOISE, "White noise", "white-noise"},
    {GSTBT_SIM_SYN_WAVE_PINK_NOISE, "Pink noise", "pink-noise"},
    {GSTBT_SIM_SYN_WAVE_SINE_TAB, "Sine table", "sine table"},
    {GSTBT_SIM_SYN_WAVE_GAUSSIAN_WHITE_NOISE, "White Gaussian noise", "gaussian-noise"},
    {GSTBT_SIM_SYN_WAVE_RED_NOISE, "Red (brownian) noise", "red-noise"},
    {GSTBT_SIM_SYN_WAVE_BLUE_NOISE, "Blue noise", "blue-noise"},
    {GSTBT_SIM_SYN_WAVE_VIOLET_NOISE, "Violet noise", "violet-noise"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSimSynWave", enums);
  }
  return type;
}

#define GSTBT_TYPE_SIM_SYN_FILTER (gst_sim_syn_filter_get_type())
static GType
gst_sim_syn_filter_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SIM_SYN_FILTER_NONE, "None", "none"},
    {GSTBT_SIM_SYN_FILTER_LOWPASS, "LowPass", "lowpass"},
    {GSTBT_SIM_SYN_FILTER_HIPASS, "HiPass", "hipass"},
    {GSTBT_SIM_SYN_FILTER_BANDPASS, "BandPass", "bandpass"},
    {GSTBT_SIM_SYN_FILTER_BANDSTOP, "BandStop", "bandstop"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSimSynFilter",
        enums);
  }
  return type;
}

static GstBaseSrcClass *parent_class = NULL;

static void gst_sim_syn_base_init (gpointer klass);
static void gst_sim_syn_class_init (GstBtSimSynClass *klass);
static void gst_sim_syn_init (GstBtSimSyn *object, GstBtSimSynClass *klass);

static void gst_sim_syn_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_sim_syn_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_sim_syn_dispose (GObject *object);

static gboolean gst_sim_syn_setcaps (GstBaseSrc * basesrc,
    GstCaps * caps);
static void gst_sim_syn_src_fixate (GstPad * pad, GstCaps * caps);

static gboolean gst_sim_syn_is_seekable (GstBaseSrc * basesrc);
static gboolean gst_sim_syn_do_seek (GstBaseSrc * basesrc,
    GstSegment * segment);
static gboolean gst_sim_syn_query (GstBaseSrc * basesrc, GstQuery * query);

static void gst_sim_syn_change_wave (GstBtSimSyn * src);
static void gst_sim_syn_change_volume (GstBtSimSyn * src);
static void gst_sim_syn_change_filter (GstBtSimSyn * src);

static gboolean gst_sim_syn_start (GstBaseSrc *basesrc);
static GstFlowReturn gst_sim_syn_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);

//-- property meta interface implementations

//-- tempo interface implementations

static void gst_sim_syn_calculate_buffer_frames(GstBtSimSyn *self) {
  const gdouble ticks_per_minute=(gdouble)(self->beats_per_minute*self->ticks_per_beat);

  self->samples_per_buffer=((self->samplerate*60.0)/ticks_per_minute);
  self->ticktime=(GstClockTime)(0.5+((GST_SECOND*60.0)/ticks_per_minute));
  GST_DEBUG("samples_per_buffer=%lf",self->samples_per_buffer);
}

static void gst_sim_syn_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstBtSimSyn *self=GSTBT_SIM_SYN(tempo);
  gboolean changed=FALSE;

  if(beats_per_minute>=0) {
    if(self->beats_per_minute!=beats_per_minute) {
      self->beats_per_minute=(gulong)beats_per_minute;
      g_object_notify(G_OBJECT(self),"beats-per-minute");
      changed=TRUE;
    }
  }
  if(ticks_per_beat>=0) {
    if(self->ticks_per_beat!=ticks_per_beat) {
      self->ticks_per_beat=(gulong)ticks_per_beat;
      g_object_notify(G_OBJECT(self),"ticks-per-beat");
      changed=TRUE;
    }
  }
  if(subticks_per_tick>=0) {
    if(self->subticks_per_tick!=subticks_per_tick) {
      self->subticks_per_tick=(gulong)subticks_per_tick;
      g_object_notify(G_OBJECT(self),"subticks-per-tick");
      changed=TRUE;
    }
  }
  if(changed) {
    GST_DEBUG("changing tempo to %lu BPM  %lu TPB  %lu STPT",self->beats_per_minute,self->ticks_per_beat,self->subticks_per_tick);
    gst_sim_syn_calculate_buffer_frames(self);
  }
}

static void gst_sim_syn_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_sim_syn_tempo_change_tempo;
}

//-- simsyn implementation

static void
gst_sim_syn_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_sim_syn_src_template));
  gst_element_class_set_details_simple (element_class,
      "Simple Synth",
      "Source/Audio",
      "Simple audio synthesizer",
      "Stefan Kost <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
   gst_element_class_set_documentation_uri (element_class,
       "file://"DATADIR""G_DIR_SEPARATOR_S"gtk-doc"G_DIR_SEPARATOR_S"html"G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S""PACKAGE"-GstBtSimSyn.html");
#endif
}

static void
gst_sim_syn_class_init (GstBtSimSynClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;

  parent_class = (GstBaseSrcClass *) g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_sim_syn_set_property;
  gobject_class->get_property = gst_sim_syn_get_property;
  gobject_class->dispose      = gst_sim_syn_dispose;

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_sim_syn_setcaps);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_sim_syn_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_sim_syn_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_sim_syn_query);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_sim_syn_start);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_sim_syn_create);

  // override interface properties
  g_object_class_override_property(gobject_class, PROP_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, PROP_STPT, "subticks-per-tick");

#if !GST_CHECK_VERSION(0,10,31)
  g_object_class_override_property(gobject_class, PROP_DOCU_URI, "documentation-uri");
#endif

  // register own properties

  g_object_class_install_property(gobject_class,PROP_NOTE,
    g_param_spec_string("note", "Musical note", "Musical note (e.g. 'c-3', 'd#4')",
          NULL, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_WAVE,
    g_param_spec_enum("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_SIM_SYN_WAVE, /* enum type */
          GSTBT_SIM_SYN_WAVE_SINE, /* default value */
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_VOLUME,
    g_param_spec_double("volume", "Volume", "Volume of tone",
          0.0, 1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_DECAY,
    g_param_spec_double("decay", "Decay", "Volume decay of the tone in seconds",
          0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_FILTER,
    g_param_spec_enum("filter", "Filtertype", "Type of audio filter",
          GSTBT_TYPE_SIM_SYN_FILTER,    /* enum type */
          GSTBT_SIM_SYN_FILTER_LOWPASS, /* default value */
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_CUTOFF,
    g_param_spec_double("cut-off", "Cut-Off", "Audio filter cut-off frequency",
          0.0, 1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,PROP_RESONANCE,
    g_param_spec_double("resonance", "Resonance", "Audio filter resonance",
          0.7, 25.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
}


static void
gst_sim_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run) return;

  switch (prop_id) {
    case PROP_NOTE:
      g_free (src->note);
      src->note = g_value_dup_string (value);
      if(src->note) {
        gdouble freq;

        GST_DEBUG("new note -> '%s'",src->note);
        freq = gstbt_tone_conversion_translate_from_string (src->n2f, src->note);
        if(freq>=0.0) {
          guint64 attack,decay;
          GValue val = { 0, };

          src->freq=freq;
          /* trigger volume 'envelope' */
          src->volenv->value=0.001;
          src->note_count=0L;
          src->flt_low=src->flt_mid=src->flt_high=0.0;
          /* src->samplerate will be one second */
          attack=src->samplerate/1000;
          decay=src->samplerate*src->decay;
          if(attack>decay) attack=decay-10;
          g_value_init (&val, G_TYPE_DOUBLE);
          gst_controller_unset_all(src->volenv_controller,"value");
          g_value_set_double(&val,0.001); // why is this not 0.0?
          gst_controller_set(src->volenv_controller,"value",G_GUINT64_CONSTANT(0),&val);
          g_value_set_double(&val,1.0);
          gst_controller_set(src->volenv_controller,"value",attack,&val);
          g_value_set_double(&val,0.0);
          gst_controller_set(src->volenv_controller,"value",decay,&val);

          /* @todo: more advanced envelope
          if(attack_time+decay_time>note_time) note_time=attack_time+decay_time;
          gst_controller_set(src->volenv_controller,"value",0,0.0);
          gst_controller_set(src->volenv_controller,"value",attack_time,1.0);
          gst_controller_set(src->volenv_controller,"value",attack_time+decay_time,sustain_level);
          gst_controller_set(src->volenv_controller,"value",note_time,sustain_level);
          gst_controller_set(src->volenv_controller,"value",note_time+release_time,0.0);
          */
        }
        else {
          src->freq=0.0;
          /* stop volume 'envelope' */
          src->volenv->value=0.0;
        }
      }
      break;
    case PROP_WAVE:
      //GST_INFO("change wave %d -> %d",g_value_get_enum (value),src->wave);
      src->wave = g_value_get_enum (value);
      gst_sim_syn_change_wave (src);
      break;
    case PROP_VOLUME:
      //GST_INFO("change volume %lf -> %lf",g_value_get_double (value),src->volume);
      src->volume = g_value_get_double (value);
      gst_sim_syn_change_volume (src);
      break;
    case PROP_DECAY:
      //GST_INFO("change decay %lf -> %lf",g_value_get_double (value),src->decay);
      src->decay = g_value_get_double (value);
      break;
    case PROP_FILTER:
      //GST_INFO("change filter %d -> %d",g_value_get_enum (value),src->filter);
      src->filter = g_value_get_enum (value);
      gst_sim_syn_change_filter (src);
      break;
    case PROP_CUTOFF:
      //GST_INFO("change cutoff %lf -> %lf",g_value_get_double (value),src->cutoff);
      src->cutoff = g_value_get_double (value);
      break;
    case PROP_RESONANCE:
      //GST_INFO("change resonance %lf -> %lf",g_value_get_double (value),src->resonance);
      src->resonance = g_value_get_double (value);
      src->flt_res=1.0/src->resonance;
      break;
	// tempo iface
    case PROP_BPM:
    case PROP_TPB:
    case PROP_STPT:
	  GST_WARNING("use gstbt_tempo_change_tempo()");
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

  if (src->dispose_has_run) return;

  switch (prop_id) {
    /*case PROP_NOTE:
      g_value_set_string (value, src->note);
      break;*/
    case PROP_WAVE:
      g_value_set_enum (value, src->wave);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    case PROP_DECAY:
      g_value_set_double (value, src->decay);
      break;
    case PROP_FILTER:
      g_value_set_enum (value, src->filter);
      break;
    case PROP_CUTOFF:
      g_value_set_double (value, src->cutoff);
      break;
    case PROP_RESONANCE:
      g_value_set_double (value, src->resonance);
      break;
	// tempo iface
    case PROP_BPM:
      g_value_set_ulong(value, src->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong(value, src->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong(value, src->subticks_per_tick);
      break;
#if !GST_CHECK_VERSION(0,10,31)
	// help iface
	case PROP_DOCU_URI:
	  g_value_set_static_string(value, "file://"DATADIR""G_DIR_SEPARATOR_S"gtk-doc"G_DIR_SEPARATOR_S"html"G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S""PACKAGE"-GstBtSimSyn.html");
	  break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sim_syn_init (GstBtSimSyn * src, GstBtSimSynClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gst_sim_syn_src_fixate);

  src->samplerate = GST_AUDIO_DEF_RATE;
  src->beats_per_minute=120;
  src->ticks_per_beat=4;
  src->subticks_per_tick=1;
  gst_sim_syn_calculate_buffer_frames (src);
  src->generate_samples_per_buffer = (guint)(0.5+src->samples_per_buffer);

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

  /* set base parameters */
  src->volume = 0.8;
  src->freq = 0.0;
  src->note = NULL;
  src->decay = 0.5;
  src->n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_CROMATIC);

  /* set the waveform */
  src->wave = GSTBT_SIM_SYN_WAVE_SINE;
  gst_sim_syn_change_wave (src);

  /* add a volume envelope generator */
  src->volenv=gstbt_envelope_new ();
  src->volenv_controller=gst_controller_new (G_OBJECT(src->volenv), "value", NULL);
  gst_controller_set_interpolation_mode (src->volenv_controller, "value", GST_INTERPOLATE_LINEAR);

  /* set filter */
  src->filter = GSTBT_SIM_SYN_FILTER_LOWPASS;
  src->cutoff = 0.8;
  src->resonance = 0.8;
  src->flt_res=1.0/src->resonance;
  gst_sim_syn_change_filter (src);

}

static void
gst_sim_syn_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (GST_PAD_PARENT (pad));
  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);
}

static gboolean
gst_sim_syn_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (basesrc);
  const GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "rate", &src->samplerate);

  return ret;
}

static gboolean
gst_sim_syn_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_DEFAULT:
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val = gst_util_uint64_scale_int (src_val, GST_SECOND,
                src->samplerate);
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val = gst_util_uint64_scale_int (src_val, src->samplerate,
                GST_SECOND);
              break;
            default:
              goto error;
          }
          break;
        default:
          goto error;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      res = TRUE;
      break;
    }
    default:
      break;
  }

  return res;
  /* ERROR */
error:
  {
    GST_DEBUG_OBJECT (src, "query failed");
    return FALSE;
  }
}

/* Wave generators */

static void
gst_sim_syn_create_sine (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble step, amp, ampf, accumulator;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0;
  accumulator = src->accumulator;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      /* @todo: move out of inner loop? */
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      samples[i] = (gint16) (sin (accumulator) * amp);
    }
  }
  src->accumulator = accumulator;
}

static void
gst_sim_syn_create_square (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble step, amp, ampf, accumulator;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0;
  accumulator = src->accumulator;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      samples[i] = (gint16) ((accumulator < M_PI) ? amp : -amp);
    }
  }
  src->accumulator = accumulator;
}

static void
gst_sim_syn_create_saw (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble step, amp, ampf, accumulator;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0 / M_PI;
  accumulator = src->accumulator;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      if (accumulator < M_PI) {
        samples[i] = (gint16) (accumulator * amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - accumulator) * -amp);
      }
    }
  }
  src->accumulator = accumulator;
}

static void
gst_sim_syn_create_triangle (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble step, amp, ampf, accumulator;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0 / M_PI;
  accumulator = src->accumulator;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      if (accumulator < (M_PI * 0.5)) {
        samples[i] = (gint16) (accumulator * amp);
      } else if (accumulator < (M_PI * 1.5)) {
        samples[i] = (gint16) ((accumulator - M_PI) * -amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - accumulator) * -amp);
      }
    }
  }
  src->accumulator = accumulator;
}

static void
gst_sim_syn_create_silence (GstBtSimSyn * src, gint16 * samples)
{
  memset (samples, 0, src->generate_samples_per_buffer * sizeof (gint16));
}

static void
gst_sim_syn_create_white_noise (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble amp, ampf;

  ampf = src->volume * 65535.0;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      samples[i] = (gint16) (32768 - (amp * rand () / (RAND_MAX + 1.0)));
    }
  }
}

/* pink noise calculation is based on
 * http://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c
 * which has been released under public domain
 * Many thanks Phil!
 */
static void
gst_sim_syn_init_pink_noise (GstBtSimSyn * src)
{
  guint num_rows = 12;           /* arbitrary: 1 .. PINK_MAX_RANDOM_ROWS */
  glong pmax;

  src->pink.index = 0;
  src->pink.index_mask = (1 << num_rows) - 1;
  /* calculate maximum possible signed random value.
   * Extra 1 for white noise always added. */
  pmax = (num_rows + 1) * (1 << (PINK_RANDOM_BITS - 1));
  src->pink.scalar = 1.0f / pmax;
  /* Initialize rows. */
  memset (src->pink.rows, 0, sizeof(src->pink.rows));
  src->pink.running_sum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
static gfloat
gst_sim_syn_generate_pink_noise_value (GstBtPinkNoise * pink)
{
  glong new_random;
  glong sum;

  /* Increment and mask index. */
  pink->index = (pink->index + 1) & pink->index_mask;

  /* If index is zero, don't update any random values. */
  if (pink->index != 0) {
    /* Determine how many trailing zeros in PinkIndex. */
    /* This algorithm will hang if n==0 so test first. */
    gint num_zeros = 0;
    gint n = pink->index;

    while ((n & 1) == 0) {
      n = n >> 1;
      num_zeros++;
    }

    /* Replace the indexed ROWS random value.
     * Subtract and add back to RunningSum instead of adding all the random
     * values together. Only one changes each time.
     */
    pink->running_sum -= pink->rows[num_zeros];
    //new_random = ((glong)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
    new_random = 32768.0 - (65536.0 * (gulong) rand () / (RAND_MAX + 1.0));
    pink->running_sum += new_random;
    pink->rows[num_zeros] = new_random;
  }

  /* Add extra white noise value. */
  new_random = 32768.0 - (65536.0 * (gulong) rand () / (RAND_MAX + 1.0));
  sum = pink->running_sum + new_random;

  /* Scale to range of -1.0 to 0.9999. */
  return (pink->scalar * sum);
}

static void
gst_sim_syn_create_pink_noise (GstBtSimSyn * src, gint16 * samples)
{
  guint i=0, j, ct=src->generate_samples_per_buffer;
  GstBtPinkNoise * pink = &src->pink;
  gdouble amp, ampf;

  ampf = src->volume * 32767.0;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      samples[i] = (gint16) (gst_sim_syn_generate_pink_noise_value (pink) * amp);
    }
  }
}

static void
gst_sim_syn_init_sine_table (GstBtSimSyn * src)
{
  guint i;
  gdouble ang = 0.0;
  gdouble step = M_PI_M2 / (gdouble)WAVE_TABLE_SIZE;
  gdouble amp = src->volume * 32767.0;
  gint16 * __restrict__ wave_table = src->wave_table;

  for (i = 0; i <  WAVE_TABLE_SIZE; i++) {
    wave_table[i] = (gint16) (sin (ang) * amp);
    ang += step;
  }
}

static void
gst_sim_syn_create_sine_table (GstBtSimSyn * src, gint16 * samples)
{
  guint i, ct = src->generate_samples_per_buffer;
  gdouble step, scl, accumulator;
  gint16 * __restrict__ wave_table = src->wave_table;

  step = M_PI_M2 * src->freq / src->samplerate;
  scl = (gdouble)WAVE_TABLE_SIZE /  M_PI_M2;
  accumulator = src->accumulator;

  for (i = 0; i < ct; i++) {
    /* @todo: add envelope */
    accumulator += step;
    samples[i] = wave_table[((guint) (accumulator * scl)) & (WAVE_TABLE_SIZE - 1)];
  }
  while (G_UNLIKELY (accumulator >= M_PI_M2)) {
    accumulator -= M_PI_M2;
  }
  src->accumulator = accumulator;
}

/* Gaussian white noise using Box-Muller algorithm.  unit variance
 * normally-distributed random numbers are generated in pairs as the real
 * and imaginary parts of a compex random variable with
 * uniformly-distributed argument and \chi^{2}-distributed modulus.
 */
static void
gst_sim_syn_create_gaussian_white_noise (GstBtSimSyn * src, gint16 * samples)
{
  gint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble amp, ampf;
  
  ampf = src->volume * 32767.0;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j+=2) {
      gdouble mag = sqrt (-2 * log (1.0 - rand () / (RAND_MAX + 1.0)));
      gdouble phs = M_PI_M2 * rand () / (RAND_MAX + 1.0);
  
      samples[i++] = (gint16) (amp * mag * cos (phs));
      if(i<ct)
        samples[i++] = (gint16) (amp * mag * sin (phs));
    }
  }
}

static void
gst_sim_syn_create_red_noise (GstBtSimSyn * src, gint16 * samples)
{
  gint i=0, j, ct=src->generate_samples_per_buffer;
  gdouble amp, ampf;
  gdouble state = src->red.state;
  
  ampf = src->volume * 32767.0;

  while (i < ct) {
    /* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
    amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      while (TRUE) {
        gdouble r = 1.0 - (2.0 * rand () / (RAND_MAX + 1.0));
        state += r;
        if (state<-8.0f || state>8.0f) state -= r;
        else break;
      }
      samples[i] = (gint16) (amp * state * 0.0625f); /* /16.0 */
    }
  }
  src->red.state = state;
}

static void
gst_sim_syn_create_blue_noise (GstBtSimSyn * src, gint16 * samples)
{
  gint i, ct=src->generate_samples_per_buffer;
  static gdouble flip=1.0;

  gst_sim_syn_create_pink_noise (src, samples);
  for (i = 0; i < ct; i++) {
    samples[i] *= flip;
    flip *= -1.0;
  }
}

static void
gst_sim_syn_create_violet_noise (GstBtSimSyn * src, gint16 * samples)
{
  gint i, ct=src->generate_samples_per_buffer;
  static gdouble flip=1.0;

  gst_sim_syn_create_red_noise (src, samples);
  for (i = 0; i < ct; i++) {
    samples[i] *= flip;
    flip *= -1.0;
  }
}


/* Filters */

static void
gst_sim_syn_filter_lowpass (GstBtSimSyn * src, gint16 * samples)
{
  guint i, ct = src->generate_samples_per_buffer;
  gdouble flt_low = src->flt_low;
  gdouble flt_mid = src->flt_mid;
  gdouble flt_high = src->flt_high;
  gdouble flt_res = src->flt_res;
  gdouble cutoff = src->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble)samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] = (gint16)CLAMP((glong)flt_low, G_MININT16, G_MAXINT16);
  }
  src->flt_low = flt_low;
  src->flt_mid = flt_mid;
  src->flt_high = flt_high;
}

static void
gst_sim_syn_filter_hipass (GstBtSimSyn * src, gint16 * samples)
{
  guint i, ct = src->generate_samples_per_buffer;
  gdouble flt_low = src->flt_low;
  gdouble flt_mid = src->flt_mid;
  gdouble flt_high = src->flt_high;
  gdouble flt_res = src->flt_res;
  gdouble cutoff = src->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble)samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] = (gint16)CLAMP((glong)flt_high, G_MININT16, G_MAXINT16);
  }
  src->flt_low = flt_low;
  src->flt_mid = flt_mid;
  src->flt_high = flt_high;
}

static void
gst_sim_syn_filter_bandpass (GstBtSimSyn * src, gint16 * samples)
{
  guint i, ct = src->generate_samples_per_buffer;
  gdouble flt_low = src->flt_low;
  gdouble flt_mid = src->flt_mid;
  gdouble flt_high = src->flt_high;
  gdouble flt_res = src->flt_res;
  gdouble cutoff = src->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble)samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i]=(gint16)CLAMP((glong)flt_mid, G_MININT16, G_MAXINT16);
  }
  src->flt_low = flt_low;
  src->flt_mid = flt_mid;
  src->flt_high = flt_high;
}

static void
gst_sim_syn_filter_bandstop (GstBtSimSyn * src, gint16 * samples)
{
  guint i, ct = src->generate_samples_per_buffer;
  gdouble flt_low = src->flt_low;
  gdouble flt_mid = src->flt_mid;
  gdouble flt_high = src->flt_high;
  gdouble flt_res = src->flt_res;
  gdouble cutoff = src->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble)samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i]=(gint16)CLAMP((glong)(flt_low+flt_high), G_MININT16, G_MAXINT16);
  }
  src->flt_low = flt_low;
  src->flt_mid = flt_mid;
  src->flt_high = flt_high;
}

/*
 * gst_sim_syn_change_wave:
 * Assign function pointer of wave genrator.
 */
static void
gst_sim_syn_change_wave (GstBtSimSyn * src)
{
  switch (src->wave) {
    case GSTBT_SIM_SYN_WAVE_SINE:
      src->process = gst_sim_syn_create_sine;
      break;
    case GSTBT_SIM_SYN_WAVE_SQUARE:
      src->process = gst_sim_syn_create_square;
      break;
    case GSTBT_SIM_SYN_WAVE_SAW:
      src->process = gst_sim_syn_create_saw;
      break;
    case GSTBT_SIM_SYN_WAVE_TRIANGLE:
      src->process = gst_sim_syn_create_triangle;
      break;
    case GSTBT_SIM_SYN_WAVE_SILENCE:
      src->process = gst_sim_syn_create_silence;
      break;
    case GSTBT_SIM_SYN_WAVE_WHITE_NOISE:
      src->process = gst_sim_syn_create_white_noise;
      break;
    case GSTBT_SIM_SYN_WAVE_PINK_NOISE:
      gst_sim_syn_init_pink_noise (src);
      src->process = gst_sim_syn_create_pink_noise;
      break;
    case GSTBT_SIM_SYN_WAVE_SINE_TAB:
      gst_sim_syn_init_sine_table (src);
      src->process = gst_sim_syn_create_sine_table;
      break;
    case GSTBT_SIM_SYN_WAVE_GAUSSIAN_WHITE_NOISE:
      src->process = gst_sim_syn_create_gaussian_white_noise;
      break;
    case GSTBT_SIM_SYN_WAVE_RED_NOISE:
      src->red.state = 0.0;
      src->process = gst_sim_syn_create_red_noise;
      break;
    case GSTBT_SIM_SYN_WAVE_BLUE_NOISE:
      gst_sim_syn_init_pink_noise (src);
      src->process = gst_sim_syn_create_blue_noise;
      break;
    case GSTBT_SIM_SYN_WAVE_VIOLET_NOISE:
      src->red.state = 0.0;
      src->process = gst_sim_syn_create_violet_noise;
      break;
    default:
      GST_ERROR ("invalid wave-form: %d",src->wave);
      break;
  }
}

/*
 * gst_sim_syn_change_volume:
 * Recalc wave tables for precalculated waves.
 */
static void
gst_sim_syn_change_volume (GstBtSimSyn * src)
{
  switch (src->wave) {
    case GSTBT_SIM_SYN_WAVE_SINE_TAB:
      gst_sim_syn_init_sine_table (src);
      break;
    default:
      break;
  }
}

/*
 * gst_sim_syn_change_filter:
 * Assign filter type function
 */
static void
gst_sim_syn_change_filter (GstBtSimSyn * src)
{
  switch (src->filter) {
    case GSTBT_SIM_SYN_FILTER_NONE:
      src->apply_filter = NULL;
      break;
    case GSTBT_SIM_SYN_FILTER_LOWPASS:
      src->apply_filter = gst_sim_syn_filter_lowpass;
      break;
    case GSTBT_SIM_SYN_FILTER_HIPASS:
      src->apply_filter = gst_sim_syn_filter_hipass;
      break;
    case GSTBT_SIM_SYN_FILTER_BANDPASS:
      src->apply_filter = gst_sim_syn_filter_bandpass;
      break;
    case GSTBT_SIM_SYN_FILTER_BANDSTOP:
      src->apply_filter = gst_sim_syn_filter_bandstop;
      break;
    default:
      GST_ERROR ("invalid filter-type: %d",src->filter);
      break;
  }
}

static gboolean
gst_sim_syn_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (basesrc);
  GstClockTime time;

  time = segment->last_stop;
  src->reverse = (segment->rate<0.0);
  src->running_time = time;

  /* now move to the time indicated */
  src->n_samples = gst_util_uint64_scale_int(time, src->samplerate, GST_SECOND);

  if (!src->reverse) {
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      segment->time = segment->start;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      time = segment->stop;
      src->n_samples_stop = gst_util_uint64_scale_int(time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
  } else {
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      segment->time = segment->stop;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      time = segment->start;
      src->n_samples_stop = gst_util_uint64_scale_int(time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
  }
  src->eos_reached = FALSE;

  GST_DEBUG_OBJECT(src,"seek from %"GST_TIME_FORMAT" to %"GST_TIME_FORMAT" cur %"GST_TIME_FORMAT" rate %lf",
    GST_TIME_ARGS(segment->start),GST_TIME_ARGS(segment->stop),GST_TIME_ARGS(segment->last_stop),segment->rate);

  return TRUE;
}

static gboolean
gst_sim_syn_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static gboolean
gst_sim_syn_start (GstBaseSrc *basesrc)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (basesrc);

  src->n_samples=G_GINT64_CONSTANT(0);
  src->running_time=G_GUINT64_CONSTANT(0);

  return(TRUE);
}

static GstFlowReturn
gst_sim_syn_create (GstBaseSrc * basesrc, guint64 offset, guint length,
    GstBuffer ** buffer)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (basesrc);
  GstFlowReturn res;
  GstBuffer *buf;
  GstClockTime next_running_time;
  gint64 n_samples;
  gdouble samples_done;
  guint samples_per_buffer;
  gboolean partial_buffer=FALSE;

  if (G_UNLIKELY(src->eos_reached)) {
    GST_WARNING_OBJECT(src,"EOS reached");
    return GST_FLOW_UNEXPECTED;
  }

  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done = (gdouble)src->running_time*(gdouble)src->samplerate/(gdouble)GST_SECOND;
  if (!src->reverse) {
    samples_per_buffer=(guint)(src->samples_per_buffer+(samples_done-(gdouble)src->n_samples));
  } else {
    samples_per_buffer=(guint)(src->samples_per_buffer+((gdouble)src->n_samples-samples_done));
  }

  /*
  GST_DEBUG_OBJECT(src,"samples_done=%lf, src->n_samples=%lf, samples_per_buffer=%u",
    samples_done,(gdouble)src->n_samples,samples_per_buffer);
  GST_DEBUG("  samplers-per-buffer = %7d (%8.3lf), length = %u",samples_per_buffer,src->samples_per_buffer,length);
  */

  /* check for eos */
  if (src->check_eos) {
    if (!src->reverse) {
      partial_buffer=((src->n_samples_stop >= src->n_samples) &&
        (src->n_samples_stop < src->n_samples + samples_per_buffer));
    } else {
      partial_buffer=((src->n_samples_stop < src->n_samples) &&
        (src->n_samples_stop >= src->n_samples - samples_per_buffer));
    }
  }

  if (G_UNLIKELY(partial_buffer)) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer = (guint)(src->n_samples_stop - src->n_samples);
    GST_INFO_OBJECT(src,"partial buffer: %u", src->generate_samples_per_buffer);
    if(G_UNLIKELY(!src->generate_samples_per_buffer)) {
      GST_WARNING_OBJECT(src,"0 samples left -> EOS reached");
      src->eos_reached = TRUE;
      return GST_FLOW_UNEXPECTED;
    }
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples = src->n_samples + (src->reverse ? (-samples_per_buffer) : samples_per_buffer);
  }
  next_running_time = src->running_time + (src->reverse ? (-src->ticktime) : src->ticktime);

  /* allocate a new buffer suitable for this pad */
  res = gst_pad_alloc_buffer_and_set_caps (basesrc->srcpad, src->n_samples,
      src->generate_samples_per_buffer * sizeof (gint16),
      GST_PAD_CAPS (basesrc->srcpad), &buf);
  if (res != GST_FLOW_OK) {
    return res;
  }

  if (!src->reverse) {
    GST_BUFFER_TIMESTAMP (buf) = src->running_time;
    GST_BUFFER_DURATION (buf) = next_running_time - src->running_time;
    GST_BUFFER_OFFSET (buf) = src->n_samples;
    GST_BUFFER_OFFSET_END (buf) = n_samples;
  } else {
    GST_BUFFER_TIMESTAMP (buf) = next_running_time;
    GST_BUFFER_DURATION (buf) = src->running_time - next_running_time;
    GST_BUFFER_OFFSET (buf) = n_samples;
    GST_BUFFER_OFFSET_END (buf) = src->n_samples;
  }

  gst_object_sync_values (G_OBJECT (src), GST_BUFFER_TIMESTAMP (buf));

  GST_DEBUG("n_samples %12"G_GUINT64_FORMAT", d_samples %6u running_time %"GST_TIME_FORMAT", next_time %"GST_TIME_FORMAT", duration %"GST_TIME_FORMAT,
    src->n_samples,src->generate_samples_per_buffer,
    GST_TIME_ARGS(src->running_time),GST_TIME_ARGS(next_running_time),
    GST_TIME_ARGS(GST_BUFFER_DURATION (buf)));

  src->running_time = next_running_time;
  src->n_samples = n_samples;

  if ((src->freq != 0.0) && (src->volenv->value > 0.0001)) {
    //GST_INFO("volenv : %6.4lf,  note-time : %6d",src->volenv->value,src->note_count);

    src->process (src, (gint16 *) GST_BUFFER_DATA (buf));
    if (src->apply_filter)
      src->apply_filter (src, (gint16 *) GST_BUFFER_DATA (buf));
  } else {
    gst_sim_syn_create_silence (src, (gint16 *) GST_BUFFER_DATA (buf));
    GST_BUFFER_FLAG_SET (buf,GST_BUFFER_FLAG_GAP);
  }

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
gst_sim_syn_dispose (GObject *object)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run) return;
  src->dispose_has_run = TRUE;

  if (src->n2f) g_object_unref (src->n2f);
  if (src->volenv_controller) g_object_unref (src->volenv_controller);
  if (src->volenv) g_object_unref (src->volenv);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

GType gstbt_sim_syn_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY(!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtSimSynClass),
      (GBaseInitFunc)gst_sim_syn_base_init,
      NULL,		          /* base_finalize */
      (GClassInitFunc)gst_sim_syn_class_init,
      NULL,		          /* class_finalize */
      NULL,               /* class_data */
      sizeof (GstBtSimSyn),
      0,                  /* n_preallocs */
      (GInstanceInitFunc) gst_sim_syn_init
    };
    const GInterfaceInfo property_meta_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) gst_sim_syn_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
#if !GST_CHECK_VERSION(0,10,31)
    const GInterfaceInfo help_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
#endif
    const GInterfaceInfo preset_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

    type = g_type_register_static(GST_TYPE_BASE_SRC, "GstBtSimSyn", &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static(type, GSTBT_TYPE_PROPERTY_META, &property_meta_interface_info);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
#if !GST_CHECK_VERSION(0,10,31)
    g_type_add_interface_static(type, GSTBT_TYPE_HELP, &help_interface_info);
#endif
    g_type_add_interface_static(type, GST_TYPE_PRESET, &preset_interface_info);
  }
  return type;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "simsyn", GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "simple audio synthesizer");

  /* initialize gst controller library */
  gst_controller_init(NULL,NULL);

  return gst_element_register (plugin, "simsyn", GST_RANK_NONE, GSTBT_TYPE_SIM_SYN);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "simsyn",
    "Simple audio synthesizer",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN);
