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
 * SECTION:element-audiotestsrc
 * @short_description: simple audio synthesizer for gstreamer
 *
 */
/* @todo:
 * - implement property-meta iface (see gstbml)
 * - implement tempo iface (needed to calculate decay steps)
 * - make decay sample-rate independent (specify as ticks)
 *
 * - add polyphonic element
 *   - simsyn-mono, simsyn-poly
 * - add svf filter
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "simsyn.h"

#define M_PI_M2 ( M_PI + M_PI )

GstElementDetails gst_sim_syn_details = {
  "Simple Synth",
  "Source/Audio",
  "Simple audio synthesizer",
  "Stefan Kost <ensonic@users.sf.net>"
};


enum
{
  PROP_0,
  PROP_SAMPLES_PER_BUFFER,
  PROP_WAVE,
  PROP_NOTE,
  PROP_VOLUME,
  PROP_DECAY,
  PROP_IS_LIVE,
  PROP_TIMESTAMP_OFFSET,
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


GST_BOILERPLATE (GstSimSyn, gst_sim_syn, GstBaseSrc,
    GST_TYPE_BASE_SRC);

#define GST_TYPE_SIM_SYN_WAVE (gst_audiostestsrc_wave_get_type())
static GType
gst_audiostestsrc_wave_get_type (void)
{
  static GType audiostestsrc_wave_type = 0;
  static GEnumValue audiostestsrc_waves[] = {
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

  if (!audiostestsrc_wave_type) {
    audiostestsrc_wave_type = g_enum_register_static ("GstSimSynWave",
        audiostestsrc_waves);
  }
  return audiostestsrc_wave_type;
}

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

static void gst_sim_syn_change_wave (GstSimSyn * src);

static void gst_sim_syn_get_times (GstBaseSrc * basesrc,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_sim_syn_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);


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
  GstBaseSrcClass *gstbasesrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_sim_syn_set_property;
  gobject_class->get_property = gst_sim_syn_get_property;
  gobject_class->dispose      = gst_sim_syn_dispose;

  g_object_class_install_property (gobject_class, PROP_SAMPLES_PER_BUFFER,
      g_param_spec_int ("samplesperbuffer", "Samples per buffer",
          "Number of samples in each outgoing buffer",
          1, G_MAXINT, 1024, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GST_TYPE_SIM_SYN_WAVE, /* enum type */
          GST_SIM_SYN_WAVE_SINE, /* default value */
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_string ("note", "Musical note", "Musical note ('c-3', 'd#4')",
          NULL, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of tone",
          0.0, 1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_double ("decay", "Decay", "Volume decay of the tone",
          0.9, 0.999999, 0.9999, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "Is Live",
          "Whether to act as a live source", FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMESTAMP_OFFSET,
      g_param_spec_int64 ("timestamp-offset", "Timestamp offset",
          "An offset added to timestamps set on buffers (in ns)", G_MININT64,
          G_MAXINT64, 0, G_PARAM_READWRITE));

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_sim_syn_setcaps);
  gstbasesrc_class->is_seekable = 
      GST_DEBUG_FUNCPTR (gst_sim_syn_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_sim_syn_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_sim_syn_query);
  gstbasesrc_class->get_times =
      GST_DEBUG_FUNCPTR (gst_sim_syn_get_times);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_sim_syn_create);
}

static void
gst_sim_syn_init (GstSimSyn * src, GstSimSynClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gst_sim_syn_src_fixate);

  src->samplerate = 44100;
  src->volume = 1.0;
  src->freq = 0.0;
  src->note = NULL;
  src->decay = 0.9999;
  src->n2f = gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

  src->samples_per_buffer = 1024;
  src->generate_samples_per_buffer = src->samples_per_buffer;
  src->timestamp_offset = G_GINT64_CONSTANT (0);

  src->wave = GST_SIM_SYN_WAVE_SINE;
  gst_sim_syn_change_wave (src);
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
              dest_val = src_val / src->samplerate;
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val = src_val * src->samplerate;
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

static void
gst_sim_syn_create_sine (GstSimSyn * src, gint16 * samples)
{
  gint i;
  gdouble step, amp;

  step = M_PI_M2 * src->freq / src->samplerate;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * 32767.0;
    src->current_volume *= src->decay;

    src->accumulator += step;
    if (src->accumulator >= M_PI_M2)
      src->accumulator -= M_PI_M2;

    samples[i] = (gint16) (sin (src->accumulator) * amp);
  }
}

static void
gst_sim_syn_create_square (GstSimSyn * src, gint16 * samples)
{
  gint i;
  gdouble step, amp;

  step = M_PI_M2 * src->freq / src->samplerate;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * 32767.0;
    src->current_volume *= src->decay;

    src->accumulator += step;
    if (src->accumulator >= M_PI_M2)
      src->accumulator -= M_PI_M2;

    samples[i] = (gint16) ((src->accumulator < M_PI) ? amp : -amp);
  }
}

static void
gst_sim_syn_create_saw (GstSimSyn * src, gint16 * samples)
{
  gint i;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = 32767.0 / M_PI;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * ampf;
    src->current_volume *= src->decay;

    src->accumulator += step;
    if (src->accumulator >= M_PI_M2)
      src->accumulator -= M_PI_M2;

    if (src->accumulator < M_PI) {
      samples[i] = (gint16) (src->accumulator * amp);
    } else {
      samples[i] = (gint16) ((M_PI_M2 - src->accumulator) * -amp);
    }
  }
}

static void
gst_sim_syn_create_triangle (GstSimSyn * src, gint16 * samples)
{
  gint i;
  gdouble step, amp, ampf;

  step = M_PI_M2 * src->freq / src->samplerate;
  ampf = 32767.0 / M_PI_2;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * ampf;
    src->current_volume *= src->decay;

    src->accumulator += step;
    if (src->accumulator >= M_PI_M2)
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

static void
gst_sim_syn_create_silence (GstSimSyn * src, gint16 * samples)
{
  memset (samples, 0, src->generate_samples_per_buffer * sizeof (gint16));
}

static void
gst_sim_syn_create_white_noise (GstSimSyn * src, gint16 * samples)
{
  gint i;
  gdouble amp;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * 65535.0;
    src->current_volume *= src->decay;
    
    samples[i] = (gint16) (32768 - (amp * rand () / (RAND_MAX + 1.0)));
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
  gint i;
  gdouble amp;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    amp = src->current_volume * 32767.0;
    src->current_volume *= src->decay;

    samples[i] =
        (gint16) (gst_sim_syn_generate_pink_noise_value (&src->pink) *
        amp);
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
  gint i;
  gdouble step, scl;

  step = M_PI_M2 * src->freq / src->samplerate;
  scl = 1024.0 /  M_PI_M2;

  for (i = 0; i < src->generate_samples_per_buffer; i++) {
    /* @todo: add envelope */
    src->accumulator += step;
    if (src->accumulator >= M_PI_M2)
      src->accumulator -= M_PI_M2;

    samples[i] = src->wave_table[(gint) (src->accumulator * scl)];
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
      GST_ERROR ("invalid wave-form");
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

  time = segment->time = segment->start;

  /* now move to the time indicated */
  src->n_samples = time * src->samplerate / GST_SECOND;
  src->running_time = src->n_samples * GST_SECOND / src->samplerate;

  g_assert (src->running_time <= time);
  
  if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
    time = segment->stop;
    src->n_samples_stop = time * src->samplerate / GST_SECOND;
    src->check_seek_stop = TRUE;
  } else {
    src->check_seek_stop = FALSE;
  }
  src->eos_reached = FALSE;

  return TRUE;
}

static gboolean
gst_sim_syn_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static GstFlowReturn
gst_sim_syn_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstSimSyn *src;
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples;

  src = GST_SIM_SYN (basesrc);
  
  if (src->eos_reached) return GST_FLOW_UNEXPECTED;

  if (!src->tags_pushed) {
    GstTagList *taglist;
    GstEvent *event;

    taglist = gst_tag_list_new ();

    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND,
        GST_TAG_DESCRIPTION, "audiotest wave", NULL);

    event = gst_event_new_tag (taglist);
    gst_pad_push_event (basesrc->srcpad, event);
    src->tags_pushed = TRUE;
  }

  if (src->check_seek_stop &&
    (src->n_samples_stop > src->n_samples) &&
    (src->n_samples_stop < src->n_samples + src->samples_per_buffer)
  ) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer = src->n_samples_stop - src->n_samples;
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = src->samples_per_buffer;
    n_samples = src->n_samples + src->samples_per_buffer;
  }
  next_time = n_samples * GST_SECOND / src->samplerate;
  
  buf = gst_buffer_new_and_alloc (src->generate_samples_per_buffer * sizeof (gint16));
  gst_buffer_set_caps (buf, GST_PAD_CAPS (basesrc->srcpad));
  
  GST_BUFFER_TIMESTAMP (buf) = src->timestamp_offset + src->running_time;
  GST_BUFFER_OFFSET (buf) = src->n_samples;
  GST_BUFFER_OFFSET_END (buf) = n_samples;
  GST_BUFFER_DURATION (buf) = next_time - src->running_time;

  gst_object_sync_values (G_OBJECT (src), src->running_time);

  src->running_time = next_time;
  src->n_samples = n_samples;

  //GST_INFO ("current_volume=%8.6lf\n",src->current_volume);
  
  if ((src->freq != 0.0) && (src->current_volume > 0.0001)) {
    src->process (src, (gint16 *) GST_BUFFER_DATA (buf));
  } else {
    gst_sim_syn_create_silence (src, (gint16 *) GST_BUFFER_DATA (buf));
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
      src->samples_per_buffer = g_value_get_int (value);
      break;
    case PROP_WAVE:
      src->wave = g_value_get_enum (value);
      gst_sim_syn_change_wave (src);
      break;
    case PROP_NOTE:
      g_free (src->note);
      src->note = g_value_dup_string (value);
      src->freq = gst_note_2_frequency_translate_from_string (src->n2f, src->note);
      /* trigger volume 'envelope' */
      src->current_volume = src->volume;
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_double (value);
      gst_sim_syn_change_volume (src);
      break;
    case PROP_DECAY:
      src->decay = g_value_get_double (value);
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_TIMESTAMP_OFFSET:
      src->timestamp_offset = g_value_get_int64 (value);
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
      g_value_set_int (value, src->samples_per_buffer);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, src->wave);
      break;
    case PROP_NOTE:
      g_value_set_string (value, src->note);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    case PROP_DECAY:
      g_value_set_double (value, src->decay);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int64 (value, src->timestamp_offset);
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
  
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "simsyn",
      GST_RANK_NONE, GST_TYPE_SIM_SYN);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "simsyn",
    "Simple audio synthesizer",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
