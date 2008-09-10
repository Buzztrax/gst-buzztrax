/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * note2frequency.c: helper class for unit conversion
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
 * SECTION:gstbttoneconversion
 * @short_description: helper class for tone unit conversion
 *
 * An instance of this class can translate a musical note to a frequency, while
 * taking a specific tuning into account. It also provides conversion betwen
 * notes as numbers and strings.
 */

#include "toneconversion.h"
#include "math.h"
#include "string.h"

//-- signal ids

//-- property ids

enum {
  GSTBT_TONE_CONVERSION_TUNING=1
};

static GObjectClass *parent_class=NULL;

//-- enums

GType gstbt_tone_conversion_tuning_get_type(void) {
  static GType type = 0;

  if(G_UNLIKELY(!type)) {
    static const GEnumValue values[] = {
      { GSTBT_TONE_CONVERSION_CROMATIC,"GSTBT_TONE_CONVERSION_CROMATIC","cromatic tuning" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("GstBtToneConversionTuning", values);
  }
  return type;
}

//-- helpers

static gboolean note_string_2_value (const gchar *note, guint *tone, guint *octave) {
  if(!note) return(FALSE);
  if((strlen(note)!=3)) return(FALSE);
  if(!(note[1]=='-' || note[1]=='#')) return (FALSE);
  if(!(note[2]>='0' && note[2]<='9')) return (FALSE);
  g_assert(tone);
  g_assert(octave);

  // parse note
  switch(note[0]) {
    case 'c':
    case 'C':
      *tone=(note[1]=='-')?0:1;
      break;
    case 'd':
    case 'D':
      *tone=(note[1]=='-')?2:3;
      break;
    case 'e':
    case 'E':
      *tone=4;
      break;
    case 'f':
    case 'F':
      *tone=(note[1]=='-')?5:6;
      break;
    case 'g':
    case 'G':
      *tone=(note[1]=='-')?7:8;
      break;
    case 'a':
    case 'A':
      *tone=(note[1]=='-')?9:10;
      break;
    case 'b':
    case 'B':
    case 'h':
    case 'H':
      *tone=11;
      break;
    default:
      g_return_val_if_reached(FALSE);
  }
  *octave=atoi(&note[2]);
  return(TRUE);
}

static gboolean note_value_2_string (guint note, guint *tone, guint *octave) {
  g_return_val_if_fail(note<((16*9)+12),0);
  g_assert(tone);
  g_assert(octave);
  
  if(note==0) return(FALSE);

  note-=1;
  *octave=note/16;
  *tone=note-(*octave*16);
  return(TRUE);
}


//-- constructor methods

/**
 * gstbt_tone_conversion_new:
 * @tuning: the #GstBtToneConversionTuning to use
 *
 * Create a new instance of a note to frequency translator, that will use the
 * given @tuning.
 *
 * Returns: a new #GstBtToneConversion translator
 */
GstBtToneConversion *gstbt_tone_conversion_new(GstBtToneConversionTuning tuning) {
  GstBtToneConversion *self;

  if(!(self=GSTBT_TONE_CONVERSION(g_object_new(GSTBT_TYPE_TONE_CONVERSION,"tuning",tuning,NULL)))) {
    goto Error;
  }
  return(self);
Error:
  return(NULL);
}

//-- methods

static gdouble gstbt_tone_conversion_translate_cromatic(GstBtToneConversion *self,guint octave, guint tone) {
  gdouble frequency=0.0, step;
  guint steps, i;

  g_assert(tone<12);
  g_assert(octave<10);

  /* calculated base frequency A-0=55 Hz*/
  frequency=(gdouble)(55<<octave);
  /* do tone stepping */
  step=pow(2.0,(1.0/12.0));
  if(tone<=9) {
    // go down
    steps=9-tone;
    for(i=0;i<steps;i++) frequency/=step;
  }
  else {
    // go up
    steps=tone-9;
    for(i=0;i<steps;i++) frequency*=step;
  }
  return(frequency);
}

static gstbt_tone_conversion_change_tuning(GstBtToneConversion *self) {
  switch(self->tuning) {
    case GSTBT_TONE_CONVERSION_CROMATIC:
      self->translate=gstbt_tone_conversion_translate_cromatic;
      break;
  }
}


/**
 * gstbt_tone_conversion_translate_from_string:
 * @self: a #GstBtToneConversion
 * @note: a musical note in string representation
 *
 * Converts the string representation of a musical note such as 'C-3' or 'd#4'
 * to a frequency in Hz.
 *
 * Returns: the frequency of the note or 0.0 in case of an error
 */
gdouble gstbt_tone_conversion_translate_from_string(GstBtToneConversion *self,gchar *note) {
  guint tone, octave;

  if(note_string_2_value(note,&tone,&octave))
    return(self->translate(self,octave,tone));
  else
    return(0.0);
}

/**
 * gstbt_tone_conversion_translate_from_number:
 * @self: a #GstBtToneConversion
 * @note: a musical note as number
 *
 * Converts the numerical number of a note to a frequency in Hz. A value of 1
 * for @note represents 'c-0'. The highest supported value is 'b-9' (or 'h-9')
 * which is 1+(9*16)+11 (1 octave has 12 tones, 4 are left unused for easy
 * coding).
 *
 * Returns: the frequency of the note or 0.0 in case of an error
 */
gdouble gstbt_tone_conversion_translate_from_number(GstBtToneConversion *self,guint note) {
  guint tone, octave;

  if(note_value_2_string(note,&tone,&octave))
    return(self->translate(self,octave,tone));
  else
    return(0.0);
}

/**
 * gstbt_tone_conversion_note_string_2_number:
 * @note: a musical note in string representation
 * 
 * Converts the string representation of a musical note such as 'C-3' or 'd#4'
 * to a note number.
 *
 * Returns: the note number or 0 in case of an error.
 */
guint gstbt_tone_conversion_note_string_2_number(const gchar *note) {
  guint tone, octave;
  
  if(note_string_2_value(note,&tone,&octave))
    return (1+(octave<<4)+tone);
  else
    return(0);
}

/**
 * gstbt_tone_conversion_note_number_2_string:
 * @note: a musical note as number
 *
 * Converts the numerical number of a note to a string. A value of 1
 * for @note represents 'c-0'. The highest supported value is 'b-9' (or 'h-9')
 * which is 1+(9*16)+11 (1 octave has 12 tones, 4 are left unused for easy
 * coding).
 *
 * Returns: the note as string or an empty string in the case of an error.
 */
const gchar *gstbt_tone_conversion_note_number_2_string(guint note) {
  static gchar str[4];
  static const gchar *key[12]= { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };
  guint tone, octave;
  
  if(note_value_2_string(note,&tone,&octave)) {
    sprintf(str,"%2s%1d",key[tone],octave);
    return(str);
  }
  else
    return("");  
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void gstbt_tone_conversion_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  GstBtToneConversion *self = GSTBT_TONE_CONVERSION(object);

  if(self->dispose_has_run) return;

  switch (property_id) {
    case GSTBT_TONE_CONVERSION_TUNING: {
      g_value_set_enum(value, self->tuning);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void gstbt_tone_conversion_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GstBtToneConversion *self = GSTBT_TONE_CONVERSION(object);

  if(self->dispose_has_run) return;

  switch (property_id) {
    case GSTBT_TONE_CONVERSION_TUNING: {
      self->tuning = g_value_get_enum(value);
	  gstbt_tone_conversion_change_tuning(self);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void gstbt_tone_conversion_dispose(GObject *object) {
  GstBtToneConversion *self = GSTBT_TONE_CONVERSION(object);

  if(self->dispose_has_run) return;
  self->dispose_has_run = TRUE;

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void gstbt_tone_conversion_finalize(GObject *object) {
  GstBtToneConversion *self = GSTBT_TONE_CONVERSION(object);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void gstbt_tone_conversion_init(GTypeInstance *instance, gpointer g_class) {
  GstBtToneConversion *self = GSTBT_TONE_CONVERSION(instance);
  self->dispose_has_run = FALSE;
}

static void gstbt_tone_conversion_class_init(GstBtToneConversionClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);

  gobject_class->set_property = gstbt_tone_conversion_set_property;
  gobject_class->get_property = gstbt_tone_conversion_get_property;
  gobject_class->dispose      = gstbt_tone_conversion_dispose;
  gobject_class->finalize     = gstbt_tone_conversion_finalize;

  g_object_class_install_property(gobject_class,GSTBT_TONE_CONVERSION_TUNING,
                                  g_param_spec_enum("tuning",
                                    "tuning prop",
                                    "selection frequency tuning table",
                                    GSTBT_TYPE_TONE_CONVERSION_TUNING,
									GSTBT_TONE_CONVERSION_CROMATIC,
                                    G_PARAM_READWRITE));

}

GType gstbt_tone_conversion_get_type(void) {
  static GType type = 0;

  if(G_UNLIKELY(!type)) {
    const GTypeInfo info = {
      sizeof(GstBtToneConversionClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)gstbt_tone_conversion_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(GstBtToneConversion),
      0,   // n_preallocs
      (GInstanceInitFunc)gstbt_tone_conversion_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"GstBtToneConversion",&info,0);
  }
  return type;
}