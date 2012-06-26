/* GStreamer
 * Copyright (C) 2007 Josh Green <josh@users.sf.net>
 *
 * Adapted from simsyn synthesizer plugin in gst-buzztard source.
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
 * @title: GstFluidSynth
 * @short_description: FluidSynth GStreamer wrapper
 *
 * FluidSynth is a SoundFont 2 capable wavetable synthesizer. Soundpatches are
 * available on <ulink url="http://sounds.resonance.org">sounds.resonance.org</ulink>.
 * Distributions also have a few soundfonts packaged. The internet offers free
 * pacthes for download.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch fluidsynth num-buffers=100 note="c-3" ! alsasink
 * ]| Plays one c-3 tone using the first instrument.
 * |[
 * gst-launch fluidsynth num-buffers=20 instrument-patch="/usr/share/sounds/sf2/Vintage_Dreams_Waves_v2.sf2" program=2 note="c-3" ! alsasink
 * ]| Load a specific patch and plays one c-3 tone using the second program.
 * </refsect2>
 */
/*
 * for API look at /usr/share/doc/libfluidsynth-dev/examples/example.c
 * TODO:
 * - make it easier to load sounds fonts
 *   - where to get them from:
 *     - local files
 *       - ubuntu/suse use /usr/share/sounds/sf2
 *       - what about LIB_INSTPATCH_PATH/SF2_PATH to look for soundfonts
 *       - http://help.lockergnome.com/linux/Bug-348290-asfxload-handle-soundfont-search-path--ftopict218300.html
 *         SFBANKDIR
 *       - ask tracker or locate for installed *.sf2 files
 *       - include a tiny beep.sf2 file with the plugin
 *     - internet
 *       - http://sounds.resonance.org/patches.py
 *   - preset iface?
 *     - we would need a way to hide the name property
 *     - also the presets would be read-only, otherwise
 *       - rename would store it localy as new name
 *       - saving is sort of fake still as one can't change the content of the
 *         patch anyway
 *     - just have a huge enum with the located sound fonts as a controlable
 *       property?
 *       - not good as it won't map between installations
 *
 * - we would need a way to store the sf2 files with the song
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "libgstbuzztard/propertymeta.h"
#include "libgstbuzztard/tempo.h"
#if !GST_CHECK_VERSION(0,10,31)
#include <libgstbuzztard/help.h>
#endif

#include "gstfluidsynth.h"

/* FIXME - Add i18n support? */
#define _(str)  (str)

#define GST_CAT_DEFAULT fluidsynth_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // dynamic class properties
  PROP_NOTE = 1,
  PROP_NOTE_LENGTH,
  PROP_NOTE_VELOCITY,
#if !GST_CHECK_VERSION(0,10,31)
  // help interface
  PROP_DOCU_URI,
#endif
  PROP_PROGRAM,
  // not yet GST_PARAM_CONTROLLABLE
  PROP_INSTRUMENT_PATCH,
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
  PROP_CHORUS_WAVEFORM
};

/* number to use for first dynamic (FluidSynth settings) property */
#define FIRST_DYNAMIC_PROP  256

#define INTERPOLATION_TYPE interp_mode_get_type ()
#define CHORUS_WAVEFORM_TYPE chorus_waveform_get_type ()

static GstBtAudioSynthClass *parent_class = NULL;

static GType interp_mode_get_type (void);
static GType chorus_waveform_get_type (void);

static void gst_fluidsynth_base_init (gpointer klass);
static void gst_fluidsynth_class_init (GstBtFluidsynthClass * klass);
static void gst_fluidsynth_init (GstBtFluidsynth * object,
    GstBtFluidsynthClass * klass);

static void gst_fluidsynth_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_fluidsynth_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_fluidsynth_dispose (GObject * object);
static void gst_fluidsynth_process (GstBtAudioSynth * base, GstBuffer * data);
static gboolean gst_fluidsynth_setup (GstPad * pad, GstCaps * caps);

static void gst_fluidsynth_update_reverb (GstBtFluidsynth * gstsynth);
static void gst_fluidsynth_update_chorus (GstBtFluidsynth * gstsynth);

/* last dynamic property ID (incremented for each dynamically installed prop) */
static int last_property_id = FIRST_DYNAMIC_PROP;

/* stores all dynamic FluidSynth setting names for mapping between property
   ID and FluidSynth setting */
static char **dynamic_prop_names;

/* fluidsynth log handler */
static void
gst_fluidsynth_error_log_function (int level, char *message, void *data)
{
  GST_ERROR ("%s", message);
}

static void
gst_fluidsynth_warning_log_function (int level, char *message, void *data)
{
  GST_WARNING ("%s", message);
}

static void
gst_fluidsynth_info_log_function (int level, char *message, void *data)
{
  GST_INFO ("%s", message);
}

static void
gst_fluidsynth_debug_log_function (int level, char *message, void *data)
{
  GST_DEBUG ("%s", message);
}

static GType
interp_mode_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] = {
    {FLUID_INTERP_NONE,
        "GSTBT_FLUIDSYNTH_INTERP_NONE", "None"},
    {FLUID_INTERP_LINEAR,
        "GSTBT_FLUIDSYNTH_INTERP_LINEAR", "Linear"},
    {FLUID_INTERP_4THORDER,
        "GSTBT_FLUIDSYNTH_INTERP_4THORDER", "4th Order"},
    {FLUID_INTERP_7THORDER,
        "GSTBT_FLUIDSYNTH_INTERP_7THORDER", "7th Order"},
    {0, NULL, NULL}
  };

  if (!G_UNLIKELY (type))
    type = g_enum_register_static ("GstBtFluidsynthInterp", values);

  return (type);
}

static GType
chorus_waveform_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] = {
    {FLUID_CHORUS_MOD_SINE,
        "GSTBT_FLUIDSYNTH_CHORUS_MOD_SINE", "Sine"},
    {FLUID_CHORUS_MOD_TRIANGLE,
        "GSTBT_FLUIDSYNTH_CHORUS_MOD_TRIANGLE", "Triangle"},
    {0, NULL, NULL}
  };

  /* initialize the type info structure for the enum type */
  if (!G_UNLIKELY (type))
    type = g_enum_register_static ("GstBtFluidsynthChorusWaveform", values);

  return (type);
}


//-- fluidsynth implementation

static void
gst_fluidsynth_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class,
      "FluidSynth",
      "Source/Audio",
      "FluidSynth wavetable synthesizer", "Josh Green <josh@users.sf.net>");
#if GST_CHECK_VERSION(0,10,31)
  gst_element_class_set_documentation_uri (element_class,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "" PACKAGE
      "-GstFluidSynth.html");
#endif
}

/* used for passing multiple values to FluidSynth foreach function */
typedef struct
{
  fluid_settings_t *settings;
  GObjectClass *klass;
} ForeachBag;


/* for counting the number of FluidSynth settings properties */
static void
settings_foreach_count (void *data, char *name, int type)
{
  int *count = (int *) data;
  *count = *count + 1;
}

/* add each FluidSynth setting as a GObject property */
static void
settings_foreach_func (void *data, char *name, int type)
{
  ForeachBag *bag = (ForeachBag *) data;
  GParamSpec *spec;
  double dmin, dmax, ddef;
  int imin, imax, idef;
  char *defstr;

  switch (type) {
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

  last_property_id++;           /* advance the last dynamic property ID */
}

static void
gst_fluidsynth_class_init (GstBtFluidsynthClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  ForeachBag bag;
  gint count = 0;

  parent_class = (GstBtAudioSynthClass *) g_type_class_peek_parent (klass);

  audio_synth_class->process = gst_fluidsynth_process;
  audio_synth_class->setup = gst_fluidsynth_setup;

  gobject_class->set_property = gst_fluidsynth_set_property;
  gobject_class->get_property = gst_fluidsynth_get_property;
  gobject_class->dispose = gst_fluidsynth_dispose;

  /* set a log handler */
#ifndef GST_DISABLE_GST_DEBUG
  fluid_set_log_function (FLUID_PANIC, gst_fluidsynth_error_log_function, NULL);
  fluid_set_log_function (FLUID_ERR, gst_fluidsynth_warning_log_function, NULL);
  fluid_set_log_function (FLUID_WARN, gst_fluidsynth_warning_log_function,
      NULL);
  fluid_set_log_function (FLUID_INFO, gst_fluidsynth_info_log_function, NULL);
  fluid_set_log_function (FLUID_DBG, gst_fluidsynth_debug_log_function, NULL);
#else
  fluid_set_log_function (FLUID_PANIC, NULL, NULL);
  fluid_set_log_function (FLUID_ERR, NULL, NULL);
  fluid_set_log_function (FLUID_WARN, NULL, NULL);
  fluid_set_log_function (FLUID_INFO, NULL, NULL);
  fluid_set_log_function (FLUID_DBG, NULL, NULL);
#endif

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

  delete_fluid_settings (bag.settings); /* not needed anymore */

#if !GST_CHECK_VERSION(0,10,31)
  g_object_class_override_property (gobject_class, PROP_DOCU_URI,
      "documentation-uri");
#endif

  // register own properties

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE_LENGTH,
      g_param_spec_int ("note-length", _("Note length"),
          _("Length of a note in ticks (buffers)"),
          1, 100, 4,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NOTE_VELOCITY,
      g_param_spec_int ("note-velocity", _("Note velocity"),
          _("Velocity of a note"),
          0, 127, 100,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROGRAM,
      g_param_spec_int ("program", _("Sound program"),
          _("Sound program number"),
          0, (0x7F << 7 | 0x7F), 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INSTRUMENT_PATCH,
      g_param_spec_string ("instrument-patch", _("Instrument patch file"),
          _("Path to soundfont intrument patch file"),
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_INTERP,
      g_param_spec_enum ("interp", _("Interpolation"),
          _("Interpolation type"),
          INTERPOLATION_TYPE,
          FLUID_INTERP_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REVERB_ENABLE,
      g_param_spec_boolean ("reverb-enable", _("Reverb enable"),
          _("Reverb enable"),
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REVERB_ROOM_SIZE,
      g_param_spec_double ("reverb-room-size", _("Reverb room size"),
          _("Reverb room size"),
          0.0, 1.2, 0.4, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REVERB_DAMP,
      g_param_spec_double ("reverb-damp", _("Reverb damp"),
          _("Reverb damp"),
          0.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REVERB_WIDTH,
      g_param_spec_double ("reverb-width", _("Reverb width"),
          _("Reverb width"),
          0.0, 100.0, 2.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REVERB_LEVEL,
      g_param_spec_double ("reverb-level", _("Reverb level"),
          _("Reverb level"),
          -30.0, 30.0, 4.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_ENABLE,
      g_param_spec_boolean ("chorus-enable", _("Chorus enable"),
          _("Chorus enable"),
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_COUNT,
      g_param_spec_int ("chorus-count", _("Chorus count"),
          _("Number of chorus delay lines"),
          1, 99, 3, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_LEVEL,
      g_param_spec_double ("chorus-level", _("Chorus level"),
          _("Output level of each chorus line"),
          0.0, 10.0, 2.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_FREQ,
      g_param_spec_double ("chorus-freq", _("Chorus freq"),
          _("Chorus modulation frequency (Hz)"),
          0.3, 5.0, 0.3, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_DEPTH,
      g_param_spec_double ("chorus-depth", _("Chorus depth"),
          _("Chorus depth"),
          0.0, 10.0, 8.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CHORUS_WAVEFORM,
      g_param_spec_enum ("chorus-waveform", _("Chorus waveform"),
          _("Chorus waveform type"),
          CHORUS_WAVEFORM_TYPE,
          FLUID_CHORUS_MOD_SINE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_fluidsynth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtFluidsynth *src = GSTBT_FLUIDSYNTH (object);

  if (src->dispose_has_run)
    return;

  /* is it one of the dynamically installed properties? */
  if (prop_id >= FIRST_DYNAMIC_PROP && prop_id < last_property_id) {
    gint retval;
    gchar *name = dynamic_prop_names[prop_id - FIRST_DYNAMIC_PROP];

    switch (G_PARAM_SPEC_VALUE_TYPE (pspec)) {
      case G_TYPE_INT:
        retval = fluid_settings_setint (src->settings, name,
            g_value_get_int (value));
        break;
      case G_TYPE_DOUBLE:
        retval = fluid_settings_setnum (src->settings, name,
            g_value_get_double (value));
        break;
      case G_TYPE_STRING:
        retval = fluid_settings_setstr (src->settings, name,
            (char *) g_value_get_string (value));
        break;
      default:
        g_critical ("Unexpected FluidSynth dynamic property type");
        return;
    }

    if (!retval)
      g_critical ("FluidSynth failure while setting FluidSynth property '%s'",
          name);

    return;
  }

  switch (prop_id) {
    case PROP_NOTE:
      src->note = g_value_get_enum (value);
      if (src->note) {
        if (src->note == GSTBT_NOTE_OFF) {
          fluid_synth_noteoff (src->fluid, /*chan */ 0, src->key);
          src->cur_note_length = 0;
        } else {
          // start note-off counter
          src->cur_note_length = src->note_length;
          src->key = src->note - GSTBT_NOTE_C_0;
          fluid_synth_noteon (src->fluid, /*chan */ 0, src->key, src->velocity);
        }
      }
      break;
    case PROP_NOTE_LENGTH:
      src->note_length = g_value_get_int (value);
      break;
    case PROP_NOTE_VELOCITY:
      src->velocity = g_value_get_int (value);
      break;
    case PROP_PROGRAM:
      src->program = g_value_get_int (value);
      GST_INFO ("Switch to program: %d, %d", src->program >> 7,
          src->program & 0x7F);
      fluid_synth_program_select (src->fluid, /*chan */ 0,
          src->instrument_patch, src->program >> 7, src->program & 0x7F);
      break;
      // not yet GST_PARAM_CONTROLLABLE
    case PROP_INSTRUMENT_PATCH:
      // unload old patch
      g_free (src->instrument_patch_path);
      fluid_synth_sfunload (src->fluid, src->instrument_patch, TRUE);
      // load new patch
      src->instrument_patch_path = g_value_dup_string (value);
      GST_INFO ("Trying to load load soundfont: '%s'",
          src->instrument_patch_path);
      if ((src->instrument_patch =
              fluid_synth_sfload (src->fluid, src->instrument_patch_path,
                  TRUE)) == -1) {
        GST_WARNING ("Couldn't load soundfont: '%s'",
            src->instrument_patch_path);
      } else {
        GST_INFO ("soundfont loaded as %d", src->instrument_patch);
        //fluid_synth_program_reset(src->fluid);
        fluid_synth_program_select (src->fluid, /*chan */ 0,
            src->instrument_patch, src->program >> 7, src->program & 0x7F);
      }
      break;
    case PROP_INTERP:
      src->interp = g_value_get_enum (value);
      fluid_synth_set_interp_method (src->fluid, -1, src->interp);
      break;
    case PROP_REVERB_ENABLE:
      src->reverb_enable = g_value_get_boolean (value);
      fluid_synth_set_reverb_on (src->fluid, src->reverb_enable);
      break;
    case PROP_REVERB_ROOM_SIZE:
      src->reverb_room_size = g_value_get_double (value);
      src->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (src);
      break;
    case PROP_REVERB_DAMP:
      src->reverb_damp = g_value_get_double (value);
      src->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (src);
      break;
    case PROP_REVERB_WIDTH:
      src->reverb_width = g_value_get_double (value);
      src->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (src);
      break;
    case PROP_REVERB_LEVEL:
      src->reverb_level = g_value_get_double (value);
      src->reverb_update = TRUE;
      gst_fluidsynth_update_reverb (src);
      break;
    case PROP_CHORUS_ENABLE:
      src->chorus_enable = g_value_get_boolean (value);
      fluid_synth_set_chorus_on (src->fluid, src->chorus_enable);
      break;
    case PROP_CHORUS_COUNT:
      src->chorus_count = g_value_get_int (value);
      src->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (src);
      break;
    case PROP_CHORUS_LEVEL:
      src->chorus_level = g_value_get_double (value);
      src->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (src);
      break;
    case PROP_CHORUS_FREQ:
      src->chorus_freq = g_value_get_double (value);
      src->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (src);
      break;
    case PROP_CHORUS_DEPTH:
      src->chorus_depth = g_value_get_double (value);
      src->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (src);
      break;
    case PROP_CHORUS_WAVEFORM:
      src->chorus_waveform = g_value_get_enum (value);
      src->chorus_update = TRUE;
      gst_fluidsynth_update_chorus (src);
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
  GstBtFluidsynth *src = GSTBT_FLUIDSYNTH (object);

  if (src->dispose_has_run)
    return;

  /* is it one of the dynamically installed properties? */
  if (prop_id >= FIRST_DYNAMIC_PROP && prop_id < last_property_id) {
    gchar *s;
    gdouble d;
    gint i;
    gint retval;
    gchar *name = dynamic_prop_names[prop_id - FIRST_DYNAMIC_PROP];

    switch (G_PARAM_SPEC_VALUE_TYPE (pspec)) {
      case G_TYPE_INT:
        retval = fluid_settings_getint (src->settings, name, &i);
        if (retval)
          g_value_set_int (value, i);
        break;
      case G_TYPE_DOUBLE:
        retval = fluid_settings_getnum (src->settings, name, &d);
        if (retval)
          g_value_set_double (value, d);
        break;
      case G_TYPE_STRING:
        retval = fluid_settings_getstr (src->settings, name, &s);
        if (retval)
          g_value_set_string (value, s);
        break;
      default:
        g_critical ("Unexpected FluidSynth dynamic property type");
        return;
    }

    if (!retval)
      g_critical ("Failed to get FluidSynth property '%s'", name);

    return;
  }

  switch (prop_id) {
    case PROP_NOTE_LENGTH:
      g_value_set_int (value, src->note_length);
      break;
    case PROP_NOTE_VELOCITY:
      g_value_set_int (value, src->velocity);
      break;
    case PROP_PROGRAM:
      g_value_set_int (value, src->program);
      break;
      // not yet GST_PARAM_CONTROLLABLE
    case PROP_INSTRUMENT_PATCH:
      g_value_set_string (value, src->instrument_patch_path);
      break;
    case PROP_INTERP:
      g_value_set_enum (value, src->interp);
      break;
    case PROP_REVERB_ENABLE:
      g_value_set_boolean (value, src->reverb_enable);
      break;
    case PROP_REVERB_ROOM_SIZE:
      g_value_set_double (value, src->reverb_room_size);
      break;
    case PROP_REVERB_DAMP:
      g_value_set_double (value, src->reverb_damp);
      break;
    case PROP_REVERB_WIDTH:
      g_value_set_double (value, src->reverb_width);
      break;
    case PROP_REVERB_LEVEL:
      g_value_set_double (value, src->reverb_level);
      break;
    case PROP_CHORUS_ENABLE:
      g_value_set_boolean (value, src->chorus_enable);
      break;
    case PROP_CHORUS_COUNT:
      g_value_set_int (value, src->chorus_count);
      break;
    case PROP_CHORUS_LEVEL:
      g_value_set_double (value, src->chorus_level);
      break;
    case PROP_CHORUS_FREQ:
      g_value_set_double (value, src->chorus_freq);
      break;
    case PROP_CHORUS_DEPTH:
      g_value_set_double (value, src->chorus_depth);
      break;
    case PROP_CHORUS_WAVEFORM:
      g_value_set_enum (value, src->chorus_waveform);
      break;
#if !GST_CHECK_VERSION(0,10,31)
      // help iface
    case PROP_DOCU_URI:
      g_value_set_static_string (value,
          "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S
          "html" G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "" PACKAGE
          "-GstFluidSynth.html");
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_fluidsynth_init (GstBtFluidsynth * src, GstBtFluidsynthClass * g_class)
{
  /* set base parameters */
  src->note_length = 4;
  src->velocity = 100;

  src->settings = new_fluid_settings ();
  //fluid_settings_setstr(src->settings, "audio.driver", "alsa");
  if (!(fluid_settings_setnum (src->settings, "synth.sample-rate",
              ((GstBtAudioSynth *) src)->samplerate))) {
    GST_WARNING ("Can't set samplerate : %d",
        ((GstBtAudioSynth *) src)->samplerate);
  }

  /* create new FluidSynth */
  src->fluid = new_fluid_synth (src->settings);
  if (!src->fluid) {            /* FIXME - Element will likely crash if used after new_fluidsynth fails */
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
  src->midi_router =
      new_fluid_midi_router (src->settings,
      fluid_synth_handle_midi_event, (void *) src);
  if (src->midi_router) {
    fluid_synth_set_midi_router (src->fluid, src->midi_router);
    src->midi =
        new_fluid_midi_driver (src->settings,
        fluid_midi_router_handle_midi_event, (void *) (src->midi_router));
    if (!src->midi)
      g_warning ("Failed to create FluidSynth MIDI input driver");
  } else
    g_warning ("Failed to create MIDI input router");

  gst_fluidsynth_update_reverb (src);   /* update reverb settings */
  gst_fluidsynth_update_chorus (src);   /* update chorus settings */

  /* FIXME(ensonic): temporary for testing, see comment at the top */
  {
    gchar **sf2, *sf2s[] = {
      "/usr/share/sounds/sf2/FluidR3_GM.sf2",
      "/usr/share/sounds/sf2/FluidR3_GS.sf2",
      "/usr/share/sounds/sf2/Vintage_Dreams_Waves_v2.sf2",
      "/usr/share/doc/libfluidsynth-dev/examples/example.sf2",
      NULL
    };
    sf2 = sf2s;
    src->instrument_patch = -1;
    while ((src->instrument_patch == -1) && *sf2) {
      GST_INFO ("trying '%s'", *sf2);
      if (g_file_test (*sf2, G_FILE_TEST_IS_REGULAR)) {
        src->instrument_patch = fluid_synth_sfload (src->fluid, *sf2, TRUE);
      }
      sf2++;
    }
  }
#if 0
  if (src->instrument_patch == -1) {
    gchar *path =
        g_strdup_printf ("%s/sbks/synth/FlangerSaw.SF2", g_get_home_dir ());
    src->instrument_patch = fluidsynth_sfload (src->fluid, path, TRUE);
    g_free (path);
  }
#endif
  if (src->instrument_patch == -1) {
    GST_WARNING ("Couldn't load any soundfont");
  } else {
    GST_INFO ("soundfont loaded as %d", src->instrument_patch);
  }
}

static gboolean
gst_fluidsynth_setup (GstPad * pad, GstCaps * caps)
{
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  /* Set channels to 2 */
  if (!gst_structure_fixate_field_nearest_int (structure, "channels", 2))
    return FALSE;

  return TRUE;
}

static void
gst_fluidsynth_update_reverb (GstBtFluidsynth * gstsynth)
{
  if (!gstsynth->reverb_update)
    return;

  if (gstsynth->reverb_enable)
    fluid_synth_set_reverb (gstsynth->fluid, gstsynth->reverb_room_size,
        gstsynth->reverb_damp, gstsynth->reverb_width, gstsynth->reverb_level);

  fluid_synth_set_reverb_on (gstsynth->fluid, gstsynth->reverb_enable);
  gstsynth->reverb_update = FALSE;
}

static void
gst_fluidsynth_update_chorus (GstBtFluidsynth * gstsynth)
{
  if (!gstsynth->chorus_update)
    return;

  if (gstsynth->chorus_enable)
    fluid_synth_set_chorus (gstsynth->fluid, gstsynth->chorus_count,
        gstsynth->chorus_level, gstsynth->chorus_freq,
        gstsynth->chorus_depth, gstsynth->chorus_waveform);

  fluid_synth_set_chorus_on (gstsynth->fluid, TRUE);
  gstsynth->chorus_update = FALSE;
}

static void
gst_fluidsynth_process (GstBtAudioSynth * base, GstBuffer * data)
{
  GstBtFluidsynth *src = ((GstBtFluidsynth *) base);

  if (src->cur_note_length) {
    src->cur_note_length--;
    if (!src->cur_note_length) {
      fluid_synth_noteoff (src->fluid, /*chan */ 0, src->key);
      GST_INFO ("note-off: %d", src->key);
    }
  }
  fluid_synth_write_s16 (src->fluid,
      ((GstBtAudioSynth *) src)->generate_samples_per_buffer,
      GST_BUFFER_DATA (data), 0, 2, GST_BUFFER_DATA (data), 1, 2);
}

static void
gst_fluidsynth_dispose (GObject * object)
{
  GstBtFluidsynth *gstsynth = GSTBT_FLUIDSYNTH (object);

  if (gstsynth->dispose_has_run)
    return;
  gstsynth->dispose_has_run = TRUE;

  if (gstsynth->midi)
    delete_fluid_midi_driver (gstsynth->midi);
  if (gstsynth->midi_router)
    delete_fluid_midi_router (gstsynth->midi_router);
  if (gstsynth->fluid)
    delete_fluid_synth (gstsynth->fluid);

  gstsynth->midi = NULL;
  gstsynth->midi_router = NULL;
  gstsynth->fluid = NULL;

  g_free (gstsynth->instrument_patch_path);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GType
gstbt_fluidsynth_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    const GTypeInfo element_type_info = {
      sizeof (GstBtFluidsynthClass),
      (GBaseInitFunc) gst_fluidsynth_base_init,
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_fluidsynth_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstBtFluidsynth),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gst_fluidsynth_init
    };
    const GInterfaceInfo property_meta_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };
    const GInterfaceInfo tempo_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };
#if !GST_CHECK_VERSION(0,10,31)
    const GInterfaceInfo help_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };
#endif
    const GInterfaceInfo preset_interface_info = {
      NULL,                     /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    type =
        g_type_register_static (GSTBT_TYPE_AUDIO_SYNTH, "GstBtFluidsynth",
        &element_type_info, (GTypeFlags) 0);
    g_type_add_interface_static (type, GSTBT_TYPE_PROPERTY_META,
        &property_meta_interface_info);
    g_type_add_interface_static (type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
#if !GST_CHECK_VERSION(0,10,31)
    g_type_add_interface_static (type, GSTBT_TYPE_HELP, &help_interface_info);
#endif
    g_type_add_interface_static (type, GST_TYPE_PRESET, &preset_interface_info);
  }
  return type;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "fluidsynth", GST_DEBUG_FG_WHITE
      | GST_DEBUG_BG_BLACK, "FluidSynth wavetable synthesizer");

  /* initialize gst controller library */
  gst_controller_init (NULL, NULL);

  return gst_element_register (plugin, "fluidsynth", GST_RANK_NONE,
      GSTBT_TYPE_FLUIDSYNTH);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "fluidsynth",
    "FluidSynth wavetable synthesizer",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
