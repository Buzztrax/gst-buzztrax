/* GStreamer
 * Copyright (C) 2007 Josh Green <josh@users.sf.net>
 *
 * Adapted from symsyn synthesizer plugin in gst-buzztard source.
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * gstfluidsynth.c: GStreamer wrapper for FluidSynth
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
 * SECTION:gstfluidsynth
 * @short_description: FluidSynth GStreamer wrapper
 *
 * FluidSynth is a SoundFont 2 capable wavetable synthesizer.
 *
 * gst-launch fluidsynth num-buffers=100 note="c-3" ! alsasink
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

//#include <gst/childbin/childbin.h>
#include "help/help.h"
#include "propertymeta/propertymeta.h"
#include "preset/preset.h"
#include "tempo/tempo.h"

#include "gstfluidsynth.h"

/* FIXME - Add i18n support? */
#define _(str)  (str)

#define GST_CAT_DEFAULT fluidsynth_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static const GstElementDetails gst_fluidsynth_details =
GST_ELEMENT_DETAILS ("FluidSynth",
  "Source/Audio",
  "FluidSynth wavetable synthesizer",
  "Josh Green <josh@users.sf.net>");

enum {
  // static class properties
  PROP_SAMPLES_PER_BUFFER=1,
  PROP_IS_LIVE,
  PROP_TIMESTAMP_OFFSET,
  // tempo iface
  PROP_BPM,
  PROP_TPB,
  PROP_STPT,
  // dynamic class properties
  PROP_NOTE,
  PROP_NOTE_LENGTH,
  PROP_NOTE_VELOCITY,
  PROP_INTERP,
  PROP_REVERB_ENABLE,
  PROP_REVERB_PRESET,
  PROP_REVERB_ROOM_SIZE,
  PROP_REVERB_DAMP,
  PROP_REVERB_WIDTH,
  PROP_REVERB_LEVEL,
  PROP_CHORUS_ENABLE,
  PROP_CHORUS_PRESET,
  PROP_CHORUS_COUNT,
  PROP_CHORUS_LEVEL,
  PROP_CHORUS_FREQ,
  PROP_CHORUS_DEPTH,
  PROP_CHORUS_WAVEFORM,
};

/* number to use for first dynamic (FluidSynth settings) property */
#define FIRST_DYNAMIC_PROP  256

#define INTERPOLATION_TYPE interp_mode_get_type ()
#define CHORUS_WAVEFORM_TYPE chorus_waveform_get_type ()

static GstStaticPadTemplate gst_fluidsynth_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16, " "rate = (int) [ 1, MAX ], " "channels = (int) 2")
    );

static GstBaseSrcClass *parent_class = NULL;

static GType interp_mode_get_type (void);
static GType chorus_waveform_get_type (void);

static void gst_fluidsynth_base_init (gpointer klass);
static void gst_fluidsynth_class_init (GstFluidsynthClass *klass);
static void settings_foreach_count (void *data, char *name, int type);
static void settings_foreach_func (void *data, char *name, int type);
static void gst_fluidsynth_init (GstFluidsynth *object, GstFluidsynthClass *klass);

static void gst_fluidsynth_update_reverb (GstFluidsynth *gstsynth);
static void gst_fluidsynth_update_chorus (GstFluidsynth *gstsynth);

static void gst_fluidsynth_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_fluidsynth_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_fluidsynth_dispose (GObject *object);

static gboolean gst_fluidsynth_setcaps (GstBaseSrc * basesrc, GstCaps * caps);
static void gst_fluidsynth_src_fixate (GstPad * pad, GstCaps * caps);

static gboolean gst_fluidsynth_is_seekable (GstBaseSrc * basesrc);
static gboolean gst_fluidsynth_do_seek (GstBaseSrc * basesrc,
                                        GstSegment * segment);
static gboolean gst_fluidsynth_query (GstBaseSrc * basesrc, GstQuery * query);

static void gst_fluidsynth_get_times (GstBaseSrc * basesrc,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_fluidsynth_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);


/* last dynamic property ID (incremented for each dynamically installed prop) */
static int last_property_id = FIRST_DYNAMIC_PROP;

/* stores all dynamic FluidSynth setting names for mapping between property
   ID and FluidSynth setting */
static char **dynamic_prop_names;


static GType
interp_mode_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] =
  {
    { FLUID_INTERP_NONE,
      "GST_FLUIDSYNTH_INTERP_NONE", "None" },
    { FLUID_INTERP_LINEAR,
      "GST_FLUIDSYNTH_INTERP_LINEAR", "Linear" },
    { FLUID_INTERP_4THORDER,
      "GST_FLUIDSYNTH_INTERP_4THORDER", "4th Order" },
    { FLUID_INTERP_7THORDER,
      "GST_FLUIDSYNTH_INTERP_7THORDER", "7th Order" },
    { 0, NULL, NULL }
  };

  if (!G_UNLIKELY (type))
    type = g_enum_register_static ("GstFluidsynthInterp", values);

  return (type);
}

static GType
chorus_waveform_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] =
  {
    { FLUID_CHORUS_MOD_SINE,
      "GST_FLUIDSYNTH_CHORUS_MOD_SINE", "Sine" },
    { FLUID_CHORUS_MOD_TRIANGLE,
      "GST_FLUIDSYNTH_CHORUS_MOD_TRIANGLE", "Triangle" },
    { 0, NULL, NULL }
  };

  /* initialize the type info structure for the enum type */
  if (!G_UNLIKELY (type))
    type = g_enum_register_static ("GstFluidsynthChorusWaveform", values);

  return (type);
}

static void gst_fluidsynth_calculate_buffer_frames(GstFluidsynth *self)
{
  self->samples_per_buffer = ((self->samplerate*60.0)/(gdouble)(self->beats_per_minute*self->ticks_per_beat));
  self->ticktime=((GST_SECOND*60)/(GstClockTime)(self->beats_per_minute*self->ticks_per_beat));
  g_object_notify(G_OBJECT(self),"samplesperbuffer");
  GST_DEBUG("samples_per_buffer=%lf",self->samples_per_buffer);
}

static void gst_fluidsynth_tempo_change_tempo(GstTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstFluidsynth *self=GST_FLUIDSYNTH(tempo);
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
    gst_fluidsynth_calculate_buffer_frames(self);
  }
}

static void gst_fluidsynth_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_fluidsynth_tempo_change_tempo;
}

//-- helper

static guint gst_fluidsynth_note_string_2_value (const gchar *note) {
  guint tone, octave;

  g_return_val_if_fail(note,0);
  g_return_val_if_fail((strlen(note)==3),0);
  g_return_val_if_fail((note[1]=='-' || note[1]=='#'),0);

  // parse note
  switch(note[0]) {
    case 'c':
    case 'C':
      tone=(note[1]=='-')?0:1;
      break;
    case 'd':
    case 'D':
      tone=(note[1]=='-')?2:3;
      break;
    case 'e':
    case 'E':
      tone=4;
      break;
    case 'f':
    case 'F':
      tone=(note[1]=='-')?5:6;
      break;
    case 'g':
    case 'G':
      tone=(note[1]=='-')?7:8;
      break;
    case 'a':
    case 'A':
      tone=(note[1]=='-')?9:10;
      break;
    case 'b':
    case 'B':
    case 'h':
    case 'H':
      tone=11;
      break;
    default:
      g_return_val_if_reached(0.0);
  }
  octave=atoi(&note[2]);
  return ((octave*12)+tone);
}

#if 0
static const gchar *gst_fluidsynth_note_value_2_string (guint note) {
  guint tone, octave;
  static gchar str[4];
  static gchar *key[12]= { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };

  //g_return_val_if_fail(note<((12*9)+12),0);

  note-=1;
  octave=note/12;
  tone=note-(octave*12);

  sprintf(str,"%2s%1d",key[tone],octave);
  return(str);
}
#endif

//-- fluidsynth implementation

static void
gst_fluidsynth_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fluidsynth_src_template));
  gst_element_class_set_details (element_class, &gst_fluidsynth_details);
}

/* used for passing multiple values to FluidSynth foreach function */
typedef struct
{
  fluid_settings_t *settings;
  GObjectClass *klass;
} ForeachBag;

static void
gst_fluidsynth_class_init (GstFluidsynthClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GParamSpec *paramspec;
  ForeachBag bag;
  int count = 0;

  parent_class = (GstBaseSrcClass *) g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_fluidsynth_set_property;
  gobject_class->get_property = gst_fluidsynth_get_property;
  gobject_class->dispose      = gst_fluidsynth_dispose;

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_fluidsynth_setcaps);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_fluidsynth_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_fluidsynth_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_fluidsynth_query);
  gstbasesrc_class->get_times = GST_DEBUG_FUNCPTR (gst_fluidsynth_get_times);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_fluidsynth_create);

  /* used for dynamically installing settings (required for settings queries) */
  bag.settings = new_fluid_settings ();

  /* count the number of FluidSynth properties */
  fluid_settings_foreach (bag.settings, &count, settings_foreach_count);

  /* have to keep a mapping of property IDs to FluidSynth setting names, since
     GObject properties get mangled ('.' turns to '-') */
  dynamic_prop_names = g_malloc (count * sizeof (char *));

  bag.klass = gobject_class;

  /* add all FluidSynth settings as class properties */
  fluid_settings_foreach (bag.settings, &bag, settings_foreach_func);

  delete_fluid_settings (bag.settings);         /* not needed anymore */

  // override interface properties
  g_object_class_override_property(gobject_class, PROP_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, PROP_STPT, "subticks-per-tick");

  // register own properties

  g_object_class_install_property(gobject_class, PROP_SAMPLES_PER_BUFFER,
        g_param_spec_int("samplesperbuffer", _("Samples per buffer"),
          _("Number of samples in each outgoing buffer"),
          1, G_MAXINT, 1024, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", _("Is Live"),
          _("Whether to act as a live source"), FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMESTAMP_OFFSET,
      g_param_spec_int64 ("timestamp-offset", _("Timestamp offset"),
          _("An offset added to timestamps set on buffers (in ns)"), G_MININT64,
          G_MAXINT64, 0, G_PARAM_READWRITE));

  paramspec = g_param_spec_string ("note", _("Musical note"),
                                   _("Musical note (e.g. 'c-3', 'd#4')"),
                                   NULL, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_param_spec_set_qdata (paramspec, gst_property_meta_quark_flags,
                          GINT_TO_POINTER (GST_PROPERTY_META_NONE));
  g_param_spec_set_qdata (paramspec, gst_property_meta_quark_no_val, NULL);
  g_object_class_install_property (gobject_class, PROP_NOTE, paramspec);

  g_object_class_install_property (gobject_class, PROP_NOTE_LENGTH,
                        g_param_spec_long ("note-length", _("Note length"),
                                          _("Length of a note in ticks (buffers)"),
                                          1, 100, 4,
                                          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_NOTE_VELOCITY,
                        g_param_spec_int ("note-velocity", _("Note velocity"),
                                          _("Velocity of a note"),
                                          0, 127, 100,
                                          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_INTERP,
                        g_param_spec_enum ("interp", _("Interpolation"),
                                           _("Interpolation type"),
                                           INTERPOLATION_TYPE,
                                           FLUID_INTERP_DEFAULT,
                                           G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REVERB_ENABLE,
              g_param_spec_boolean ("reverb-enable", _("Reverb enable"),
                                   _("Reverb enable"),
                                   TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REVERB_ROOM_SIZE,
                g_param_spec_double ("reverb-room-size", _("Reverb room size"),
                                     _("Reverb room size"),
                                     0.0, 1.2, 0.4, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REVERB_DAMP,
                g_param_spec_double ("reverb-damp", _("Reverb damp"),
                                     _("Reverb damp"),
                                     0.0, 1.0, 0.0, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REVERB_WIDTH,
                        g_param_spec_double ("reverb-width", _("Reverb width"),
                                             _("Reverb width"),
                                             0.0, 100.0, 2.0,
                                             G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_REVERB_LEVEL,
                        g_param_spec_double ("reverb-level", _("Reverb level"),
                                             _("Reverb level"),
                                             -30.0, 30.0, 4.0,
                                             G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHORUS_ENABLE,
                        g_param_spec_boolean ("chorus-enable", _("Chorus enable"),
                                             _("Chorus enable"),
                                             TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHORUS_COUNT,
                        g_param_spec_int ("chorus-count", _("Chorus count"),
                                          _("Number of chorus delay lines"),
                                          1, 99, 3, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHORUS_LEVEL,
                g_param_spec_double ("chorus-level", _("Chorus level"),
                                     _("Output level of each chorus line"),
                                     0.0, 10.0, 2.0, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHORUS_FREQ,
                g_param_spec_double ("chorus-freq", _("Chorus freq"),
                                     _("Chorus modulation frequency (Hz)"),
                                     0.3, 5.0, 0.3, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHORUS_DEPTH,
                        g_param_spec_double ("chorus-depth", _("Chorus depth"),
                                             _("Chorus depth"),
                                             0.0, 10.0, 8.0,
                                             G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHORUS_WAVEFORM,
                g_param_spec_enum ("chorus-waveform", _("Chorus waveform"),
                                   _("Chorus waveform type"),
                                   CHORUS_WAVEFORM_TYPE,
                                   FLUID_CHORUS_MOD_SINE,
                                   G_PARAM_READWRITE));
}

/* for counting the number of FluidSynth settings properties */
static void
settings_foreach_count (void *data, char *name, int type)
{
  int *count = (int *)data;
  *count = *count + 1;
}

/* add each FluidSynth setting as a GObject property */
static void
settings_foreach_func (void *data, char *name, int type)
{
  ForeachBag *bag = (ForeachBag *)data;
  GParamSpec *spec;
  double dmin, dmax, ddef;
  int imin, imax, idef;
  char *defstr;

  switch (type)
  {
  case FLUID_NUM_TYPE:
    fluid_settings_getnum_range (bag->settings, name, &dmin, &dmax);
    ddef = fluid_settings_getnum_default (bag->settings, name);
    spec = g_param_spec_double (name, name, name, dmin, dmax, ddef,
                                G_PARAM_READWRITE);
    break;
  case FLUID_INT_TYPE:
    fluid_settings_getint_range (bag->settings, name, &imin, &imax);
    idef = fluid_settings_getint_default (bag->settings, name);
    spec = g_param_spec_int (name, name, name, imin, imax, idef,
                             G_PARAM_READWRITE);
    break;
  case FLUID_STR_TYPE:
    defstr = fluid_settings_getstr_default (bag->settings, name);
    spec = g_param_spec_string (name, name, name, defstr, G_PARAM_READWRITE);
    break;
  case FLUID_SET_TYPE:
    g_warning ("Enum not handled for property '%s'", name);
    return;
  default:
    return;
  }

  /* install the property */
  g_object_class_install_property (bag->klass, last_property_id, spec);

  /* store the name to the property name array */
  dynamic_prop_names[last_property_id - FIRST_DYNAMIC_PROP] = g_strdup (name);

  last_property_id++;   /* advance the last dynamic property ID */
}

static void
gst_fluidsynth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFluidsynth *gstsynth = GST_FLUIDSYNTH (object);
  char *name;
  int retval;

  if (gstsynth->dispose_has_run) return;

  /* is it one of the dynamically installed properties? */
  if (prop_id >= FIRST_DYNAMIC_PROP && prop_id < last_property_id)
  {
    name = dynamic_prop_names[prop_id - FIRST_DYNAMIC_PROP];

    switch (G_PARAM_SPEC_VALUE_TYPE (pspec))
      {
        case G_TYPE_INT:
          retval = fluid_settings_setint (gstsynth->settings, name,
                                          g_value_get_int (value));
          break;
        case G_TYPE_DOUBLE:
          retval = fluid_settings_setnum (gstsynth->settings, name,
                                          g_value_get_double (value));
          break;
        case G_TYPE_STRING:
          retval = fluid_settings_setstr (gstsynth->settings, name,
                                          (char *)g_value_get_string (value));
          break;
        default:
          g_critical ("Unexpected FluidSynth dynamic property type");
          return;
      }

    if (!retval)
      g_critical ("FluidSynth failure while setting FluidSynth property '%s'", name);

    return;
  }

  switch (prop_id)
  {
    case PROP_INTERP:
      gstsynth->interp = g_value_get_enum (value);
      fluid_synth_set_interp_method (gstsynth->fluid, -1, gstsynth->interp);
      break;
    case PROP_REVERB_ENABLE:
      gstsynth->reverb_enable = g_value_get_boolean (value);
      fluid_synth_set_reverb_on (gstsynth->fluid, gstsynth->reverb_enable);
      break;
    case PROP_REVERB_ROOM_SIZE:
      gstsynth->reverb_room_size = g_value_get_double (value);
      gstsynth->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (gstsynth);
      break;
    case PROP_REVERB_DAMP:
      gstsynth->reverb_damp = g_value_get_double (value);
      gstsynth->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (gstsynth);
      break;
    case PROP_REVERB_WIDTH:
      gstsynth->reverb_width = g_value_get_double (value);
      gstsynth->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (gstsynth);
      break;
    case PROP_REVERB_LEVEL:
      gstsynth->reverb_level = g_value_get_double (value);
      gstsynth->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (gstsynth);
      break;
    case PROP_CHORUS_ENABLE:
      gstsynth->chorus_enable = g_value_get_boolean (value);
      fluid_synth_set_chorus_on (gstsynth->fluid, gstsynth->chorus_enable);
      break;
    case PROP_CHORUS_COUNT:
      gstsynth->chorus_count = g_value_get_int (value);
      gstsynth->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (gstsynth);
      break;
    case PROP_CHORUS_LEVEL:
      gstsynth->chorus_level = g_value_get_double (value);
      gstsynth->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (gstsynth);
      break;
    case PROP_CHORUS_FREQ:
      gstsynth->chorus_freq = g_value_get_double (value);
      gstsynth->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (gstsynth);
      break;
    case PROP_CHORUS_DEPTH:
      gstsynth->chorus_depth = g_value_get_double (value);
      gstsynth->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (gstsynth);
      break;
    case PROP_CHORUS_WAVEFORM:
      gstsynth->chorus_waveform = g_value_get_enum (value);
      gstsynth->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (gstsynth);
      break;
    case PROP_SAMPLES_PER_BUFFER:
      gstsynth->samples_per_buffer = (gdouble)g_value_get_int (value);
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (gstsynth),
      g_value_get_boolean (value));
      break;
    case PROP_TIMESTAMP_OFFSET:
      gstsynth->timestamp_offset = g_value_get_int64 (value);
      break;
    case PROP_NOTE:
      g_free (gstsynth->note);
      gstsynth->note = g_value_dup_string (value);
      if (gstsynth->note) {
        // start note-off counter
        gstsynth->cur_note_length = gstsynth->note_length;
        gstsynth->key = gst_fluidsynth_note_string_2_value(gstsynth->note);
        GST_INFO("new note: '%s' = %d",gstsynth->note,gstsynth->key);
        fluid_synth_noteon (gstsynth->fluid, /*chan*/ 0,  gstsynth->key, gstsynth->velocity);
      }
      break;
    case PROP_NOTE_LENGTH:
      gstsynth->note_length = g_value_get_long (value);
      break;
    case PROP_NOTE_VELOCITY:
      gstsynth->velocity = g_value_get_int (value);
      break;
	// tempo iface
    case PROP_BPM:
    case PROP_TPB:
    case PROP_STPT:
	  GST_WARNING("use gst_tempo_change_tempo()");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_fluidsynth_get_property (GObject * object, guint prop_id,
                             GValue * value, GParamSpec * pspec)
{
  GstFluidsynth *gstsynth = GST_FLUIDSYNTH (object);
  char *s;
  char *name;
  double d;
  int retval;
  int i;

  if (gstsynth->dispose_has_run) return;

  /* is it one of the dynamically installed properties? */
  if (prop_id >= FIRST_DYNAMIC_PROP && prop_id < last_property_id)
  {
    name = dynamic_prop_names[prop_id - FIRST_DYNAMIC_PROP];

    switch (G_PARAM_SPEC_VALUE_TYPE (pspec))
    {
      case G_TYPE_INT:
        retval = fluid_settings_getint (gstsynth->settings, name, &i);
        if (retval) g_value_set_int (value, i);
        break;
      case G_TYPE_DOUBLE:
        retval = fluid_settings_getnum (gstsynth->settings, name, &d);
        if (retval) g_value_set_double (value, d);
        break;
      case G_TYPE_STRING:
        retval = fluid_settings_getstr (gstsynth->settings, name, &s);
        if (retval) g_value_set_string (value, s);
        break;
      default:
        g_critical ("Unexpected FluidSynth dynamic property type");
        return;
    }

    if (!retval)
        g_critical ("Failed to get FluidSynth property '%s'", name);

    return;
  }

  switch (prop_id)
    {
    case PROP_INTERP:
      g_value_set_enum (value, gstsynth->interp);
      break;
    case PROP_REVERB_ENABLE:
      g_value_set_boolean (value, gstsynth->reverb_enable);
      break;
    case PROP_REVERB_ROOM_SIZE:
      g_value_set_double (value, gstsynth->reverb_room_size);
      break;
    case PROP_REVERB_DAMP:
      g_value_set_double (value, gstsynth->reverb_damp);
      break;
    case PROP_REVERB_WIDTH:
      g_value_set_double (value, gstsynth->reverb_width);
      break;
    case PROP_REVERB_LEVEL:
      g_value_set_double (value, gstsynth->reverb_level);
      break;
    case PROP_CHORUS_ENABLE:
      g_value_set_boolean (value, gstsynth->chorus_enable);
      break;
    case PROP_CHORUS_COUNT:
      g_value_set_int (value, gstsynth->chorus_count);
      break;
    case PROP_CHORUS_LEVEL:
      g_value_set_double (value, gstsynth->chorus_level);
      break;
    case PROP_CHORUS_FREQ:
      g_value_set_double (value, gstsynth->chorus_freq);
      break;
    case PROP_CHORUS_DEPTH:
      g_value_set_double (value, gstsynth->chorus_depth);
      break;
    case PROP_CHORUS_WAVEFORM:
      g_value_set_enum (value, gstsynth->chorus_waveform);
      break;
    case PROP_NOTE:
      g_value_set_string (value, gstsynth->note);
      break;
    case PROP_NOTE_LENGTH:
      g_value_set_long (value, gstsynth->note_length);
      break;
    case PROP_NOTE_VELOCITY:
      g_value_set_int (value, gstsynth->velocity);
      break;
    case PROP_SAMPLES_PER_BUFFER:
      g_value_set_int (value, (gint)gstsynth->samples_per_buffer);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (gstsynth)));
      break;
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int64 (value, gstsynth->timestamp_offset);
      break;
	// tempo iface
    case PROP_BPM:
      g_value_set_ulong(value, gstsynth->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong(value, gstsynth->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong(value, gstsynth->subticks_per_tick);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_fluidsynth_init (GstFluidsynth *gstsynth, GstFluidsynthClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (gstsynth);

  gst_pad_set_fixatecaps_function (pad, gst_fluidsynth_src_fixate);

  gstsynth->samples_per_buffer = 1024.0;
  gstsynth->generate_samples_per_buffer = (gint)gstsynth->samples_per_buffer;
  gstsynth->timestamp_offset = G_GINT64_CONSTANT (0);

  gstsynth->samplerate = 44100;
  gstsynth->beats_per_minute=120;
  gstsynth->ticks_per_beat=4;
  gstsynth->subticks_per_tick=1;
  gst_fluidsynth_calculate_buffer_frames (gstsynth);

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (gstsynth), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (gstsynth), FALSE);

  /* set base parameters */
  gstsynth->note = NULL;
  gstsynth->note_length = 4;
  gstsynth->velocity = 100;

  gstsynth->settings = new_fluid_settings ();

  /* create new FluidSynth */
  gstsynth->fluid = new_fluid_synth (gstsynth->settings);
  if (!gstsynth->fluid)
  {     /* FIXME - Element will likely crash if used after new_fluid_synth fails */
    g_critical ("Failed to create FluidSynth context");
    return;
  }

#if 0
  /* hook our sfloader */
  loader = g_malloc0 (sizeof (fluid_sfloader_t));
  loader->data = wavetbl;
  loader->free = sfloader_free;
  loader->load = sfloader_load_sfont;
  fluid_synth_add_sfloader (wavetbl->fluid, loader);
#endif

  /* create MIDI router to send MIDI to FluidSynth */
  gstsynth->midi_router =
    new_fluid_midi_router (gstsynth->settings,
                           fluid_synth_handle_midi_event,
                           (void *)gstsynth);
  if (gstsynth->midi_router)
    {
      fluid_synth_set_midi_router (gstsynth->fluid, gstsynth->midi_router);
      gstsynth->midi =
        new_fluid_midi_driver (gstsynth->settings,
                               fluid_midi_router_handle_midi_event,
                               (void *)(gstsynth->midi_router));
      if (!gstsynth->midi)
        g_warning ("Failed to create FluidSynth MIDI input driver");
    }
  else g_warning ("Failed to create MIDI input router");

  gst_fluidsynth_update_reverb (gstsynth);      /* update reverb settings */
  gst_fluidsynth_update_chorus (gstsynth);      /* update chorus settings */

  /* FIXME temporary for testing */
  {
    int res;

    //res=fluid_synth_sfload (gstsynth->fluid, "/usr/share/doc/libfluidsynth-dev/examples/example.sf2", TRUE);
    res=fluid_synth_sfload (gstsynth->fluid, "/usr/share/sounds/sf2/Vintage_Dreams_Waves_v2.sf2", TRUE);
    //g_strdup_printf("%s/sbks/synth/FlangerSaw.SF2",g_get_home_dir());
    //res=fluid_synth_sfload (gstsynth->fluid, "/home/josh/sbks/synth/FlangerSaw.SF2", TRUE);
    if(res==-1) {
      GST_WARNING("Couldn't load soundfont");
    }
    //fluid_synth_noteon (gstsynth->fluid, 0, 60, 127);
  }
}

static void
gst_fluidsynth_update_reverb (GstFluidsynth *gstsynth)
{
  if (!gstsynth->reverb_update) return;

  if (gstsynth->reverb_enable)
    fluid_synth_set_reverb (gstsynth->fluid, gstsynth->reverb_room_size,
                            gstsynth->reverb_damp, gstsynth->reverb_width,
                            gstsynth->reverb_level);

  fluid_synth_set_reverb_on (gstsynth->fluid, gstsynth->reverb_enable);
  gstsynth->reverb_update = FALSE;
}

static void
gst_fluidsynth_update_chorus (GstFluidsynth *gstsynth)
{
  if (!gstsynth->chorus_update) return;

  if (gstsynth->chorus_enable)
    fluid_synth_set_chorus (gstsynth->fluid, gstsynth->chorus_count,
			    gstsynth->chorus_level, gstsynth->chorus_freq,
			    gstsynth->chorus_depth, gstsynth->chorus_waveform);

  fluid_synth_set_chorus_on (gstsynth->fluid, TRUE);
  gstsynth->chorus_update = FALSE;
}

static void
gst_fluidsynth_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstFluidsynth *src = GST_FLUIDSYNTH (GST_PAD_PARENT (pad));
  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);
}

static gboolean
gst_fluidsynth_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstFluidsynth *src = GST_FLUIDSYNTH (basesrc);
  const GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "rate", &src->samplerate);

  return ret;
}

static gboolean
gst_fluidsynth_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstFluidsynth *src = GST_FLUIDSYNTH (basesrc);
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
gst_fluidsynth_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
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
gst_fluidsynth_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstFluidsynth *src = GST_FLUIDSYNTH (basesrc);
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
  src->seek_flags = segment->flags;
  src->eos_reached = FALSE;

  GST_DEBUG("seek from %"GST_TIME_FORMAT" to %"GST_TIME_FORMAT,
    GST_TIME_ARGS(segment->start),GST_TIME_ARGS(segment->stop));

  return TRUE;
}

static gboolean
gst_fluidsynth_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static GstFlowReturn
gst_fluidsynth_create (GstBaseSrc * basesrc, guint64 offset, guint length,
		       GstBuffer ** buffer)
{
  GstFlowReturn res;
  GstFluidsynth *src = GST_FLUIDSYNTH (basesrc);
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples,samples_done;
  guint samples_per_buffer = length;

  if (G_UNLIKELY(src->eos_reached)) {
    GST_DEBUG("  EOS reached");
    return GST_FLOW_UNEXPECTED;
  }

  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  //GST_DEBUG("rounding correction : %ld <> %"G_GUINT64_FORMAT,(glong)(((src->timestamp_offset+src->running_time)*src->samplerate)/GST_SECOND),src->n_samples);
  //samples_per_buffer=src->samples_per_buffer+(((src->running_time*src->samplerate)/GST_SECOND)-src->timestamp_offset);
  samples_done = gst_util_uint64_scale ((src->timestamp_offset + src->running_time), (guint64)src->samplerate, GST_SECOND);
  samples_per_buffer = (gint)(src->samples_per_buffer + (gdouble)(src->n_samples-samples_done));
  //GST_DEBUG("  samplers-per-buffer = %7d (%8.3lf)",samples_per_buffer,src->samples_per_buffer);

  /* check for eos */
  if (src->check_seek_stop &&
    (src->n_samples_stop > src->n_samples) &&
    (src->n_samples_stop < src->n_samples + samples_per_buffer))
  {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer = src->n_samples_stop - src->n_samples;
    n_samples = src->n_samples_stop;

    if (!(src->seek_flags & GST_SEEK_FLAG_SEGMENT))
      src->eos_reached = TRUE;
  }
  else
  {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples = src->n_samples + samples_per_buffer;
  }

  //next_time = n_samples * GST_SECOND / src->samplerate;
  next_time = gst_util_uint64_scale (n_samples, GST_SECOND, (guint64)src->samplerate);

  /* allocate a new buffer suitable for this pad */
  if ((res = gst_pad_alloc_buffer_and_set_caps (basesrc->srcpad, src->n_samples,
      src->generate_samples_per_buffer * sizeof (gint16) * 2,
      GST_PAD_CAPS (basesrc->srcpad),
      &buf)) != GST_FLOW_OK)
    return res;

  GST_BUFFER_TIMESTAMP (buf) = src->timestamp_offset + src->running_time;
  GST_BUFFER_OFFSET_END (buf) = n_samples;
  GST_BUFFER_DURATION (buf) = next_time - src->running_time;

  gst_object_sync_values (G_OBJECT (src), src->running_time);

  GST_DEBUG("n_samples %12"G_GUINT64_FORMAT", running_time %"GST_TIME_FORMAT", next_time %"GST_TIME_FORMAT", duration %"GST_TIME_FORMAT,
    src->n_samples,GST_TIME_ARGS(src->running_time),GST_TIME_ARGS(next_time),GST_TIME_ARGS(GST_BUFFER_DURATION (buf)));

  src->running_time = next_time;
  src->n_samples = n_samples;

  if(src->cur_note_length) {
    src->cur_note_length--;
    if(!src->cur_note_length) {
      fluid_synth_noteoff(src->fluid, /*chan*/ 0, src->key);
      GST_INFO("note-off: %d",src->key);
    }
  }

  fluid_synth_write_s16 (src->fluid, src->generate_samples_per_buffer, GST_BUFFER_DATA(buf), 0, 2, GST_BUFFER_DATA(buf), 1, 2);

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
gst_fluidsynth_dispose (GObject *object)
{
  GstFluidsynth *src = GST_FLUIDSYNTH (object);

  if (src->dispose_has_run) return;
  src->dispose_has_run = TRUE;

  if (src->midi) delete_fluid_midi_driver (src->midi);
  if (src->midi_router) delete_fluid_midi_router (src->midi_router);
  if (src->fluid) delete_fluid_synth (src->fluid);

  src->midi = NULL;
  src->midi_router = NULL;
  src->fluid = NULL;

  if (G_OBJECT_CLASS (parent_class)->dispose)
    G_OBJECT_CLASS (parent_class)->dispose (object);
}

GType gst_fluidsynth_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY(!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstFluidsynthClass),
      (GBaseInitFunc)gst_fluidsynth_base_init,
      NULL,                       /* base_finalize */
      (GClassInitFunc)gst_fluidsynth_class_init,
      NULL,                       /* class_finalize */
      NULL,               /* class_data */
      sizeof (GstFluidsynth),
      0,                  /* n_preallocs */
      (GInstanceInitFunc) gst_fluidsynth_init
    };
    const GInterfaceInfo property_meta_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) gst_fluidsynth_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo preset_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

    type = g_type_register_static(GST_TYPE_BASE_SRC, "GstFluidsynth", &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static(type, GST_TYPE_PROPERTY_META, &property_meta_interface_info);
    g_type_add_interface_static(type, GST_TYPE_TEMPO, &tempo_interface_info);
    g_type_add_interface_static(type, GST_TYPE_PRESET, &preset_interface_info);
  }
  return type;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "fluidsynth", GST_DEBUG_FG_WHITE
                          | GST_DEBUG_BG_BLACK, "FluidSynth wavetable synthesizer");

  /* initialize gst controller library */
  gst_controller_init(NULL,NULL);

  return gst_element_register (plugin, "fluidsynth",
                               GST_RANK_NONE, GST_TYPE_FLUIDSYNTH);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "fluidsynth",
    "FluidSynth wavetable synthesizer",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
