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
 * SECTION:gstnote2frequency
 * @short_description: helper class for unit conversion
 *
 * An instance of this class can translate a musical note to a frequency, while
 * taking a specific tuning into account.
 */

#include "note2frequency.h"
#include "math.h"
#include "string.h"

//-- signal ids

//-- property ids

enum {
  GST_NOTE_2_FREQUENCY_TUNING=1
};

static GObjectClass *parent_class=NULL;

//-- enums

GType gst_note_2_frequency_tuning_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static GEnumValue values[] = {
      { GST_NOTE_2_FREQUENCY_CROMATIC,"GST_NOTE_2_FREQUENCY_CROMATIC","cromatic tuning" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("GstNote2FrequencyTuning", values);
  }
  return type;
}

//-- constructor methods

/**
 * gst_note_2_frequency_new:
 * @tuning: the #GstNote2FrequencyTuning to use
 *
 * Create a new instance of a note to frequency translator, that will use the
 * given @tuning.
 *
 * Returns: a new #GstNote2Frequency translator
 */
GstNote2Frequency *gst_note_2_frequency_new(GstNote2FrequencyTuning tuning) {
  GstNote2Frequency *self;
  
  if(!(self=GST_NOTE_2_FREQUENCY(g_object_new(GST_TYPE_NOTE_2_FREQUENCY,"tuning",tuning,NULL)))) {
    goto Error;
  }  
  return(self);
Error:
  return(NULL);
}

//-- methods

static gdouble gst_note_2_frequency_translate_cromatic(GstNote2Frequency *self,guint octave, guint tone) {
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

static gst_note_2_frequency_change_tuning(GstNote2Frequency *self) {
  switch(self->tuning) {
	case GST_NOTE_2_FREQUENCY_CROMATIC:
	  self->translate=gst_note_2_frequency_translate_cromatic;
	  break;
  }
}

/**
 * gst_note_2_frequency_translate_from_string:
 * @self: a #GstNote2Frequency
 * @note: a musical note in string representation
 *
 * Converts the string representation of a musical note such as 'C-3' or 'd#4'
 * to a frequency in Hz.
 *
 * Returns: the frequency of the note or 0.0 in case of an error
 */
gdouble gst_note_2_frequency_translate_from_string(GstNote2Frequency *self,gchar *note) {
  guint tone, octave;
  
  g_return_val_if_fail(note,0.0);
  g_return_val_if_fail((strlen(note)==3),0.0);
  g_return_val_if_fail((note[1]=='-' || note[1]=='#'),0.0);

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

  return(self->translate(self,octave,tone));
}

/**
 * gst_note_2_frequency_translate_from_number:
 * @self: a #GstNote2Frequency
 * @note: a musical note as number
 *
 * Converts the numerical number of a note to a frequency in Hz. A value of 0
 * for @note represents 'c-0'. The highes supported value is 'b-9' (or 'h-9')
 * which is (10*12)-1.
 *
 * Returns: the frequency of the note or 0.0 in case of an error
 */
gdouble gst_note_2_frequency_translate_from_number(GstNote2Frequency *self,guint note) {
  guint tone, octave;
  
  g_return_val_if_fail(note<(10*12),0.0);
  
  octave=note/12;
  tone=note-(octave*12);

  return(self->translate(self,octave,tone));
}
//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void gst_note_2_frequency_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  GstNote2Frequency *self = GST_NOTE_2_FREQUENCY(object);
  
  if(self->dispose_has_run) return;
	
  switch (property_id) {
    case GST_NOTE_2_FREQUENCY_TUNING: {
      g_value_set_enum(value, self->tuning);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void gst_note_2_frequency_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GstNote2Frequency *self = GST_NOTE_2_FREQUENCY(object);
  
  if(self->dispose_has_run) return;
	
  switch (property_id) {
    case GST_NOTE_2_FREQUENCY_TUNING: {
      self->tuning = g_value_get_enum(value);
	  gst_note_2_frequency_change_tuning(self);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void gst_note_2_frequency_dispose(GObject *object) {
  GstNote2Frequency *self = GST_NOTE_2_FREQUENCY(object);

  if(self->dispose_has_run) return;
  self->dispose_has_run = TRUE;
  
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void gst_note_2_frequency_finalize(GObject *object) {
  GstNote2Frequency *self = GST_NOTE_2_FREQUENCY(object);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void gst_note_2_frequency_init(GTypeInstance *instance, gpointer g_class) {
  GstNote2Frequency *self = GST_NOTE_2_FREQUENCY(instance);
  self->dispose_has_run = FALSE;
}

static void gst_note_2_frequency_class_init(GstNote2FrequencyClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
  gobject_class->set_property = gst_note_2_frequency_set_property;
  gobject_class->get_property = gst_note_2_frequency_get_property;
  gobject_class->dispose      = gst_note_2_frequency_dispose;
  gobject_class->finalize     = gst_note_2_frequency_finalize;

  g_object_class_install_property(gobject_class,GST_NOTE_2_FREQUENCY_TUNING,
                                  g_param_spec_enum("tuning",
                                    "tuning prop",
                                    "selection frequency tuning table",
                                    GST_TYPE_NOTE_2_FREQUENCY_TUNING,
									GST_NOTE_2_FREQUENCY_CROMATIC,
                                    G_PARAM_READWRITE));

}

GType gst_note_2_frequency_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GstNote2FrequencyClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)gst_note_2_frequency_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(GstNote2Frequency),
      0,   // n_preallocs
      (GInstanceInitFunc)gst_note_2_frequency_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"GstNote2Frequency",&info,0);
  }
  return type;
}
