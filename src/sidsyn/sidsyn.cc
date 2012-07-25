/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsyn.cc: c64 sid synthesizer
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
/**
 * SECTION:gstsidsyn
 * @title: GstBtSidSyn
 * @short_description: c64 sid synthesizer
 *
 * A synthesizer based on the RSID emulation library of the C64 sound chip.
 * The element provides a sound generator with 3 voices.
 *
 * For technical details see:
 * http://en.wikipedia.org/wiki/MOS_Technology_SID#Technical_details.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch sidsyn num-buffers=1000 voice0::note="c-4" ! autoaudiosink
 * ]| Render a beep.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include <libgstbuzztard/childbin.h>
#include "libgstbuzztard/propertymeta.h"

#include "sidsyn.h"

//#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
//#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

#define GST_CAT_DEFAULT sid_syn_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

enum
{
  PROP_CHILDREN = 1,
  PROP_CUTOFF,
  PROP_RESONANCE,
  PROP_VOLUME,
  PROP_FILTER_LOW_PASS,
  PROP_FILTER_BAND_PASS,
  PROP_FILTER_HI_PASS,
  PROP_VOICE_3_OFF
};

static GstBtAudioSynthClass *parent_class = NULL;

static void gst_sid_syn_base_init (gpointer klass);
static void gst_sid_syn_class_init (GstBtSidSynClass * klass);
static void gst_sid_syn_init (GstBtSidSyn * object, GstBtSidSynClass * klass);

static void gst_sid_syn_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_sid_syn_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_sid_syn_dispose (GObject * object);
static void gst_sid_syn_process (GstBtAudioSynth * base, GstBuffer * data);
static gboolean gst_sid_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps);

//-- child proxy interface implementations

static GstObject *gst_sid_syn_child_proxy_get_child_by_index(GstChildProxy *child_proxy, guint index) {
  GstBtSidSyn *src = GSTBT_SID_SYN(child_proxy);

  g_return_val_if_fail (index < NUM_VOICES,NULL);

  return (GstObject *)gst_object_ref(src->voices[index]);
}

static guint gst_sid_syn_child_proxy_get_children_count(GstChildProxy *child_proxy) {
  return NUM_VOICES;
}


static void gst_sid_syn_child_proxy_interface_init(gpointer g_iface, gpointer iface_data) {
  GstChildProxyInterface *iface = (GstChildProxyInterface *)g_iface;

  GST_INFO("initializing iface");

  iface->get_child_by_index = gst_sid_syn_child_proxy_get_child_by_index;
  iface->get_children_count = gst_sid_syn_child_proxy_get_children_count;
}

//-- sidsyn implementation

static void
gst_sid_syn_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class,
      "C64 SID Synth",
      "Source/Audio",
      "c64 sid synthesizer", "Stefan Sauer <ensonic@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
  gst_element_class_set_documentation_uri (element_class,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "" PACKAGE
      "-GstBtSidSyn.html");
#endif
}

static void
gst_sid_syn_class_init (GstBtSidSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  const GParamFlags pflags = (GParamFlags) 
      (G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  parent_class = (GstBtAudioSynthClass *) g_type_class_peek_parent (klass);

  audio_synth_class->process = gst_sid_syn_process;
  audio_synth_class->setup = gst_sid_syn_setup;

  gobject_class->set_property = gst_sid_syn_set_property;
  gobject_class->get_property = gst_sid_syn_get_property;
  gobject_class->dispose = gst_sid_syn_dispose;
  
  g_object_class_override_property (gobject_class, PROP_CHILDREN, "children");

  // register own properties
  g_object_class_install_property (gobject_class, PROP_CUTOFF,
      g_param_spec_uint ("cut-off", "Cut-Off",
          "Audio filter cut-off frequency", 0, 2047, 1024, pflags));

  g_object_class_install_property (gobject_class, PROP_RESONANCE,
      g_param_spec_uint ("resonance", "Resonance", "Audio filter resonance",
          0, 15, 2, pflags));

  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_uint ("volume", "Volume", "Volume of tone",
          0, 15, 15, pflags));
  
  g_object_class_install_property (gobject_class, PROP_FILTER_LOW_PASS,
      g_param_spec_boolean ("low-pass", "LowPass", "Enable LowPass Filter",
          FALSE, pflags));

  g_object_class_install_property (gobject_class, PROP_FILTER_BAND_PASS,
      g_param_spec_boolean ("band-pass", "BandPass", "Enable BandPass Filter",
          FALSE, pflags));

  g_object_class_install_property (gobject_class, PROP_FILTER_HI_PASS,
      g_param_spec_boolean ("hi-pass", "HiPass", "Enable HiPass Filter",
          FALSE, pflags));

  g_object_class_install_property (gobject_class, PROP_VOICE_3_OFF,
      g_param_spec_boolean ("voice3-off", "Voice3Off", 
          "Detach voice 3 from mixer",  FALSE, pflags));
}

static void
gst_sid_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSidSyn *src = GSTBT_SID_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_CHILDREN:
      break;
    case PROP_CUTOFF:
      src->cutoff = g_value_get_uint (value);
      break;
    case PROP_RESONANCE:
      src->resonance = g_value_get_uint (value);
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_uint (value);
      break;
    case PROP_FILTER_LOW_PASS:
      src->filter_low_pass = g_value_get_boolean (value);
      break;
    case PROP_FILTER_BAND_PASS:
      src->filter_band_pass = g_value_get_boolean (value);
      break;
    case PROP_FILTER_HI_PASS:
      src->filter_hi_pass = g_value_get_boolean (value);
      break;
    case PROP_VOICE_3_OFF:
      src->voice_3_off = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sid_syn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtSidSyn *src = GSTBT_SID_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_CHILDREN:
      g_value_set_ulong (value, NUM_VOICES);
      break;
    case PROP_CUTOFF:
      g_value_set_uint (value, src->cutoff);
      break;
    case PROP_RESONANCE:
      g_value_set_uint (value, src->resonance);
      break;
    case PROP_VOLUME:
      g_value_set_uint (value, src->volume);
      break;
    case PROP_FILTER_LOW_PASS:
      g_value_set_boolean (value, src->filter_low_pass);
      break;
    case PROP_FILTER_BAND_PASS:
      g_value_set_boolean (value, src->filter_band_pass);
      break;
    case PROP_FILTER_HI_PASS:
      g_value_set_boolean (value, src->filter_hi_pass);
      break;
    case PROP_VOICE_3_OFF:
      g_value_set_boolean (value, src->voice_3_off);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sid_syn_init (GstBtSidSyn * src, GstBtSidSynClass * g_class)
{
  gint i;
  gchar name[7];

  src->clockrate = PALCLOCKRATE;
  src->emu = new SID;
  // make a proeprty?
  //src->emu->set_chip_model (MOS8580);
	src->emu->set_chip_model (MOS6581);
	src->n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_CROMATIC);
	
	for (i = 0; i < NUM_VOICES; i++) {
	  src->voices[i] = (GstBtSidSynV *) g_object_new (GSTBT_TYPE_SID_SYNV, NULL);
	  sprintf (name, "voice%1d",i);
	  gst_object_set_name ((GstObject *)src->voices[i],name);
	  gst_object_set_parent ((GstObject *)src->voices[i], (GstObject *)src);
	  GST_WARNING_OBJECT (src->voices[i], "created %p", src->voices[i]);
	}
	src->cutoff = 1024;
	src->resonance = 2;
	src->volume = 15;
}

#define NUM_REGS 29

static void
gst_sid_syn_update_regs (GstBtSidSyn *src)
{
  gint i;
  guchar regs[NUM_REGS] = {0, };
  gint filters = 0;

  for (i = 0; i < NUM_VOICES; i++) {
    GstBtSidSynV *v = src->voices[i];
        
    if (v->filter)
      filters |= (1 << i);

    if (v->note_set) {
      gdouble freq =
          gstbt_tone_conversion_translate_from_number (src->n2f, v->note);
      GST_INFO_OBJECT (src, "%1d: note-on: %d, %lf Hz", i, v->note, freq);
      v->note_set = FALSE;      
      if (freq >= 0.0) {
        // set up note (C-0 .. H-6)
        // PAL:  x = f * (18*2^24)/17734475 (0 - 3848 Hz)
        // NTSC: x = f * (14*2^24)/14318318 (0 - 3995 Hz)
        v->tone = (guint)((freq * (1L<<24)) / src->clockrate);
        v->gate = TRUE;
      } else {
        // stop voice
        v->gate = FALSE;
        v->tone = 0;
      }
    }
    regs[0x00 + i*7] = (guchar)(v->tone & 0xFF);
    regs[0x01 + i*7] = (guchar)(v->tone >> 8);
    GST_DEBUG_OBJECT (src, "%1d: tone: 0x%x", i, v->tone);
          
    regs[0x02 + i*7] = (guchar) (v->pulse_width & 0xFF);
    regs[0x03 + i*7] = (guchar) (v->pulse_width >> 8);
    regs[0x05 + i*7] = (guchar) ((v->attack << 4) | v->decay);
    regs[0x06 + i*7] = (guchar) ((v->sustain << 4) | v->release);
    regs[0x04 + i*7] = (guchar) ((v->wave << 4 )|
      (v->test << 3) | (v->ringmod << 2) | (v->sync << 1) | v->gate);
    GST_DEBUG ("%1d: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
      i, regs[0x00 + i*7], regs[0x01 + i*7], regs[0x02 + i*7], regs[0x03 + i*7],
      regs[0x04 + i*7], regs[0x05 + i*7], regs[0x06 + i*7]);
  }

  regs[0x15] = (guchar) (src->cutoff & 7);
  regs[0x16] = (guchar) (src->cutoff >> 3);
  regs[0x17] = (guchar) (src->resonance << 4) | filters;
  regs[0x18] = (guchar) ((src->voice_3_off << 7) | (src->filter_hi_pass << 6) |
      (src->filter_band_pass << 5) | (src->filter_low_pass << 4) | src->volume);
  
  for (i = 0; i < NUM_REGS; i++) {
    src->emu->write (i, regs[i]);
  }
}

static gboolean
gst_sid_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtSidSyn *src = ((GstBtSidSyn *) base);
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  /* set channels to 1 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels", 1))
    return FALSE;
  
  src->emu->set_sampling_parameters (src->clockrate, SAMPLE_FAST,
      base->samplerate);

  return TRUE;
}

static void
gst_sid_syn_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtSidSyn *src = ((GstBtSidSyn *) base);
  gint16 *out = (gint16 *) GST_BUFFER_DATA (data);
  gint i, n = base->generate_samples_per_buffer, samples = n;
  gdouble scale = (gdouble)src->clockrate / (gdouble)base->samplerate;

  for (i = 0; i < NUM_VOICES; i++) {
    gst_object_sync_values (G_OBJECT (src->voices[i]), 
        GST_BUFFER_TIMESTAMP (data));
  }
  gst_sid_syn_update_regs (src); 
  
  GST_INFO_OBJECT (src, "generate %d samples", n);

  while (samples > 0) {
    gint tdelta = (gint)(scale * samples) + 4;
    GST_INFO_OBJECT (src, "tdelta %d", tdelta);
    gint result = src->emu->clock (tdelta, out, n);
    out = &out[result];
    samples -= result;
  }
}

static void
gst_sid_syn_dispose (GObject * object)
{
  GstBtSidSyn *src = GSTBT_SID_SYN (object);
  gint i;

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;
  
  if (src->n2f)
    g_object_unref (src->n2f);
	for (i = 0; i < NUM_VOICES; i++) {
	  gst_object_unparent ((GstObject *)src->voices[i]);
	}
	
	delete src->emu;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GType
gstbt_sid_syn_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtSidSynClass),
      (GBaseInitFunc) gst_sid_syn_base_init,
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_sid_syn_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstBtSidSyn),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gst_sid_syn_init
    };
    const GInterfaceInfo child_proxy_interface_info = {
      (GInterfaceInitFunc) gst_sid_syn_child_proxy_interface_init,
      NULL,                    /* interface_finalize */
      NULL                     /* interface_data */
    };
    const GInterfaceInfo child_bin_interface_info = {
      NULL,                    /* interface_init */
      NULL,                    /* interface_finalize */
      NULL                     /* interface_data */
    };
    const GInterfaceInfo property_meta_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    type =
        g_type_register_static (GSTBT_TYPE_AUDIO_SYNTH, "GstBtSidSyn",
        &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static (type, GST_TYPE_CHILD_PROXY,
        &child_proxy_interface_info);
    g_type_add_interface_static (type, GSTBT_TYPE_CHILD_BIN,
        &child_bin_interface_info);
    g_type_add_interface_static (type, GSTBT_TYPE_PROPERTY_META,
        &property_meta_interface_info);
  }
  return type;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "sidsyn",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "c64 sid synthesizer");

  /* initialize gst controller library */
  gst_controller_init (NULL, NULL);

  return gst_element_register (plugin, "sidsyn", GST_RANK_NONE,
      GSTBT_TYPE_SID_SYN);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "sidsyn",
    "c64 sid synthesizer",
    plugin_init, VERSION, "GPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
