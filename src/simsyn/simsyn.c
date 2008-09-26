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
 * @short_description: simple monophonic audio synthesizer
 *
 * Simple monophonic audio synthesizer with a decay envelope and a
 * state-variable filter.
 */
/* @todo:
 * - implement property-meta iface (see gstbml)
 * - cut-off is now relative to samplerate, needs change
 * - get rid of samples-per-buffer, is-live properties
 *
 * - add polyphonic element
 *   - simsyn-mono, simsyn-poly
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
#include "libgstbuzztard/help.h"
#include "libgstbuzztard/propertymeta.h"
#include "libgstbuzztard/preset.h"
#include "libgstbuzztard/tempo.h"

#include "simsyn.h"

#define M_PI_M2 ( M_PI + M_PI )
#define INNER_LOOP 32

#define GST_CAT_DEFAULT sim_syn_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static const GstElementDetails gst_sim_syn_details =
GST_ELEMENT_DETAILS ("Simple Synth",
  "Source/Audio",
  "Simple audio synthesizer",
  "Stefan Kost <ensonic@users.sf.net>");


enum {
  // static class properties
  PROP_SAMPLES_PER_BUFFER=1,
  PROP_IS_LIVE,
  // dynamic class properties
  PROP_NOTE,
  PROP_WAVE,
  PROP_VOLUME,
  PROP_DECAY,
  PROP_FILTER,
  PROP_CUTOFF,
  PROP_RESONANCE,
  // tempo iface
  PROP_BPM,
  PROP_TPB,
  PROP_STPT,
  // help iface
  PROP_DOCU_URI
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

#define GST_TYPE_SIM_SYN_WAVE (gst_sim_syn_wave_get_type())
static GType
gst_sim_syn_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GST_SIM_SYN_WAVE_SINE, "Sine", "sine"},
    {GST_SIM_SYN_WAVE_SQUARE, "Square", "square"},
    {GST_SIM_SYN_WAVE_SAW, "Saw", "saw"},
    {GST_SIM_SYN_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GST_SIM_SYN_WAVE_SILENCE, "Silence", "silence"},
    {GST_SIM_SYN_WAVE_WHITE_NOISE, "White noise", "white-noise"},
    {GST_SIM_SYN_WAVE_PINK_NOISE, "Pink noise", "pink-noise"},
    {GST_SIM_SYN_WAVE_SINE_TAB, "Sine table", "sine table"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstSimSynWave", enums);
  }
  return type;
}

#define GST_TYPE_SIM_SYN_FILTER (gst_sim_syn_filter_get_type())
static GType
gst_sim_syn_filter_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GST_SIM_SYN_FILTER_NONE, "None", "none"},
    {GST_SIM_SYN_FILTER_LOWPASS, "LowPass", "lowpass"},
    {GST_SIM_SYN_FILTER_HIPASS, "HiPass", "hipass"},
    {GST_SIM_SYN_FILTER_BANDPASS, "BandPass", "bandpass"},
    {GST_SIM_SYN_FILTER_BANDSTOP, "BandStop", "bandstop"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstSimSynFilter",
        enums);
  }
  return type;
}

static GstBaseSrcClass *parent_class = NULL;

static void gst_sim_syn_base_init (gpointer klass);
static void gst_sim_syn_class_init (GstSimSynClass *klass);
static void gst_sim_syn_init (GstSimSyn *object, GstSimSynClass *klass);

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
static gboolean gst_sim_syn_query (GstBaseSrc * basesrc,
    GstQuery * query);

static gboolean gst_sim_syn_send_event (GstElement *elem, GstEvent *event);

static void gst_sim_syn_change_wave (GstSimSyn * src);
static void gst_sim_syn_change_volume (GstSimSyn * src);
static void gst_sim_syn_change_filter (GstSimSyn * src);

static void gst_sim_syn_get_times (GstBaseSrc * basesrc,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static gboolean gst_sim_syn_start (GstBaseSrc *basesrc);
static GstFlowReturn gst_sim_syn_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);

//-- property meta interface implementations

//-- tempo interface implementations

static void gst_sim_syn_calculate_buffer_frames(GstSimSyn *self) {
  const gdouble ticks_per_minute=(gdouble)(self->beats_per_minute*self->ticks_per_beat);
  
  self->samples_per_buffer=((self->samplerate*60.0)/ticks_per_minute);
  self->ticktime=(GstClockTime)(0.5+((GST_SECOND*60.0)/ticks_per_minute));
  g_object_notify(G_OBJECT(self),"samplesperbuffer");
  GST_INFO("samples_per_buffer=%lf",self->samples_per_buffer);
}

static void gst_sim_syn_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstSimSyn *self=GST_SIM_SYN(tempo);
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
    GST_DEBUG("changing tempo to %d BPM  %d TPB  %d STPT",self->beats_per_minute,self->ticks_per_beat,self->subticks_per_tick);
    gst_sim_syn_calculate_buffer_frames(self);
  }
}

static void gst_sim_syn_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_sim_syn_tempo_change_tempo;
}

//-- preset interface implementations

//-- simsyn implementation

static void
gst_sim_syn_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_sim_syn_src_template));
  gst_element_class_set_details (element_class, &gst_sim_syn_details);
}

static void
gst_sim_syn_class_init (GstSimSynClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GParamSpec *paramspec;

  parent_class = (GstBaseSrcClass *) g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_sim_syn_set_property;
  gobject_class->get_property = gst_sim_syn_get_property;
  gobject_class->dispose      = gst_sim_syn_dispose;

  gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_sim_syn_send_event);

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_sim_syn_setcaps);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_sim_syn_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_sim_syn_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_sim_syn_query);
  gstbasesrc_class->get_times = GST_DEBUG_FUNCPTR (gst_sim_syn_get_times);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_sim_syn_start);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_sim_syn_create);

  // override interface properties
  g_object_class_override_property(gobject_class, PROP_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, PROP_STPT, "subticks-per-tick");

  g_object_class_override_property(gobject_class, PROP_DOCU_URI, "documentation-uri");

  // register own properties

  g_object_class_install_property(gobject_class, PROP_SAMPLES_PER_BUFFER,
  	g_param_spec_int("samplesperbuffer", "Samples per buffer",
          "Number of samples in each outgoing buffer",
          1, G_MAXINT, 1024, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "Is Live",
          "Whether to act as a live source", FALSE, G_PARAM_READWRITE));

  paramspec=g_param_spec_string("note", "Musical note", "Musical note (e.g. 'c-3', 'd#4')",
          NULL, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_param_spec_set_qdata(paramspec,gstbt_property_meta_quark,GINT_TO_POINTER(TRUE));
  g_param_spec_set_qdata(paramspec,gstbt_property_meta_quark_flags,GINT_TO_POINTER(GSTBT_PROPERTY_META_NONE));
  g_param_spec_set_qdata(paramspec,gstbt_property_meta_quark_no_val,NULL);
  g_object_class_install_property(gobject_class,PROP_NOTE, paramspec);

  paramspec=g_param_spec_enum("wave", "Waveform", "Oscillator waveform",
          GST_TYPE_SIM_SYN_WAVE, /* enum type */
          GST_SIM_SYN_WAVE_SINE, /* default value */
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_WAVE, paramspec);

  paramspec=g_param_spec_double("volume", "Volume", "Volume of tone",
          0.0, 1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_VOLUME, paramspec);

  paramspec=g_param_spec_double("decay", "Decay", "Volume decay of the tone in seconds",
          0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_DECAY, paramspec);

  paramspec=g_param_spec_enum("filter", "Filtertype", "Type of audio filter",
          GST_TYPE_SIM_SYN_FILTER,    /* enum type */
          GST_SIM_SYN_FILTER_LOWPASS, /* default value */
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_FILTER, paramspec);

  paramspec=g_param_spec_double("cut-off", "Cut-Off", "Audio filter cut-off frequency",
          0.0, 1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_CUTOFF, paramspec);

  paramspec=g_param_spec_double("resonance", "Resonance", "Audio filter resonance",
          0.7, 25.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property(gobject_class, PROP_RESONANCE, paramspec);
}

static void
gst_sim_syn_init (GstSimSyn * src, GstSimSynClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gst_sim_syn_src_fixate);

  src->samples_per_buffer = 1024.0;
  src->generate_samples_per_buffer = (guint)src->samples_per_buffer;

  src->samplerate = GST_AUDIO_DEF_RATE;
  src->beats_per_minute=120;
  src->ticks_per_beat=4;
  src->subticks_per_tick=1;
  gst_sim_syn_calculate_buffer_frames (src);
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
  src->wave = GST_SIM_SYN_WAVE_SINE;
  gst_sim_syn_change_wave (src);

  /* add a volume envelope generator */
  src->volenv=gstbt_envelope_new ();
  src->volenv_controller=gst_controller_new (G_OBJECT(src->volenv), "value", NULL);
  gst_controller_set_interpolation_mode (src->volenv_controller, "value", GST_INTERPOLATE_LINEAR);

  /* set filter */
  src->filter = GST_SIM_SYN_FILTER_LOWPASS;
  src->cutoff = 0.8;
  src->resonance = 0.8;
  src->flt_res=1.0/src->resonance;
  gst_sim_syn_change_filter (src);
}

static void
gst_sim_syn_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstSimSyn *src = GST_SIM_SYN (GST_PAD_PARENT (pad));
  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);
}

static gboolean
gst_sim_syn_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstSimSyn *src = GST_SIM_SYN (basesrc);
  const GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "rate", &src->samplerate);

  return ret;
}

static gboolean
gst_sim_syn_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstSimSyn *src = GST_SIM_SYN (basesrc);
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

static gboolean
gst_sim_syn_send_event (GstElement *elem, GstEvent *event) {
  //GST_INFO_OBJECT(elem,"reveived event: %s",GST_EVENT_TYPE_NAME(event));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG: {
      GstTagList *list;
      gchar *str;

      gst_event_parse_tag (event, &list);
      str=gst_structure_to_string (list);
      GST_INFO ("got tags: %s",str);
      g_free (str);
      //gst_event_unref (event);
      return gst_pad_push_event (GST_BASE_SRC_PAD(elem),event);
    } break;
    default:
      break;
  }
  return GST_ELEMENT_CLASS (parent_class)->send_event (elem,event);
}

/* Wave generators */

static void
gst_sim_syn_create_sine (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
      src->accumulator += step;
      /* @todo: move out of inner loop? */
      if (G_UNLIKELY (src->accumulator >= M_PI_M2))
        src->accumulator -= M_PI_M2;

      samples[i] = (gint16) (sin (src->accumulator) * amp);
    }
  }
}

static void
gst_sim_syn_create_square (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
      src->accumulator += step;
      if (G_UNLIKELY (src->accumulator >= M_PI_M2))
        src->accumulator -= M_PI_M2;

      samples[i] = (gint16) ((src->accumulator < M_PI) ? amp : -amp);
    }
  }
}

static void
gst_sim_syn_create_saw (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0 / M_PI;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
      src->accumulator += step;
      if (G_UNLIKELY (src->accumulator >= M_PI_M2))
        src->accumulator -= M_PI_M2;

      if (src->accumulator < M_PI) {
        samples[i] = (gint16) (src->accumulator * amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - src->accumulator) * -amp);
      }
    }
  }
}

static void
gst_sim_syn_create_triangle (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = src->volume * 32767.0 / M_PI;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
      src->accumulator += step;
      if (G_UNLIKELY (src->accumulator >= M_PI_M2))
        src->accumulator -= M_PI_M2;

      if (src->accumulator < (M_PI * 0.5)) {
        samples[i] = (gint16) (src->accumulator * amp);
      } else if (src->accumulator < (M_PI * 1.5)) {
        samples[i] = (gint16) ((src->accumulator - M_PI) * -amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - src->accumulator) * -amp);
      }
    }
  }
}

static void
gst_sim_syn_create_silence (GstSimSyn * src, gint16 * samples)
{
  memset (samples, 0, src->generate_samples_per_buffer * sizeof (gint16));
}

static void
gst_sim_syn_create_white_noise (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble amp, ampf;

  ampf = src->volume * 65535.0;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
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
gst_sim_syn_init_pink_noise (GstSimSyn * src)
{
  gint i;
  gint num_rows = 12;           /* arbitrary: 1 .. PINK_MAX_RANDOM_ROWS */
  glong pmax;

  src->pink.index = 0;
  src->pink.index_mask = (1 << num_rows) - 1;
  /* calculate maximum possible signed random value.
   * Extra 1 for white noise always added. */
  pmax = (num_rows + 1) * (1 << (PINK_RANDOM_BITS - 1));
  src->pink.scalar = 1.0f / pmax;
  /* Initialize rows. */
  for (i = 0; i < num_rows; i++)
    src->pink.rows[i] = 0;
  src->pink.running_sum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
static gfloat
gst_sim_syn_generate_pink_noise_value (GstPinkNoise * pink)
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
gst_sim_syn_create_pink_noise (GstSimSyn * src, gint16 * samples)
{
  guint i=0, j;
  gdouble amp, ampf;

  ampf = src->volume * 32767.0;

  while (i < src->generate_samples_per_buffer) {
	/* the volume envelope */
    gst_controller_sync_values (src->volenv_controller, src->note_count);
	amp = src->volenv->value * ampf;
    src->note_count += INNER_LOOP;
    for (j = 0; ((j < INNER_LOOP) && (i<src->generate_samples_per_buffer)); j++, i++) {
      samples[i] =
        (gint16) (gst_sim_syn_generate_pink_noise_value (&src->pink) *
        amp);
    }
  }
}

static void
gst_sim_syn_init_sine_table (GstSimSyn * src)
{
  gint i;
  gdouble ang = 0.0;
  gdouble step = M_PI_M2 / 1024.0;
  gdouble amp = src->volume * 32767.0;

  for (i = 0; i < 1024; i++) {
    src->wave_table[i] = (gint16) (sin (ang) * amp);
    ang += step;
  }
}

static void
gst_sim_syn_create_sine_table (GstSimSyn * src, gint16 * samples)
{
  guint i;
  gdouble step, scl;

  step = M_PI_M2 * src->freq / src->samplerate;
  scl = 1024.0 /  M_PI_M2;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    /* @todo: add envelope */

    src->accumulator += step;
    if (G_UNLIKELY (src->accumulator >= M_PI_M2))
      src->accumulator -= M_PI_M2;

    samples[i] = src->wave_table[(gint) (src->accumulator * scl)];
  }
}

/* Filters */

static void
gst_sim_syn_filter_lowpass (GstSimSyn * src, gint16 * samples)
{
  guint i;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    src->flt_high = (gdouble)samples[i] - (src->flt_mid * src->flt_res) - src->flt_low;
    src->flt_mid += (src->flt_high * src->cutoff);
    src->flt_low += (src->flt_mid * src->cutoff);

    samples[i]=(gint16)CLAMP((glong)src->flt_low,G_MININT16,G_MAXINT16);
  }
}

static void
gst_sim_syn_filter_hipass (GstSimSyn * src, gint16 * samples)
{
  guint i;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    src->flt_high = (gdouble)samples[i] - (src->flt_mid * src->flt_res) - src->flt_low;
    src->flt_mid += (src->flt_high * src->cutoff);
    src->flt_low += (src->flt_mid * src->cutoff);

    samples[i]=(gint16)CLAMP((glong)src->flt_high,G_MININT16,G_MAXINT16);
  }
}

static void
gst_sim_syn_filter_bandpass (GstSimSyn * src, gint16 * samples)
{
  guint i;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    src->flt_high = (gdouble)samples[i] - (src->flt_mid * src->flt_res) - src->flt_low;
    src->flt_mid += (src->flt_high * src->cutoff);
    src->flt_low += (src->flt_mid * src->cutoff);

    samples[i]=(gint16)CLAMP((glong)src->flt_mid,G_MININT16,G_MAXINT16);
  }
}

static void
gst_sim_syn_filter_bandstop (GstSimSyn * src, gint16 * samples)
{
  guint i;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    src->flt_high = (gdouble)samples[i] - (src->flt_mid * src->flt_res) - src->flt_low;
    src->flt_mid += (src->flt_high * src->cutoff);
    src->flt_low += (src->flt_mid * src->cutoff);

    samples[i]=(gint16)CLAMP((glong)(src->flt_low+src->flt_high),G_MININT16,G_MAXINT16);
  }
}

/*
 * gst_sim_syn_change_wave:
 * Assign function pointer of wave genrator.
 */
static void
gst_sim_syn_change_wave (GstSimSyn * src)
{
  switch (src->wave) {
    case GST_SIM_SYN_WAVE_SINE:
      src->process = gst_sim_syn_create_sine;
      break;
    case GST_SIM_SYN_WAVE_SQUARE:
      src->process = gst_sim_syn_create_square;
      break;
    case GST_SIM_SYN_WAVE_SAW:
      src->process = gst_sim_syn_create_saw;
      break;
    case GST_SIM_SYN_WAVE_TRIANGLE:
      src->process = gst_sim_syn_create_triangle;
      break;
    case GST_SIM_SYN_WAVE_SILENCE:
      src->process = gst_sim_syn_create_silence;
      break;
    case GST_SIM_SYN_WAVE_WHITE_NOISE:
      src->process = gst_sim_syn_create_white_noise;
      break;
    case GST_SIM_SYN_WAVE_PINK_NOISE:
      gst_sim_syn_init_pink_noise (src);
      src->process = gst_sim_syn_create_pink_noise;
      break;
    case GST_SIM_SYN_WAVE_SINE_TAB:
      gst_sim_syn_init_sine_table (src);
      src->process = gst_sim_syn_create_sine_table;
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
gst_sim_syn_change_volume (GstSimSyn * src)
{
  switch (src->wave) {
    case GST_SIM_SYN_WAVE_SINE_TAB:
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
gst_sim_syn_change_filter (GstSimSyn * src)
{
  switch (src->filter) {
    case GST_SIM_SYN_FILTER_NONE:
      src->apply_filter = NULL;
      break;
    case GST_SIM_SYN_FILTER_LOWPASS:
      src->apply_filter = gst_sim_syn_filter_lowpass;
      break;
    case GST_SIM_SYN_FILTER_HIPASS:
      src->apply_filter = gst_sim_syn_filter_hipass;
      break;
    case GST_SIM_SYN_FILTER_BANDPASS:
      src->apply_filter = gst_sim_syn_filter_bandpass;
      break;
    case GST_SIM_SYN_FILTER_BANDSTOP:
      src->apply_filter = gst_sim_syn_filter_bandstop;
      break;
    default:
      GST_ERROR ("invalid filter-type: %d",src->filter);
      break;
  }
}

static void
gst_sim_syn_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live (basesrc)) {
    GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* get duration to calculate end time */
      GstClockTime duration = GST_BUFFER_DURATION (buffer);

      if (GST_CLOCK_TIME_IS_VALID (duration)) {
        *end = timestamp + duration;
      }
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

static gboolean
gst_sim_syn_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstSimSyn *src = GST_SIM_SYN (basesrc);
  GstClockTime time;

  segment->time = segment->start;
  time = segment->last_stop;

  /* now move to the time indicated */
  src->n_samples = gst_util_uint64_scale_int(time, src->samplerate, GST_SECOND);
  src->running_time = time;

  g_assert (src->running_time <= time);

  if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
    time = segment->stop;
    src->n_samples_stop = gst_util_uint64_scale_int(time, src->samplerate,
        GST_SECOND);
    src->check_seek_stop = TRUE;
  } else {
    src->check_seek_stop = FALSE;
  }
  src->seek_flags = segment->flags;
  src->eos_reached = FALSE;

  GST_DEBUG("seek from %"GST_TIME_FORMAT" to %"GST_TIME_FORMAT,
    GST_TIME_ARGS(segment->start),GST_TIME_ARGS(segment->stop));

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
  GstSimSyn *src = GST_SIM_SYN (basesrc);

  src->n_samples=G_GINT64_CONSTANT(0);
  src->running_time=G_GUINT64_CONSTANT(0);
  return(TRUE);
}

static GstFlowReturn
gst_sim_syn_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstSimSyn *src = GST_SIM_SYN (basesrc);
  GstFlowReturn res;
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples;
  gdouble samples_done;
  guint samples_per_buffer;

  if (G_UNLIKELY(src->eos_reached)) {
    GST_DEBUG("  EOS reached");
    return GST_FLOW_UNEXPECTED;
  }

  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done = (gdouble)src->running_time*(gdouble)src->samplerate/(gdouble)GST_SECOND;
  samples_per_buffer=(guint)(src->samples_per_buffer+(samples_done-(gdouble)src->n_samples));

  //GST_DEBUG("  samplers-per-buffer = %7d (%8.3lf), length = %u",samples_per_buffer,src->samples_per_buffer,length);

  /* check for eos */
  if (G_UNLIKELY(src->check_seek_stop &&
    (src->n_samples_stop > src->n_samples) &&
    (src->n_samples_stop < src->n_samples + samples_per_buffer))
  ) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer = (guint)(src->n_samples_stop - src->n_samples);
    n_samples = src->n_samples_stop;
    if (!(src->seek_flags&GST_SEEK_FLAG_SEGMENT)) {
      src->eos_reached = TRUE;
    }
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples = src->n_samples + samples_per_buffer;
  }

  next_time = gst_util_uint64_scale(n_samples,GST_SECOND,(guint64)src->samplerate);

  /* allocate a new buffer suitable for this pad */
  if ((res = gst_pad_alloc_buffer_and_set_caps (basesrc->srcpad, src->n_samples,
      src->generate_samples_per_buffer * sizeof (gint16),
      GST_PAD_CAPS (basesrc->srcpad),
      &buf)) != GST_FLOW_OK
  ) {
    return res;
  }

  GST_BUFFER_TIMESTAMP (buf) = src->running_time;
  GST_BUFFER_OFFSET_END (buf) = n_samples;
  GST_BUFFER_DURATION (buf) = next_time - src->running_time;

  gst_object_sync_values (G_OBJECT (src), src->running_time);

  GST_DEBUG("n_samples %12"G_GUINT64_FORMAT", d_samples %6u running_time %"GST_TIME_FORMAT", next_time %"GST_TIME_FORMAT", duration %"GST_TIME_FORMAT,
    src->n_samples,src->generate_samples_per_buffer,
    GST_TIME_ARGS(src->running_time),GST_TIME_ARGS(next_time),GST_TIME_ARGS(GST_BUFFER_DURATION (buf)));

  src->running_time += src->ticktime;
  //src->running_time = next_time;
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
gst_sim_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSimSyn *src = GST_SIM_SYN (object);

  if (src->dispose_has_run) return;

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      src->samples_per_buffer = (gdouble)g_value_get_int (value);
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_NOTE:
      g_free (src->note);
      src->note = g_value_dup_string (value);
      if(src->note) {
        guint64 attack,decay;
        GValue val = { 0, };

        GST_DEBUG("new note -> '%s'",src->note);
        src->freq = gstbt_tone_conversion_translate_from_string (src->n2f, src->note);
        /* trigger volume 'envelope' */
        src->volenv->value=0.001;
        src->note_count=0L;
        src->flt_low=src->flt_mid=src->flt_high=0.0;
        /* src->samplerate will be one second */
        attack=src->samplerate/100;
        decay=src->samplerate*src->decay;
        if(attack>decay) attack=decay-1;
        g_value_init (&val, G_TYPE_DOUBLE);
        gst_controller_unset_all(src->volenv_controller,"value");
                g_value_set_double(&val,0.001);
        gst_controller_set(src->volenv_controller,"value",0,&val);
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
  GstSimSyn *src = GST_SIM_SYN (object);

  if (src->dispose_has_run) return;

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      g_value_set_int (value, (gint)src->samples_per_buffer);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_NOTE:
      g_value_set_string (value, src->note);
      break;
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
	// help iface
	case PROP_DOCU_URI:
	  g_value_set_string(value, "file://"DATADIR""G_DIR_SEPARATOR_S"gtk-doc"G_DIR_SEPARATOR_S"html"G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S""PACKAGE"-GstSimSyn.html");
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sim_syn_dispose (GObject *object)
{
  GstSimSyn *src = GST_SIM_SYN (object);

  if (src->dispose_has_run) return;
  src->dispose_has_run = TRUE;

  if (src->n2f) g_object_unref (src->n2f);
  if (src->volenv_controller) g_object_unref (src->volenv_controller);
  if (src->volenv) g_object_unref (src->volenv);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

GType gst_sim_syn_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY(!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstSimSynClass),
      (GBaseInitFunc)gst_sim_syn_base_init,
      NULL,		          /* base_finalize */
      (GClassInitFunc)gst_sim_syn_class_init,
      NULL,		          /* class_finalize */
      NULL,               /* class_data */
      sizeof (GstSimSyn),
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
    const GInterfaceInfo help_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo preset_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

    type = g_type_register_static(GST_TYPE_BASE_SRC, "GstBtSimSyn", &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static(type, GSTBT_TYPE_PROPERTY_META, &property_meta_interface_info);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
    g_type_add_interface_static(type, GSTBT_TYPE_HELP, &help_interface_info);
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

  return gst_element_register (plugin, "simsyn",
      GST_RANK_NONE, GST_TYPE_SIM_SYN);
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
