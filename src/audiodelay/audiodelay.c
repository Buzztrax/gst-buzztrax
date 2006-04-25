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
 * @short_description: audioecho effect
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

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/gstcontroller.h>

#include "audiodelay.h"

GST_DEBUG_CATEGORY_STATIC (gst_audiodelay_debug);
#define GST_CAT_DEFAULT gst_audiodelay_debug

static const GstElementDetails element_details =
GST_ELEMENT_DETAILS ("Audiodelay",
  "Filter/Effect/Audio",
  "Add echos to audio streams",
  "Stefan Kost <ensonic@users.sf.net>");

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_DRYWET,
  PROP_DELAYTIME,
  PROP_FEEDBACK
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

#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_audiodelay_debug, "audiodelay", 0, "audiodelay plugin");

GST_BOILERPLATE_FULL (GstAudiodelay, gst_audiodelay, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM, DEBUG_INIT);

static void gst_audiodelay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audiodelay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audiodelay_dispose (GObject * object);

static gboolean gst_audiodelay_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_audiodelay_start (GstBaseTransform * base);
static GstFlowReturn gst_audiodelay_transform_ip (GstBaseTransform * base,
    GstBuffer * outbuf);
static gboolean gst_audiodelay_stop (GstBaseTransform * base);

/* GObject vmethod implementations */

static void
gst_audiodelay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_set_details (element_class, &element_details);
}

static void
gst_audiodelay_class_init (GstAudiodelayClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  gobject_class->set_property = gst_audiodelay_set_property;
  gobject_class->get_property = gst_audiodelay_get_property;
  gobject_class->dispose = gst_audiodelay_dispose;

  g_object_class_install_property (gobject_class, PROP_DRYWET,
    g_param_spec_uint ("drywet", "Dry-Wet", "Intensity of effect (0 none -> 100 full)",
          0, 100, 50,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_DELAYTIME,
    g_param_spec_uint ("delaytime", "Delay time", "Time difference between two echos as milli seconds.",
          1, DELAYTIME_MAX, 100,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_FEEDBACK,
    g_param_spec_uint ("feedback", "Fedback", "Echo feedback in percent.",
          0, 99, 50,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  GST_BASE_TRANSFORM_CLASS (klass)->set_caps =
      GST_DEBUG_FUNCPTR (gst_audiodelay_set_caps);
  GST_BASE_TRANSFORM_CLASS (klass)->start =
      GST_DEBUG_FUNCPTR (gst_audiodelay_start);
  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audiodelay_transform_ip);
  GST_BASE_TRANSFORM_CLASS (klass)->stop =
      GST_DEBUG_FUNCPTR (gst_audiodelay_stop);
}

static void
gst_audiodelay_init (GstAudiodelay *filter, GstAudiodelayClass * klass)
{
  filter->drywet = 50;
  filter->delaytime = 100;
  filter->feedback = 50;
  
  filter->samplerate = 44100;
  filter->ring_buffer = NULL;
}

static void
gst_audiodelay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudiodelay *filter = GST_AUDIODELAY (object);

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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audiodelay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudiodelay *filter = GST_AUDIODELAY (object);

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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audiodelay_dispose (GObject * object)
{
  GstAudiodelay *filter = GST_AUDIODELAY (object);

  if (filter->ring_buffer) {
    g_free (filter->ring_buffer);
    filter->ring_buffer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/* GstBaseTransform vmethod implementations */

static gboolean
gst_audiodelay_set_caps (GstBaseTransform * base, GstCaps * incaps, GstCaps * outcaps)
{
  GstAudiodelay *filter = GST_AUDIODELAY (base);
  const GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (incaps, 0);
  ret = gst_structure_get_int (structure, "rate", &filter->samplerate);

  return ret;
}

/* initialize processing
 */
static gboolean
gst_audiodelay_start (GstBaseTransform *base)
{
  GstAudiodelay *filter = GST_AUDIODELAY (base);

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
gst_audiodelay_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstAudiodelay *filter = GST_AUDIODELAY (base);
  guint delaytime;
  gdouble feedback, dry, wet;
  gint16 *data = (gint16 *) GST_BUFFER_DATA (outbuf);
  gdouble val_dry, val_fx;
  glong val; 
  guint i, num_samples = GST_BUFFER_SIZE (outbuf) / sizeof (gint16);
  guint rb_in, rb_out;
  
  if (!gst_buffer_is_writable (outbuf)) return GST_FLOW_OK;

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP (outbuf)))
    gst_object_sync_values (G_OBJECT (filter), GST_BUFFER_TIMESTAMP (outbuf));
  
  delaytime = (filter->delaytime * filter->samplerate) / 100;
  feedback = (gdouble)filter->feedback / 100.0;
  wet = (gdouble)filter->drywet / 100.0;
  dry = 1.0 - wet;
  rb_in = filter->rb_ptr;
  rb_out = (rb_in >= delaytime) ?
        rb_in - delaytime :
        (rb_in + filter->max_delaytime) - delaytime; 
        
  for (i = 0; i < num_samples; i++) {
    val_fx = (gdouble)filter->ring_buffer[rb_out];
    val_dry = (gdouble)*data;
    val = (glong)(val_fx * feedback + val_dry);
    filter->ring_buffer[rb_in] = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
    val = (glong)(wet * val_fx + dry * val_dry);
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
    rb_in++; if(rb_in == filter->max_delaytime) rb_in = 0;
    rb_out++; if(rb_out == filter->max_delaytime) rb_out = 0;
  }
  filter->rb_ptr=rb_in;

  return GST_FLOW_OK;
}

/* clean up after processing
 */
static gboolean
gst_audiodelay_stop (GstBaseTransform *base)
{
  GstAudiodelay *filter = GST_AUDIODELAY (base);

  if (filter->ring_buffer) {
    g_free (filter->ring_buffer);
    filter->ring_buffer = NULL;
  }

  return TRUE;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 *
 * exchange the string 'plugin' with your elemnt name
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  /* initialize gst controller library */
  gst_controller_init(NULL, NULL);

  return gst_element_register (plugin, "audiodelay", GST_RANK_NONE,
      GST_TYPE_AUDIODELAY);
}

/* this is the structure that gstreamer looks for to register plugins
 *
 * exchange the strings 'plugin' and 'Template plugin' with you plugin name and
 * description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "audiodelay",
    "Audio echo plugin",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
