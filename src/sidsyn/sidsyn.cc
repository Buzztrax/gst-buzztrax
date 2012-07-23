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
 * A synthesizer based on the RSID emulation library.
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

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

#define GST_CAT_DEFAULT sid_syn_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

enum
{
  PROP_CHILDREN = 1,
  PROP_CUTOFF,
  PROP_RESONANCE,
  PROP_VOLUME,
  PROP_MODE
};

#define GSTBT_TYPE_SID_SYN_MODE (gst_sid_syn_mode_get_type())
static GType
gst_sid_syn_mode_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SID_SYN_MODE_LOWPASS, "LowPass", "lowpass"},
    {GSTBT_SID_SYN_MODE_BANDPASS, "BandPass", "bandpass"},
    {GSTBT_SID_SYN_MODE_HIPASS, "HiPass", "hipass"},
    {GSTBT_SID_SYN_MODE_VOICE_3_OFF, "Voice3Off", "voice-3-off"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSidSynMode", enums);
  }
  return type;
}


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
  
  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode", "Filter/Osc. mode", 
          GSTBT_TYPE_SID_SYN_MODE,
          GSTBT_SID_SYN_MODE_LOWPASS, pflags));
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
    case PROP_MODE:
      src->mode = (GstBtSidSynMode) g_value_get_enum (value);
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
    case PROP_MODE:
      g_value_set_enum (value, (gint) src->mode);
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
	src->mode = GSTBT_SID_SYN_MODE_LOWPASS;
	src->volume = 15;
}

#define NUM_REGS 29

static void
gst_sid_syn_update_regs (GstBtSidSyn *src)
{
  gint i;
  guchar regs[NUM_REGS];
  //gint samplerate = ((GstBtAudioSynth *)src)->samplerate;
  const guchar freqtbllo[] = {
    0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
    0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
    0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
    0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
    0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
    0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
    0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
    0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  const guchar freqtblhi[] = {
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
    0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
    0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
    0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
    0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
    0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
    0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
    0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  gint filters = 0;
  for (gint t = 0; t < NUM_VOICES; t++) {
    GstBtSidSynV *voice = src->voices[t];
	  gint tone = 0, note = 0;
	  gint attack = voice->attack;
	  gint decay = voice->decay;
	  gint sustain = voice->sustain;
	  gint release = voice->release;
	  gint gate = voice->gate; 
	  gint sync = voice->sync;
	  gint ringmod = voice->ringmod;
	  GstBtSidSynWave wave = voice->wave;
        
    if (voice->filter)
      filters |= (1 << t);

    if (voice->note_set) {
      voice->note_set = FALSE;
      
      note = voice->note;
      gdouble freq =
          gstbt_tone_conversion_translate_from_number (src->n2f, note);
      GST_WARNING_OBJECT (src, "%1d: note-on: %d, %lf Hz", t, note, freq);
      if (freq >= 0.0) {
        // set up note
        //float tnote = voice->note;
        //float Fout = 440.0f * pow(2.0f, (tnote - 69.0f) / 12.0f) * 44100 / 44100;
        //Fout / 0.058725357f;	// Fout = (Fn * 0.058725357) Hz for PAL
        tone = freq;
        voice->gate = gate = TRUE;
        //GST_WARNING_OBJECT (src, "me: %lf, orig: %lf", freq, Fout);
      } else {
        // stop voice
        voice->gate = gate = FALSE;
      }
      regs[0x00 + t*7] = freqtbllo[note];
      regs[0x01 + t*7] = freqtblhi[note];
    }
          
    regs[0x02 + t*7] = (guchar) (voice->pulse_width & 0xFF);
    regs[0x03 + t*7] = (guchar) (voice->pulse_width >> 8);
    regs[0x05 + t*7] = (guchar) ((attack << 4) | decay);
    regs[0x06 + t*7] = (guchar) ((sustain << 4) | release);
    // we don't expose the test bit (1<<3)
    regs[0x04 + t*7] = (guchar) (wave | (ringmod << 2) | (sync << 1) | gate);
    GST_DEBUG ("%1d: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
      t, regs[0x00 + t*7], regs[0x01 + t*7], regs[0x02 + t*7], regs[0x03 + t*7],
      regs[0x04 + t*7], regs[0x05 + t*7], regs[0x06 + t*7]);
  }

  regs[0x15] = (guchar) (src->cutoff & 7);
  regs[0x16] = (guchar) (src->cutoff >> 3);
  regs[0x17] = (guchar) (src->resonance << 4) | filters;
  regs[0x18] = (guchar) ((1 << (gint)src->mode) << 4) | src->volume;
  
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
