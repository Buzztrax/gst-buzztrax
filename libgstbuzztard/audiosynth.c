/* GStreamer
 *
 * audiosynth.c: base audio synthesizer
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gstbtaudiosynth
 * @title: GstBtAudioSynth
 * @short_description: base audio synthesizer
 *
 * Base audio synthesizer to use as a foundation for new synthesizers. Handles
 * tempo, seeking, trick mode playback and format negotiation.
 * The pure virtual process and setup methods must be implemented by the child
 * class.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>
#include <gst/audio/audio.h>

#include "libgstbuzztard/tempo.h"

#include "audiosynth.h"

#define GST_CAT_DEFAULT audiosynth_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // tempo interface
  PROP_BPM = 1,
  PROP_TPB,
  PROP_STPT,
};

static GstStaticPadTemplate gstbt_audio_synth_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 16, " "depth = (int) 16, " "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, 2 ]")
    );

static GstBaseSrcClass *parent_class = NULL;

static void gstbt_audio_synth_base_init (gpointer klass);
static void gstbt_audio_synth_class_init (GstBtAudioSynthClass * klass);
static void gstbt_audio_synth_init (GstBtAudioSynth * object,
    GstBtAudioSynthClass * klass);

static void gstbt_audio_synth_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gstbt_audio_synth_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gstbt_audio_synth_dispose (GObject * object);

static gboolean gstbt_audio_synth_setcaps (GstBaseSrc * basesrc,
    GstCaps * caps);
static void gstbt_audio_synth_src_fixate (GstPad * pad, GstCaps * caps);

static gboolean gstbt_audio_synth_is_seekable (GstBaseSrc * basesrc);
static gboolean gstbt_audio_synth_do_seek (GstBaseSrc * basesrc,
    GstSegment * segment);
static gboolean gstbt_audio_synth_query (GstBaseSrc * basesrc,
    GstQuery * query);

static gboolean gstbt_audio_synth_start (GstBaseSrc * basesrc);
static GstFlowReturn gstbt_audio_synth_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);

static void
gstbt_audio_synth_calculate_buffer_frames (GstBtAudioSynth * self)
{
  const gdouble ticks_per_minute =
      (gdouble) (self->beats_per_minute * self->ticks_per_beat);
  const gdouble div = 60.0 / self->subticks_per_tick;
  const gdouble subticktime = ((GST_SECOND * div) / ticks_per_minute);
  GstClockTime ticktime =
      (GstClockTime) (0.5 + ((GST_SECOND * 60.0) / ticks_per_minute));

  self->samples_per_buffer = ((self->samplerate * div) / ticks_per_minute);
  self->ticktime = (GstClockTime) (0.5 + subticktime);
  GST_DEBUG ("samples_per_buffer=%lf", self->samples_per_buffer);
  // the sequence is quantized to ticks and not subticks
  // we need to compensate for the rounding errors :/
  self->ticktime_err =
      ((gdouble) ticktime -
      (gdouble) (self->subticks_per_tick * self->ticktime)) /
      (gdouble) self->subticks_per_tick;
  GST_DEBUG ("ticktime err=%lf", self->ticktime_err);
}

static void
gstbt_audio_synth_tempo_change_tempo (GstBtTempo * tempo,
    glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (tempo);
  gboolean changed = FALSE;

  if (beats_per_minute >= 0) {
    if (self->beats_per_minute != beats_per_minute) {
      self->beats_per_minute = (gulong) beats_per_minute;
      g_object_notify (G_OBJECT (self), "beats-per-minute");
      changed = TRUE;
    }
  }
  if (ticks_per_beat >= 0) {
    if (self->ticks_per_beat != ticks_per_beat) {
      self->ticks_per_beat = (gulong) ticks_per_beat;
      g_object_notify (G_OBJECT (self), "ticks-per-beat");
      changed = TRUE;
    }
  }
  if (subticks_per_tick >= 0) {
    if (self->subticks_per_tick != subticks_per_tick) {
      self->subticks_per_tick = (gulong) subticks_per_tick;
      g_object_notify (G_OBJECT (self), "subticks-per-tick");
      changed = TRUE;
    }
  }
  if (changed) {
    GST_DEBUG ("changing tempo to %lu BPM  %lu TPB  %lu STPT",
        self->beats_per_minute, self->ticks_per_beat, self->subticks_per_tick);
    gstbt_audio_synth_calculate_buffer_frames (self);
  }
}

static void
gstbt_audio_synth_tempo_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtTempoInterface *iface = g_iface;
  GST_INFO ("initializing interface");

  iface->change_tempo = gstbt_audio_synth_tempo_change_tempo;
}

//-- audiosynth implementation

static void
gstbt_audio_synth_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "audiosynth",
      GST_DEBUG_FG_BLUE | GST_DEBUG_BG_BLACK, "Base Audio synthesizer");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gstbt_audio_synth_src_template));
  gst_element_class_set_details_simple (element_class,
      "Audio Synth",
      "Source/Audio",
      "Base Audio synthesizer",
      "Tom Mast <amishmansteve@users.sourceforge.net>");
}

static void
gstbt_audio_synth_class_init (GstBtAudioSynthClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;

  parent_class = (GstBaseSrcClass *) g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gstbt_audio_synth_set_property;
  gobject_class->get_property = gstbt_audio_synth_get_property;
  gobject_class->dispose = gstbt_audio_synth_dispose;

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gstbt_audio_synth_setcaps);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gstbt_audio_synth_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gstbt_audio_synth_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gstbt_audio_synth_query);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gstbt_audio_synth_start);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gstbt_audio_synth_create);

  /* make process and setup method pure virtual */
  klass->process = NULL;
  klass->setup = NULL;

  /* override interface properties */
  g_object_class_override_property (gobject_class, PROP_BPM,
      "beats-per-minute");
  g_object_class_override_property (gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property (gobject_class, PROP_STPT,
      "subticks-per-tick");
}

static void
gstbt_audio_synth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
      /* tempo interface */
    case PROP_BPM:
    case PROP_TPB:
    case PROP_STPT:
      GST_WARNING ("use gstbt_tempo_change_tempo()");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_audio_synth_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
      /* tempo interface */
    case PROP_BPM:
      g_value_set_ulong (value, src->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong (value, src->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong (value, src->subticks_per_tick);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_audio_synth_init (GstBtAudioSynth * src, GstBtAudioSynthClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gstbt_audio_synth_src_fixate);

  src->samplerate = GST_AUDIO_DEF_RATE;
  src->beats_per_minute = 120;
  src->ticks_per_beat = 4;
  src->subticks_per_tick = 1;
  gstbt_audio_synth_calculate_buffer_frames (src);
  src->generate_samples_per_buffer = (guint) (0.5 + src->samples_per_buffer);

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

}

static void
gstbt_audio_synth_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (GST_PAD_PARENT (pad));
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (src);
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);

  if (klass->setup) {
    klass->setup (src, pad, caps);
  } else {
    GST_ERROR_OBJECT (pad, "class lacks setup() vmethod implementation");
  }
}

static gboolean
gstbt_audio_synth_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  const GstStructure *structure = gst_caps_get_structure (caps, 0);
  gboolean ret;

  ret = gst_structure_get_int (structure, "rate", &src->samplerate);
  ret &= gst_structure_get_int (structure, "channels", &src->channels);
  return ret;
}

static gboolean
gstbt_audio_synth_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
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
      res = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
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
gstbt_audio_synth_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstClockTime time;

  time = segment->last_stop;
  src->reverse = (segment->rate < 0.0);
  src->running_time = time;
  src->ticktime_err_accum = 0.0;

  /* now move to the time indicated */
  src->n_samples =
      gst_util_uint64_scale_int (time, src->samplerate, GST_SECOND);

  if (!src->reverse) {
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      segment->time = segment->start;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      time = segment->stop;
      src->n_samples_stop = gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = src->subticks_per_tick;
  } else {
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      segment->time = segment->stop;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      time = segment->start;
      src->n_samples_stop = gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = 1;
  }
  src->eos_reached = FALSE;

  GST_DEBUG_OBJECT (src,
      "seek from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT " cur %"
      GST_TIME_FORMAT " rate %lf", GST_TIME_ARGS (segment->start),
      GST_TIME_ARGS (segment->stop), GST_TIME_ARGS (segment->last_stop),
      segment->rate);

  return TRUE;
}

static gboolean
gstbt_audio_synth_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static gboolean
gstbt_audio_synth_start (GstBaseSrc * basesrc)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);

  src->n_samples = G_GINT64_CONSTANT (0);
  src->running_time = G_GUINT64_CONSTANT (0);
  src->ticktime_err_accum = 0.0;

  return TRUE;
}

static GstFlowReturn
gstbt_audio_synth_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (src);
  GstFlowReturn res;
  GstBuffer *buf;
  GstClockTime next_running_time;
  gint64 n_samples;
  gdouble samples_done;
  guint samples_per_buffer;
  gboolean partial_buffer = FALSE;

  if (G_UNLIKELY (src->eos_reached)) {
    GST_WARNING_OBJECT (src, "EOS reached");
    return GST_FLOW_UNEXPECTED;
  }
  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done =
      (gdouble) src->running_time * (gdouble) src->samplerate /
      (gdouble) GST_SECOND;
  if (!src->reverse) {
    samples_per_buffer =
        (guint) (src->samples_per_buffer + (samples_done -
            (gdouble) src->n_samples));
  } else {
    samples_per_buffer =
        (guint) (src->samples_per_buffer + ((gdouble) src->n_samples -
            samples_done));
  }

  /*
     GST_DEBUG_OBJECT(src,"samples_done=%lf, src->n_samples=%lf, samples_per_buffer=%u",
     samples_done,(gdouble)src->n_samples,samples_per_buffer);
     GST_DEBUG("  samplers-per-buffer = %7d (%8.3lf), length = %u",samples_per_buffer,src->samples_per_buffer,length);
   */

  /* check for eos */
  if (src->check_eos) {
    if (!src->reverse) {
      partial_buffer = ((src->n_samples_stop >= src->n_samples) &&
          (src->n_samples_stop < src->n_samples + samples_per_buffer));
    } else {
      partial_buffer = ((src->n_samples_stop < src->n_samples) &&
          (src->n_samples_stop >= src->n_samples - samples_per_buffer));
    }
  }

  if (G_UNLIKELY (partial_buffer)) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer =
        (guint) (src->n_samples_stop - src->n_samples);
    GST_INFO_OBJECT (src, "partial buffer: %u",
        src->generate_samples_per_buffer);
    if (G_UNLIKELY (!src->generate_samples_per_buffer)) {
      GST_WARNING_OBJECT (src, "0 samples left -> EOS reached");
      src->eos_reached = TRUE;
      return GST_FLOW_UNEXPECTED;
    }
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples =
        src->n_samples +
        (src->reverse ? (-samples_per_buffer) : samples_per_buffer);
  }
  next_running_time =
      src->running_time + (src->reverse ? (-src->ticktime) : src->ticktime);
  src->ticktime_err_accum =
      src->ticktime_err_accum +
      (src->reverse ? (-src->ticktime_err) : src->ticktime_err);

  /* allocate a new buffer suitable for this pad */
  res = gst_pad_alloc_buffer_and_set_caps (basesrc->srcpad, src->n_samples,
      src->channels * src->generate_samples_per_buffer * sizeof (gint16),
      GST_PAD_CAPS (basesrc->srcpad), &buf);
  if (res != GST_FLOW_OK) {
    return res;
  }

  if (!src->reverse) {
    GST_BUFFER_TIMESTAMP (buf) =
        src->running_time + (GstClockTime) src->ticktime_err_accum;
    GST_BUFFER_DURATION (buf) = next_running_time - src->running_time;
    GST_BUFFER_OFFSET (buf) = src->n_samples;
    GST_BUFFER_OFFSET_END (buf) = n_samples;
  } else {
    GST_BUFFER_TIMESTAMP (buf) =
        next_running_time + (GstClockTime) src->ticktime_err_accum;
    GST_BUFFER_DURATION (buf) = src->running_time - next_running_time;
    GST_BUFFER_OFFSET (buf) = n_samples;
    GST_BUFFER_OFFSET_END (buf) = src->n_samples;
  }

  if (src->subtick_count >= src->subticks_per_tick) {
    src->subtick_count = 1;
  } else {
    src->subtick_count++;
  }

  gst_object_sync_values (G_OBJECT (src), GST_BUFFER_TIMESTAMP (buf));

  GST_DEBUG ("n_samples %12" G_GUINT64_FORMAT ", d_samples %6u running_time %"
      GST_TIME_FORMAT ", next_time %" GST_TIME_FORMAT ", duration %"
      GST_TIME_FORMAT, src->n_samples, src->generate_samples_per_buffer,
      GST_TIME_ARGS (src->running_time), GST_TIME_ARGS (next_running_time),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  src->running_time = next_running_time;
  src->n_samples = n_samples;

  klass->process (src, buf);

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
gstbt_audio_synth_dispose (GObject * object)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GType
gstbt_audio_synth_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtAudioSynthClass),
      (GBaseInitFunc) gstbt_audio_synth_base_init,
      NULL,                     /* base_finalize */
      (GClassInitFunc) gstbt_audio_synth_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstBtAudioSynth),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gstbt_audio_synth_init
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) gstbt_audio_synth_tempo_interface_init,      /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };
    const GInterfaceInfo preset_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    type =
        g_type_register_static (GST_TYPE_BASE_SRC, "GstBtAudioSynth",
        &element_type_info, G_TYPE_FLAG_ABSTRACT);
    g_type_add_interface_static (type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
    g_type_add_interface_static (type, GST_TYPE_PRESET, &preset_interface_info);
  }
  return type;
}
