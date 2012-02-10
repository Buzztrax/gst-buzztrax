/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbml.h: Header for BML plugin
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


#ifndef __GST_BML_H__
#define __GST_BML_H__

#include "plugin.h"

G_BEGIN_DECLS

typedef float BMLData;

typedef enum {
  ARG_BPM=1,  // tempo iface
  ARG_TPB,
  ARG_STPT,
  ARG_HOST_CALLBACKS, // buzztard host callbacks
  //ARG_VOICES,  // child bin iface
  //ARG_DOCU_URI,// help iface
  ARG_LAST
} GstBMLPropertyIDs;

typedef enum {
  PT_NOTE=0,
  PT_SWITCH,
  PT_BYTE,
  PT_WORD,
  PT_ENUM,
  PT_ATTR
} GstBMLParameterTypes;

typedef enum {
  MT_MASTER=0,
  MT_GENERATOR,
  MT_EFFECT
} GstBMLMachineTypes;

extern GQuark gst_bml_property_meta_quark_type;

#define GST_BML(obj)         ((GstBML *)(&obj->bml))
#define GST_BML_CLASS(klass) ((GstBMLClass *)(&klass->bml_class))

typedef struct _GstBML GstBML;
typedef struct _GstBMLClass GstBMLClass;

struct _GstBML {
  gboolean dispose_has_run;

  GstElement *self;

  // the buzz machine handle (to use with libbml API)
  gpointer bm;

  // the number of voices the plugin has
  gulong num_voices;
  GList *voices;

  // tempo handling
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  gulong subtick_count;

  // pads
  GstPad **sinkpads,**srcpads;

  gint samplerate, samples_per_buffer;
  
  // array with an entry for each parameter
  // flags that a g_object_set has set a value for a trigger param
  gint * volatile triggers_changed;

  /* < private > */
  gboolean tags_pushed;			/* send tags just once ? */
  GstClockTime ticktime;
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_eos;
  gboolean eos_reached;
  gboolean reverse;                  /* play backwards */
};

struct _GstBMLClass {
  // the buzz machine handle (to use with libbml API)
  gpointer bmh;

  gchar *dll_name;
  gchar *help_uri;
  gchar *preset_path;

  // the type for a voice parameter faccade
  GType voice_type;

  // preset names and data
  GList *presets;
  GHashTable *preset_data;
  GHashTable *preset_comments;

  // basic machine data
  gint numsinkpads,numsrcpads;
  gint numattributes,numglobalparams,numtrackparams;

  gint input_channels,output_channels;
  
  // param specs
  GParamSpec **global_property,**track_property;
};

G_END_DECLS

#endif /* __GST_BML_H__ */
