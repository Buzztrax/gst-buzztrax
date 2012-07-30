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
 * http://www.waitingforfriday.com/index.php/Commodore_SID_6581_Datasheet.
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

#define M_PI_M2 ( M_PI + M_PI )
#define NUM_STEPS 6

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

//-- property meta interface implementations

static gchar *
gst_sid_syn_property_meta_describe_property (GstBtPropertyMeta * property_meta,
    guint prop_id, const GValue * value)
{
  //GstBtSidSyn *src = GSTBT_SID_SYN (property_meta);
  gchar *res = NULL;

  switch (prop_id) {
    case PROP_CUTOFF:
      res = g_strdup_printf ("%7.1lf Hz",
          30.0 + (gfloat) g_value_get_uint (value) * 10000.0/2047.0);
      break;
    default:
      break;
  }
  return res;
}

static void
gst_sid_syn_property_meta_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GstBtPropertyMetaInterface *const iface =
      (GstBtPropertyMetaInterface * const) g_iface;

  GST_INFO ("initializing iface");

  iface->describe_property = gst_sid_syn_property_meta_describe_property;

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
  guint filters = 0;
  guint subticks = NUM_STEPS * ((GstBtAudioSynth *)src)->subticks_per_tick;

  for (i = 0; i < NUM_VOICES; i++) {
    GstBtSidSynV *v = src->voices[i];
    guint tone;
        
    if (v->filter)
      filters |= (1 << i);

    // init note
    if (v->note_set) {
      // set up note (C-0 .. H-6)
      v->prev_freq = v->freq;
      v->freq = gstbt_tone_conversion_translate_from_number (src->n2f, v->note);
      GST_INFO_OBJECT (src, "%1d: note-on: %d, %lf Hz", i, v->note, v->freq);
      if (v->freq > 0.0) {
        v->gate = TRUE;
      } else {
        // stop voice
        v->gate = FALSE;
      }        
      v->note_set = FALSE;      
    }
    // init effects
    if (v->effect_set) {
      guint value = v->effect_value;
      // arpeggio, portamento and vibrato is only active until the next tick
      switch (v->effect_type) {
      case GSTBT_SID_SYN_EFFECT_ARPEGGIO: {
          if (value > 0) {
            // need to use _prev_note to also work after note_off
            guint note = 
                gstbt_tone_conversion_note_number_offset (v->prev_note, (value & 0xF));
            v->arp_freq[0] = v->finetune *
              gstbt_tone_conversion_translate_from_number (src->n2f, note);
            note = 
              gstbt_tone_conversion_note_number_offset (v->prev_note, (value >> 4));
            v->arp_freq[1] = v->finetune *
              gstbt_tone_conversion_translate_from_number (src->n2f, note);          
            v->arp_freq[2] = v->freq;
            v->fx_ticks_remain = subticks;
            GST_INFO_OBJECT (src, "%1d: arpeggio: %d + %d -> %lf, %lf, %lf",
              i, v->prev_note, value, v->arp_freq[0],v->arp_freq[1],v->arp_freq[2]);
          } else {
            v->effect_type = GSTBT_SID_SYN_EFFECT_NONE;
          }
        } break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP:
          v->portamento = pow (2.0, (value / 512.0)); // <- relate this to the subticks
          v->fx_ticks_remain = subticks;
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN:
          v->portamento = 1.0 / pow (2.0, (value / 512.0));
          v->fx_ticks_remain = subticks;
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO:
          if (v->prev_freq > 0.0 && v->freq > 0.0) {
            if (value > 0) {
              if (v->freq > v->prev_freq) {
                v->portamento = pow (2.0, (value / 512.0));
              } else {
                v->portamento = 1.0 / pow (2.0, (value / 512.0));
              }
              v->want_freq = v->freq;
              v->freq = v->prev_freq;
            }
            v->fx_ticks_remain = subticks;
            GST_INFO_OBJECT (src, "%1d: portamento: %d -> %lf, %lf -> %lf",
              i, value, v->portamento, v->freq, v->want_freq);
          } else {
            v->effect_type = GSTBT_SID_SYN_EFFECT_NONE;
          }
          break;
        case GSTBT_SID_SYN_EFFECT_VIBRATO:
          if (value >0) {
            guint val;
            v->vib_pos = 0.0;
            v->vib_center = v->freq;
            val = value >> 4;
            if (val > 0) {
              // 0xF: 1 cycle per tick
              v->vib_speed = (val * val) * M_PI_M2 / 225.0;
            }
            val = value & 0xF;
            if (val > 0) {
              v->vib_depth = (val * val) / 700.0;
            }
          }
          v->fx_ticks_remain = subticks;
          break;
        case GSTBT_SID_SYN_EFFECT_GLISSANDO_CONTROL:
          break;
        case GSTBT_SID_SYN_EFFECT_VIBRATO_TYPE:
          break;
        case GSTBT_SID_SYN_EFFECT_FINETUNE:
          // center around 128
          if (value >= 128) {
            v->finetune = pow (2.0,((value-128)/256.0)/12.0);
          } else {
            v->finetune = 1.0 / pow (2.0,((128-value)/256.0)/12.0);
          }
          GST_INFO_OBJECT (src, "%1d: finetune: %d -> %lf", i, value,
              v->finetune);
          v->freq *= v->finetune;
          break;
        default:
          break;;
      }
      v->effect_set = FALSE;
    }
    // apply effects
    if (v->fx_ticks_remain) {
      GST_INFO_OBJECT (src, "%1d: fx 0x%02x ticks left %2d", i,
        v->effect_type, v->fx_ticks_remain);
      switch (v->effect_type) {
        case GSTBT_SID_SYN_EFFECT_ARPEGGIO:
          v->freq = v->arp_freq[v->fx_ticks_remain%3];
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP:
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN:
          v->freq *= v->portamento;
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO:
          v->freq *= v->portamento;
          if (((v->portamento > 1.0) && (v->freq > v->want_freq)) ||
              ((v->portamento < 1.0) && (v->freq < v->want_freq))) {
            v->freq = v->want_freq;
            v->effect_type = GSTBT_SID_SYN_EFFECT_NONE;
            GST_WARNING_OBJECT (src, "%1d: portamento done",i);
          }
          break;
        case GSTBT_SID_SYN_EFFECT_VIBRATO: {
          gdouble	depth = 0.0;
        
          switch (v->vib_type) {
            case 0:
              depth = sin (v->vib_pos);
              break;
            case 1:
              depth = v->vib_pos / M_PI - 1.0;
              break;
            case 2:
              depth = v->vib_pos >= M_PI ? 1.0 : -1.0;
              break;
            default:
              break;
          }
          depth *= v->vib_depth;
          v->freq = v->vib_center * pow(2.0, depth);
          v->vib_pos += v->vib_speed;
          if (v->vib_pos >= M_PI_M2)
            v->vib_pos -= M_PI_M2;
        } break;
        default:
          break;
      }
      v->fx_ticks_remain--;
    }
    
    if (v->freq > 0.0) {
      GST_INFO_OBJECT (src, "%1d: freq: %lf Hz", i, v->freq);
      // PAL:  x = f * (18*2^24)/17734475 (0 - 3848 Hz)
      // NTSC: x = f * (14*2^24)/14318318 (0 - 3995 Hz)
      tone = (guint)((v->freq * (1L<<24)) / src->clockrate);
    } else {
      tone = 0;
    }

    regs[0x00 + i*7] = (guchar)(tone & 0xFF);
    regs[0x01 + i*7] = (guchar)(tone >> 8);
    GST_DEBUG_OBJECT (src, "%1d: tone: 0x%x", i, tone);
          
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
  gint i;

  /* set channels to 1 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels", 1))
    return FALSE;
  
  src->emu->set_sampling_parameters (src->clockrate, SAMPLE_FAST,
      base->samplerate);
  
  for (i = 0; i < NUM_VOICES; i++) {
    src->voices[i]->prev_freq = 0.0;
    src->voices[i]->want_freq = 0.0;
  }

  return TRUE;
}

// TODO(ensonic): silence detection (gate off + release time is over)
static void
gst_sid_syn_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtSidSyn *src = ((GstBtSidSyn *) base);
  gint16 *out = (gint16 *) GST_BUFFER_DATA (data);
  gint i, n, m, samples;
  gdouble scale = (gdouble)src->clockrate / (gdouble)base->samplerate;
  gint step = NUM_STEPS * (base->subtick_count - 1);
  gint step_mod = base->subticks_per_tick;
  gint fx_ticks_remain = 0;

  for (i = 0; i < NUM_VOICES; i++) {
    GstBtSidSynV *v = src->voices[i];
    gst_object_sync_values ((GObject *)v, GST_BUFFER_TIMESTAMP (data));
    fx_ticks_remain += v->fx_ticks_remain; 
  }
  
  GST_DEBUG_OBJECT (src, "generate %d samples (using %d subticks)", base->generate_samples_per_buffer, 
    base->subticks_per_tick);
  
  if (!fx_ticks_remain) {
    /* no need to subdivide if no effect runs */
    m = base->generate_samples_per_buffer;
    samples = m;
    
    GST_LOG_OBJECT (src, "subtick: %2d -- -- sync", step/6);
    gst_sid_syn_update_regs (src);

    while (samples > 0) {
      gint tdelta = (gint)(scale * samples) + 4;
      gint result = src->emu->clock (tdelta, out, m);
      out = &out[result];
      samples -= result;
    }
  } else {
    /* In trackers subtick is called 'speed' and is 6 by default, For arpeggio,
     * we want to ensure we have a multiple of 3. More subdivisions are good for
     * smooth fx anyway.
     */
    n = base->generate_samples_per_buffer / NUM_STEPS;
    m = base->generate_samples_per_buffer - ((NUM_STEPS - 1) * n);
    samples = m;
    for (i = 0; i < NUM_STEPS; i++, step++) {
      if ((step % step_mod) == 0) {
        GST_LOG_OBJECT (src, "subtick: %2d %2d %2d sync", step/6, i, (step % step_mod));
        gst_sid_syn_update_regs (src);
      } else {
        GST_LOG_OBJECT (src, "subtick: %2d %2d %2d", step/6, i, (step % step_mod));
      }
      while (samples > 0) {
        gint tdelta = (gint)(scale * samples) + 4;
        gint result = src->emu->clock (tdelta, out, m);
        out = &out[result];
        samples -= result;
      }
      samples = n;
    }
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
      (GInterfaceInitFunc) gst_sid_syn_property_meta_interface_init,
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
