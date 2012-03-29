/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:gstaudiodelay
 * @title: GstBtAudioDelay
 * @short_description: audio echo effect
 *
 * <refsect2>
 * Echo effect with controllable effect-ratio, delay-time and feedback.
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audiodelay drywet=50 delaytime=25 feedback=75 ! osssink
 * gst-launch osssrc ! audiodelay delaytime=25 feedback=75 ! osssink
 * </programlisting>
 * In the latter example the echo is applied to the input signal of the
 * soundcard (like a microphone).
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/gstcontroller.h>
#include <gst/audio/audio.h>

#if !GST_CHECK_VERSION(0,10,31)
#include <libgstbuzztard/help.h>
#endif
#include <libgstbuzztard/tempo.h>

#include "audiodelay.h"

#define GST_CAT_DEFAULT gst_audio_delay_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  // static class properties
  // dynamic class properties
  PROP_DRYWET=1,
  PROP_DELAYTIME,
  PROP_FEEDBACK,
  // tempo iface
  PROP_BPM,
  PROP_TPB,
  PROP_STPT,
#if !GST_CHECK_VERSION(0,10,31)
  // help iface
  PROP_DOCU_URI
#endif
};

#define DELAYTIME_MAX 1000

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1, "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "signed = (boolean) true")
);

static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1, "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "signed = (boolean) true")
);

static GstBaseTransformClass *parent_class = NULL;

static void gst_audio_delay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_delay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audio_delay_finalize (GObject * object);

static gboolean gst_audio_delay_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps);

static gboolean gst_audio_delay_start (GstBaseTransform * base);
static GstFlowReturn gst_audio_delay_transform_ip (GstBaseTransform * base,
    GstBuffer * outbuf);
static gboolean gst_audio_delay_stop (GstBaseTransform * base);

/* tempo interface implementations */

static void gst_audio_delay_calculate_tick_time(GstBtAudioDelay *self) {
  self->ticktime=((GST_SECOND*60)/(GstClockTime)(self->beats_per_minute*self->ticks_per_beat));
}

static void gst_audio_delay_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstBtAudioDelay *self=GSTBT_AUDIO_DELAY(tempo);
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
    GST_DEBUG("changing tempo to %ld BPM  %ld TPB  %ld STPT",self->beats_per_minute,self->ticks_per_beat,self->subticks_per_tick);
    gst_audio_delay_calculate_tick_time(self);
  }
}

static void gst_audio_delay_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_audio_delay_tempo_change_tempo;
}

/* help interface implementations */

/* GObject vmethod implementations */

static void
gst_audio_delay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_set_details_simple (element_class,
    "AudioDelay",
    "Filter/Effect/Audio",
    "Add echos to audio streams",
    "Stefan Kost <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
   gst_element_class_set_documentation_uri (element_class,
       "file://"DATADIR""G_DIR_SEPARATOR_S"gtk-doc"G_DIR_SEPARATOR_S"html"G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S""PACKAGE"-GstBtAudioDelay.html");
#endif
}

static void
gst_audio_delay_class_init (GstBtAudioDelayClass * klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_audio_delay_set_property;
  gobject_class->get_property = gst_audio_delay_get_property;
  gobject_class->finalize = gst_audio_delay_finalize;

  // override interface properties
  g_object_class_override_property(gobject_class, PROP_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, PROP_STPT, "subticks-per-tick");

#if !GST_CHECK_VERSION(0,10,31)
  g_object_class_override_property(gobject_class, PROP_DOCU_URI, "documentation-uri");
#endif

  // register own properties

  g_object_class_install_property (gobject_class, PROP_DRYWET,
    g_param_spec_uint ("drywet", "Dry-Wet", "Intensity of effect (0 none -> 100 full)",
          0, 100, 50,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_DELAYTIME,
    g_param_spec_uint ("delaytime", "Delay time", "Time difference between two echos as milliseconds",
          1, DELAYTIME_MAX, 100,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_FEEDBACK,
    g_param_spec_uint ("feedback", "Fedback", "Echo feedback in percent",
          0, 99, 50,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  GST_BASE_TRANSFORM_CLASS (klass)->set_caps =
      GST_DEBUG_FUNCPTR (gst_audio_delay_set_caps);
  GST_BASE_TRANSFORM_CLASS (klass)->start =
      GST_DEBUG_FUNCPTR (gst_audio_delay_start);
  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_delay_transform_ip);
  GST_BASE_TRANSFORM_CLASS (klass)->stop =
      GST_DEBUG_FUNCPTR (gst_audio_delay_stop);
}

static void
gst_audio_delay_init (GstBtAudioDelay *filter, GstBtAudioDelayClass * klass)
{
  filter->drywet = 50;
  filter->delaytime = 100;
  filter->feedback = 50;

  filter->samplerate = GST_AUDIO_DEF_RATE;
  filter->beats_per_minute=120;
  filter->ticks_per_beat=4;
  filter->subticks_per_tick=1;
  gst_audio_delay_calculate_tick_time (filter);

  filter->ring_buffer = NULL;

  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (filter), TRUE);
}

static void
gst_audio_delay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (object);

  switch (prop_id) {
    case PROP_DRYWET:
      filter->drywet = g_value_get_uint (value);
      break;
    case PROP_DELAYTIME:
      filter->delaytime = g_value_get_uint (value);
      break;
    case PROP_FEEDBACK:
      filter->feedback = g_value_get_uint (value);
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
gst_audio_delay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (object);

  switch (prop_id) {
    case PROP_DRYWET:
      g_value_set_uint (value, filter->drywet);
      break;
    case PROP_DELAYTIME:
      g_value_set_uint (value, filter->delaytime);
      break;
    case PROP_FEEDBACK:
      g_value_set_uint (value, filter->feedback);
      break;
	// tempo iface
    case PROP_BPM:
      g_value_set_ulong(value, filter->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong(value, filter->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong(value, filter->subticks_per_tick);
      break;
#if !GST_CHECK_VERSION(0,10,31)
    // help iface
    case PROP_DOCU_URI:
      g_value_set_static_string(value, "file://"DATADIR""G_DIR_SEPARATOR_S"gtk-doc"G_DIR_SEPARATOR_S"html"G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S""PACKAGE"-GstBtAudioDelay.html");
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_delay_finalize (GObject * object)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (object);

  if (filter->ring_buffer) {
    g_free (filter->ring_buffer);
    filter->ring_buffer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* GstElement vmethod implementations */

/* GstBaseTransform vmethod implementations */

static gboolean
gst_audio_delay_set_caps (GstBaseTransform * base, GstCaps * incaps, GstCaps * outcaps)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (base);
  const GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (incaps, 0);
  ret = gst_structure_get_int (structure, "rate", &filter->samplerate);

  return ret;
}

/* initialize processing
 */
static gboolean
gst_audio_delay_start (GstBaseTransform *base)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (base);

  filter->max_delaytime=(2 + (DELAYTIME_MAX * filter->samplerate) / 100);
  filter->ring_buffer = (gint16 *) g_new0 (gint16, filter->max_delaytime);
  filter->rb_ptr = 0;

  GST_INFO ("max_delaytime %d at %d Hz sampling rate", filter->max_delaytime,
    filter->samplerate);
  GST_INFO ("delaytime %d, feedback %d, drywet %d", filter->delaytime,
    filter->feedback,filter->drywet);

  return TRUE;
}

/* this function does the actual processing
 */
static GstFlowReturn
gst_audio_delay_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (base);
  GstClockTime timestamp;
  guint delaytime;
  gdouble feedback, dry, wet;
  gint16 *data = (gint16 *) GST_BUFFER_DATA (outbuf);
  gdouble val_dry, val_fx;
  glong val;
  guint i, num_samples = GST_BUFFER_SIZE (outbuf) / sizeof (gint16);
  guint rb_in, rb_out;
  gint32 sum_fx=0;

  /* flush ring_buffer on DISCONT */
  if (GST_BUFFER_FLAG_IS_SET (outbuf,GST_BUFFER_FLAG_DISCONT)) {
    memset (filter->ring_buffer, 0, sizeof (gint16) * filter->max_delaytime);
    filter->rb_ptr = 0;
  }

  timestamp = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME,
    GST_BUFFER_TIMESTAMP (outbuf));
  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (G_OBJECT (filter), timestamp);

  delaytime = (filter->delaytime * filter->samplerate) / 100;
  feedback = (gdouble)filter->feedback / 100.0;
  wet = (gdouble)filter->drywet / 100.0;
  dry = 1.0 - wet;
  rb_in = filter->rb_ptr;
  rb_out = (rb_in >= delaytime) ?
        rb_in - delaytime :
        (rb_in + filter->max_delaytime) - delaytime;

  if (G_UNLIKELY(GST_BUFFER_FLAG_IS_SET (outbuf,GST_BUFFER_FLAG_GAP) ||
    gst_base_transform_is_passthrough (base))) {
    /* input is silence */

    for (i = 0; i < num_samples; i++) {
      val_fx = (gdouble)filter->ring_buffer[rb_out];
      sum_fx += abs (filter->ring_buffer[rb_out]);
      val = (glong)(val_fx * feedback);
      filter->ring_buffer[rb_in] = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
      val = (glong)(wet * val_fx);
      *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
      rb_in++; if(rb_in == filter->max_delaytime) rb_in = 0;
      rb_out++; if(rb_out == filter->max_delaytime) rb_out = 0;
    }
  }
  else {
    for (i = 0; i < num_samples; i++) {
      val_fx = (gdouble)filter->ring_buffer[rb_out];
      sum_fx += abs (filter->ring_buffer[rb_out]);
      val_dry = (gdouble)*data;
      val = (glong)(val_fx * feedback + val_dry);
      filter->ring_buffer[rb_in] = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
      val = (glong)(wet * val_fx + dry * val_dry);
      *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
      rb_in++; if(rb_in == filter->max_delaytime) rb_in = 0;
      rb_out++; if(rb_out == filter->max_delaytime) rb_out = 0;
    }
  }
  filter->rb_ptr=rb_in;

  if (GST_BUFFER_FLAG_IS_SET (outbuf,GST_BUFFER_FLAG_GAP) && sum_fx) {
    GST_BUFFER_FLAG_UNSET (outbuf,GST_BUFFER_FLAG_GAP);
  }

  return GST_FLOW_OK;
}

/* clean up after processing
 */
static gboolean
gst_audio_delay_stop (GstBaseTransform *base)
{
  GstBtAudioDelay *filter = GSTBT_AUDIO_DELAY (base);

  if (filter->ring_buffer) {
    g_free (filter->ring_buffer);
    filter->ring_buffer = NULL;
  }

  return TRUE;
}


GType gstbt_audio_delay_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY(!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtAudioDelayClass),
      (GBaseInitFunc)gst_audio_delay_base_init,
      NULL,		          /* base_finalize */
      (GClassInitFunc)gst_audio_delay_class_init,
      NULL,		          /* class_finalize */
      NULL,               /* class_data */
      sizeof (GstBtAudioDelay),
      0,                  /* n_preallocs */
      (GInstanceInitFunc) gst_audio_delay_init
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) gst_audio_delay_tempo_interface_init,          /* interface_init */
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
    type = g_type_register_static(GST_TYPE_BASE_TRANSFORM, "GstBtAudioDelay", &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
#if !GST_CHECK_VERSION(0,10,31)
    g_type_add_interface_static(type, GSTBT_TYPE_HELP, &help_interface_info);
#endif
  }
  return type;
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_audio_delay_debug, "audiodelay", 0, "audiodelay plugin");

  /* initialize gst controller library */
  gst_controller_init(NULL, NULL);

  return gst_element_register (plugin, "audiodelay", GST_RANK_NONE,
      GSTBT_TYPE_AUDIO_DELAY);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "audiodelay",
    "Audio echo plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN);
